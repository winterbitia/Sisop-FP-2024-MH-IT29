/*
     ___  ___ _ ____   _____ _ __ ___
    / __|/ _ \ '__\ \ / / _ \ '__/ __|
    \__ \  __/ |   \ V /  __/ | | (__
    |___/\___|_|    \_/ \___|_|(_)___|

    COMPILING INSTRUCTIONS:
    Compile the server with the following command:
        gcc server.c -o <file output> -lcrypt

    COMMANDS:
    The server will run in the background by default, as a daemon process.
        ./<file output>
    This will run the server in the foreground, allowing you to see the output.
        ./<file output> -f
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <crypt.h>
#include <stdbool.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>

//=========//
// GLOBALS //
//=========//

// Define
#define PORT 8080
#define MAX_CLIENTS 10
#define MAX_BUFFER 1028
#define HASHCODE "sk1b1d1_to1l3t_r1zz_gy477_s1gm4"

// Dir variables
char cwd[MAX_BUFFER];
char users_csv[MAX_BUFFER];
char channels_csv[MAX_BUFFER];

// Socket variables
int server_fd, gclient_fd;
struct sockaddr_in address;
socklen_t addrlen = sizeof(address);

// Save client information
typedef struct {
    // Client socket and address
    int socket_fd;
    struct sockaddr_in address;
    // Client details
    int id;
    char username[MAX_BUFFER];
    char role[5];
    char channel[100];
    char room[100];
} client_data;

//=================//
// PRE-DECLARATION //
//=================//

// Main server
void daemonize();
void start_server();
void *handle_client(void *arg);
void  handle_input(void *arg);

// Account Handlers
void register_user(char *username, char *password, client_data *client);
int  login_user(char *username, char *password, client_data *client);

// Channel Handlers
void make_directory(char *path);
void admin_init_channel(char *path_auth, client_data *client);
void create_channel(char *channel, char *key, client_data *client);
void list_channels(client_data *client);

//===========================================================================================//
//----------------------------------------- SERVER ------------------------------------------//
//===========================================================================================//

//======//
// MAIN //
//======//
int main(int argc, char *argv[]){
    // Get current directory
    getcwd(cwd, sizeof(cwd));    
    sprintf(users_csv, "%s/users.csv", cwd);
    sprintf(channels_csv, "%s/channels.csv", cwd);

    // Start server
    start_server();

    // Foreground handling
    if (argc > 1 && strcmp(argv[1], "-f") == 0){
        printf("Server: running in foreground!\n");
    } else daemonize();

    // Accept incoming connections
    while (1){
        if ((gclient_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0){
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        // Prepare client data
        client_data *client = (client_data *)malloc(sizeof(client_data));
        client->socket_fd = gclient_fd;
        memset(client->username, 0, MAX_BUFFER);
        memset(client->role, 0, 5);

        // Create thread for client
        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, (void *)client);

        // Daemon sleep
        sleep(1);
    }
}

//==================//
// SERVER FUNCTIONS //
//==================//

void daemonize(){
    // Fork off the parent process
    pid_t pid, sid;
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    // Change the file mode mask
    umask(0);

    // Create a new SID for the child process
    sid = setsid();
    if (sid < 0) exit(EXIT_FAILURE);

    // Change the current working directory
    if ((chdir("/")) < 0) exit(EXIT_FAILURE);

    // Close out the standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Redirect standard file descriptors to /dev/null
    open("/dev/null", O_RDONLY); // stdin
    open("/dev/null", O_WRONLY); // stdout
    open("/dev/null", O_WRONLY); // stderr
}

void start_server(){
    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Bind socket to port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket to port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, MAX_CLIENTS) < 0){
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    // DEBUGGING
    printf("Server: started on port %d\n", PORT);
}

//===========================================================================================//
//---------------------------------------- HANDLER ------------------------------------------//
//===========================================================================================//

//===============//
// HANDLE CLIENT //
//===============//

void *handle_client(void *arg){
    client_data *client = (client_data *)arg;
    int client_fd = client->socket_fd;

    char buffer[MAX_BUFFER];
    memset(buffer, 0, MAX_BUFFER);

    // Receive data from client
    if (recv(client_fd, buffer, MAX_BUFFER, 0) < 0){
        perror("recv failed");
        exit(EXIT_FAILURE);
    }

    // Parse data from client
    char *command = strtok(buffer, " ");
    char *username = strtok(NULL, " ");
    char *password = strtok(NULL, " ");

    // DEBUGGING
    printf("command: %s, username: %s, password: %s\n", command, username, password);

    // Register user
    if (strcmp(command, "REGISTER") == 0){
        // Call register user function
        register_user(username, password, client);

        // Close client connection
        close(client_fd);
        free(client);
        pthread_exit(NULL);
        return NULL;
    }

    // Login user
    else if (strcmp(command, "LOGIN") == 0){
        // Call login user function
        if (!login_user(username, password, client)){
            // Close client connection when login fails
            close(client_fd);
            free(client);
            pthread_exit(NULL);
            return NULL;
        }

        // DEBUGGING
        printf("LOGIN: %s\n", username);

        // Call handle input function
        handle_input(client);
    }
}

//==============//
// HANDLE INPUT //
//==============//

void handle_input(void *arg){
    client_data *client = (client_data *)arg;
    int client_fd = client->socket_fd;

    char buffer[MAX_BUFFER];
    char response[MAX_BUFFER];

    while(1){
        // DEBUGGING
        printf("Input: waiting %s\n", client->username);

        // Clear buffer
        memset(buffer, 0, MAX_BUFFER);

        // Receive data from client
        if (recv(client_fd, buffer, MAX_BUFFER, 0) < 0){
            perror("recv failed");
            exit(EXIT_FAILURE);
        }

        // Prepare parse data from client
        char *command = strtok(buffer, " ");

        if (strcmp(command, "QUIT") == 0){
            // DEBUGGING
            printf("Force quit: %s\n", client->username);

            // Send response to client
            memset(response, 0, MAX_BUFFER);
            sprintf(response, "QUIT,Gracefully quitting from server...");
            send(client_fd, response, strlen(response), 0);

            // Close client connection
            close(client_fd);
            free(client);
            pthread_exit(NULL);
            return;
            
 } else if (strcmp(command, "CREATE") == 0){
            // Parse data from client
            char *type = strtok(NULL, " ");

            // Check if command is valid
            if (type == NULL){
                // DEBUGGING
                printf("Error: Invalid command\n");

                // Send response to client
                memset(response, 0, MAX_BUFFER);
                sprintf(response, "MSG,Error: Invalid command");
                send(client_fd, response, strlen(response), 0);
                continue;
            }

            // DEBUGGING
            printf("command: %s, type: %s", command, type);

            // Create channel
            if (strcmp(type, "CHANNEL") == 0){
                // Parse data from client
                char *channel = strtok(NULL, " ");
                char *flag = strtok(NULL, " ");
                char *key = strtok(NULL, " ");

                // Check if command is valid
                if (channel == NULL || key == NULL || strcmp(flag, "-k") != 0){
                    // DEBUGGING
                    printf("Error: Invalid command\n");

                    // Send response to client
                    memset(response, 0, MAX_BUFFER);
                    sprintf(response, "MSG,Error: Invalid command");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // DEBUGGING
                printf("channel: %s, key: %s\n", channel, key);

                // Call create channel function
                create_channel(channel, key, client);
            }

 } else if (strcmp(command, "LIST") == 0){
            // Parse data from client
            char *type = strtok(NULL, " ");

            // Check if command is valid
            if (type == NULL){
                // DEBUGGING
                printf("Error: Invalid command\n");

                // Send response to client
                memset(response, 0, MAX_BUFFER);
                sprintf(response, "MSG,Error: Invalid command");
                send(client_fd, response, strlen(response), 0);
                continue;
            }

            // DEBUGGING
            printf("command: %s, type: %s\n", command, type);

            // List channels
            if (strcmp(type, "CHANNEL") == 0){
                // Call list channels function
                list_channels(client);
            }

        } else {
            // DEBUGGING
            printf("Error: Command not found\n");

            // Send response to client
            memset(response, 0, MAX_BUFFER);
            sprintf(response, "MSG,Error: Command not found");
            send(client_fd, response, strlen(response), 0);
        }
    }
}

//===========================================================================================//
//----------------------------------------- ACCOUNTS ----------------------------------------//
//===========================================================================================//

//===============//
// REGISTER USER //
//===============//

void register_user(char *username, char *password, client_data *client) {
    int client_fd = client->socket_fd;

    // Open file
    FILE *file = fopen(users_csv, "a+");

    // Prepare response
    char response[MAX_BUFFER];

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("Error: Unable to open file\n");

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop through id and username
    int id = 0; char namecheck[MAX_BUFFER];
    while (fscanf(file, "%d,%[^,],%*s", &id, namecheck) == 2) {
        // DEBUGGING
        printf("id: %d, name: %s\n", id, namecheck);
        // sleep(1);

        // Fail if username already exists
        if (strcmp(namecheck, username) == 0) {
            // DEBUGGING
            printf("Error: Username already exists\n");

            // Close file
            fclose(file);

            // Send response to client
            sprintf(response, "MSG,Error: Username %s already exists", username);
            send(client_fd, response, strlen(response), 0);
            return;
        }
    }

    // Hash password
    char hash[MAX_BUFFER];
    strcpy(hash,crypt(password, HASHCODE));

    // Set role to USER by default
    char role[6];
    if (id == 0) strcpy(role, "ROOT");
    else         strcpy(role, "USER");

    // DEBUGGING
    printf("id: %d, name: %s, pass: %s, role: %s\n", id+1, username, hash, role);
    
    // Write to file
    fprintf(file, "%d,%s,%s,%s\n", id+1, username, hash, role);
    fclose(file);

    // Send response to client
    sprintf(response, "MSG,Success: User %s registered", username);
    send(client_fd, response, strlen(response), 0);
}

//============//
// LOGIN UsER //
//============//

int login_user(char *username, char *password, client_data *client) {
    int client_fd = client->socket_fd;

    // Hash password from input
    char hash[MAX_BUFFER];
    strcpy(hash,crypt(password, HASHCODE));
    
    // Open file
    FILE *file = fopen(users_csv, "r");

    // Prepare response
    char response[MAX_BUFFER];

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("Error: Unable to open file\n");

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return 0;
    }

    // Loop through id, username, and password
    int id = 0; char namecheck[MAX_BUFFER], passcheck[MAX_BUFFER], role[6];
    while (fscanf(file, "%d,%[^,],%[^,],%s", &id, namecheck, passcheck, role) == 4) {
        // DEBUGGING
        printf("id: %d, name: %s, pass: %s, role: %s\n", id, namecheck, passcheck, role);

        // Fail if username and password do not match
        if (strcmp(namecheck, username) == 0) {
            if (strcmp(passcheck, hash) == 0) {
                // DEBUGGING
                printf("Success: Username and password match\n");

                // Finishing up
                client->id = id;
                strcpy(client->username, username);
                strcpy(client->role, role);
                fclose(file);

                // Send response to client
                sprintf(response, "LOGIN,Welcome to DiscorIT!\n");
                send(client_fd, response, strlen(response), 0);
                return 1;
            }

            // DEBUGGING
            printf("Error: Password does not match\n");

            // Fail if password does not match
            sprintf(response, "MSG,Error: Password does not match");
            send(client_fd, response, strlen(response), 0);
            fclose(file);
            return 0;
        }
    }

    // DEBUGGING
    printf("Error: Username does not exist\n");

    // Send response to client
    sprintf(response, "MSG,Error: Username does not exist");
    send(client_fd, response, strlen(response), 0);
    fclose(file);
    return 0;
}

//===========================================================================================//
//----------------------------------------- CHANNELS ----------------------------------------//
//===========================================================================================//

//================//
// MAKE DIRECTORY //
//================//

void make_directory(char *path) {
    // Create directory
    if (mkdir(path, 0777) == -1)
    // Fail if directory cannot be created
    if (errno != EEXIST) {
        printf("Error: Unable to create directory\n");
        return;
    }
}

//====================//
// INIT CHANNEL ADMIN //
//====================//

void admin_init_channel(char *path_auth, client_data *client){
    // Parse users.csv
    FILE *file = fopen(users_csv, "r");

    // Fail if file cannot be opened
    if (file == NULL) {
        printf("Error: Unable to open file\n");
        return;
    }

    // Loop through id
    int id;
    while (fscanf(file, "%d,%*[^,],%*[^,],%*s", &id) == 1) {
        // DEBUGGING
        printf("id: %d\n", id);

        // Make sure admin id exists
        if (client->id == id) {
            // Write to auth.csv
            FILE *file_auth = fopen(path_auth, "w");
            fprintf(file_auth, "%d,%s,ADMIN\n", id, client->username);
            fclose(file_auth);
        }
    }

    // Close file
    fclose(file);
}

//================//
// CREATE CHANNEL //
//================//

void create_channel(char *channel, char *key, client_data *client) {
    int client_fd = client->socket_fd;

    // Open file
    FILE *file = fopen(channels_csv, "a+");

    // Prepare response
    char response[MAX_BUFFER];

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("Error: Unable to open file\n");

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop through id and channel
    int id = 0; char namecheck[MAX_BUFFER];
    while (fscanf(file, "%d,%[^,],%*s", &id, namecheck) == 2) {
        // DEBUGGING
        printf("id: %d, name: %s\n", id, namecheck);
        // sleep(1);

        // Fail if channel already exists
        if (strcmp(namecheck, channel) == 0) {
            // DEBUGGING
            printf("Error: Channel already exists\n");

            // Close file
            fclose(file);

            // Send response to client
            sprintf(response, "MSG,Error: Channel %s already exists", channel);
            send(client_fd, response, strlen(response), 0);
            return;
        }
    }

    // Hash key
    char hash[MAX_BUFFER];
    strcpy(hash,crypt(key, HASHCODE));

    // Write to file
    fprintf(file, "%d,%s,%s\n", id+1, channel, hash);
    fclose(file);

    // Create directory and all related preparations
    char path[MAX_BUFFER], path_admin[MAX_BUFFER], path_auth[MAX_BUFFER];
    sprintf(path, "%s/%s", cwd, channel);
    sprintf(path_admin, "%s/%s/admin", cwd, channel);
    sprintf(path_auth, "%s/%s/admin/auth.csv", cwd, channel);
    make_directory(path);
    make_directory(path_admin);
    admin_init_channel(path_auth, client);

    // Send response to client
    sprintf(response, "MSG,Success: Channel %s created", channel);
    send(client_fd, response, strlen(response), 0);
}

//===============//
// LIST CHANNELS //
//===============//

void list_channels(client_data *client) {
    int client_fd = client->socket_fd;

    // Open file
    FILE *file = fopen(channels_csv, "r");

    // Prepare response
    char response[MAX_BUFFER];
    sprintf(response, "MSG,");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("Error: Unable to open file\n");

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop through id and channel
    int id = 1; char channel[MAX_BUFFER];
    while (fscanf(file, "%d,%[^,],%*s", &id, channel) == 2) {
        // DEBUGGING
        printf("id: %d, name: %s\n", id, channel);

        // Concatenate response
        if (id > 1) strcat(response, " ");
        strcat(response, channel);
    }

    // Close file
    fclose(file);

    // Send response to client
    send(client_fd, response, strlen(response), 0);
}