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
#define MAX_BUFFER 1024
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
int  check_channel(char *channel, client_data *client);
void create_channel(char *channel, char *key, client_data *client);
void list_channel(client_data *client);

// Channel Join Handlers
void  join_channel(char *channel, client_data *client);
char* get_key(char *channel, client_data *client);
int   verify_key(client_data *client, char *channel);

// Room Handlers
void create_room(char *room, client_data *client);
void list_room(client_data *client);

// User Handlers
void print_user(client_data *client);
void list_user(client_data *client);


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
            printf("Quit: %s\n", client->username);

            // Send response to client
            memset(response, 0, MAX_BUFFER);
            sprintf(response, "QUIT,Gracefully disconnecting from server...");
            send(client_fd, response, strlen(response), 0);

            // Close client connection
            close(client_fd);
            free(client);
            pthread_exit(NULL);
            return;

 } else if (strcmp(command, "USER") == 0){
            // Call print user function
            print_user(client);

 } else if (strcmp(command, "CREATE") == 0){
            // Parse data from client
            char *type = strtok(NULL, " ");

            // Check if command is valid
            if (type == NULL){
                // DEBUGGING
                printf("Error: Invalid command\n");

                // Send response to client
                memset(response, 0, MAX_BUFFER);
                sprintf(response, "MSG,Error: Invalid command (missing create type)");
                send(client_fd, response, strlen(response), 0);
                continue;
            }

            // DEBUGGING
            printf("command: %s, type: %s\n", command, type);

            // Create channel
            if (strcmp(type, "CHANNEL") == 0){
                // Parse data from client
                char *channel = strtok(NULL, " ");
                char *flag = strtok(NULL, " ");
                char *key = strtok(NULL, " ");

                // Check if command is valid
                if (channel == NULL || key == NULL){
                    // DEBUGGING
                    printf("Error: Invalid command (missing name/key)\n");

                    // Send response to client
                    memset(response, 0, MAX_BUFFER);
                    sprintf(response, "MSG,Error: Invalid command (missing channel name or key)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                } else if (strcmp(flag, "-k") != 0){
                    // DEBUGGING
                    printf("Error: Invalid flag statement\n");

                    // Send response to client
                    memset(response, 0, MAX_BUFFER);
                    sprintf(response, "MSG,Error: Invalid command (missing -k flag)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // DEBUGGING
                printf("channel: %s, key: %s\n", channel, key);

                // Call create channel function
                create_channel(channel, key, client);
            } else if (strcmp(type, "ROOM") == 0){
                // Parse data from client
                char *room = strtok(NULL, " ");

                // Check if command is valid
                if (room == NULL){
                    // DEBUGGING
                    printf("Error: Invalid command (missing room name)\n");

                    // Send response to client
                    memset(response, 0, MAX_BUFFER);
                    sprintf(response, "MSG,Error: Invalid command (missing room name)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // DEBUGGING
                printf("room: %s\n", room);

                // Call create room function
                create_room(room, client);
            } else {
                // DEBUGGING
                printf("Error: Create type not found\n");

                // Send response to client
                memset(response, 0, MAX_BUFFER);
                sprintf(response, "MSG,Error: Create type not found");
                send(client_fd, response, strlen(response), 0);
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
                sprintf(response, "MSG,Error: Invalid command (missing list type)");
                send(client_fd, response, strlen(response), 0);
                continue;
            }

            // DEBUGGING
            printf("command: %s, type: %s\n", command, type);

            // List channels
            if (strcmp(type, "CHANNEL") == 0){
                // Call list channels function
                list_channel(client);
            } else if (strcmp(type, "ROOM") == 0){
                // Call list room function
                list_room(client);
            } else if (strcmp(type, "USER") == 0){
                // Call list user function
                list_user(client);
            } else {
                // DEBUGGING
                printf("Error: List type not found\n");

                // Send response to client
                memset(response, 0, MAX_BUFFER);
                sprintf(response, "MSG,Error: List type not found");
                send(client_fd, response, strlen(response), 0);
            }

 } else if (strcmp(command, "JOIN") == 0){
            // Parse data from client
            char *target = strtok(NULL, " ");

            // Check if command is valid
            if (target == NULL){
                // DEBUGGING
                printf("Error: Invalid command\n");

                // Send response to client
                memset(response, 0, MAX_BUFFER);
                sprintf(response, "MSG,Error: Invalid command (missing target)");
                send(client_fd, response, strlen(response), 0);
                continue;
            }

            // DEBUGGING
            printf("command: %s, type: %s\n", command, target);

            // Join channel if user is not in a channel
            if (strlen(client->channel) == 0){
                // Call join channel function
                join_channel(target, client);
            } else if (strlen(client->room) == 0){
                // insert join room function here
            } else {
                // DEBUGGING
                printf("Error: User is already in a room\n");

                // Send response to client
                memset(response, 0, MAX_BUFFER);
                sprintf(response, "MSG,Error: User is already in a channel");
                send(client_fd, response, strlen(response), 0);
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

                // DEBUGGING
                printf("id: %d, name: %s, role: %s\n", client->id, client->username, client->role);

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
    // Write to auth.csv
    FILE *file_auth = fopen(path_auth, "a");

    // Fail if file cannot be opened
    if (file_auth == NULL) {
        // DEBUGGING
        printf("Error: Unable to open file\n");
        return;
    }

    // Write to file
    if (strcmp(client->role, "ROOT") == 0)
        fprintf(file_auth, "%d,%s,ROOT\n", client->id, client->username);
    else
        fprintf(file_auth, "%d,%s,ADMIN\n", client->id, client->username);
    fclose(file_auth);
}

//========================//
// CHECK CHANNEL EXISTING //
//========================//

int check_channel(char *channel, client_data *client){
    int client_fd = client->socket_fd;

    // Prepare channel path
    char path_channel[MAX_BUFFER];
    sprintf(path_channel, "%s/%s", cwd, channel);

    // Prepare response
    char response[MAX_BUFFER];

    // Open channel list
    FILE *file = fopen(channels_csv, "r");
    if (file == NULL) {
        // DEBUGGING
        printf("Error: Unable to open file\n");

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return -1;
    }

    // Loop through channel list
    int id; char channelcheck[100];
    while (fscanf(file, "%d,%[^,],%*s", &id, channelcheck) == 2) {
        if (strcmp(channel, channelcheck) == 0) {
            // Return id if channel exists
            return -2;
        }
    }

    // Close file
    fclose(file);

    // Return -1 if channel does not exist
    return id;
}

//================//
// CREATE CHANNEL //
//================//

void create_channel(char *channel, char *key, client_data *client) {
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_BUFFER];

    // Check if channel id exists
    int id = check_channel(channel, client);
    if (id == -2) {
        // DEBUGGING
        printf("Error: Channel already exists\n");

        // Send response to client
        sprintf(response, "MSG,Error: Channel %s already exists", channel);
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Open file
    FILE *file = fopen(channels_csv, "a+");

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

void list_channel(client_data *client) {
    int client_fd = client->socket_fd;

    // Open file
    FILE *file = fopen(channels_csv, "r");

    // Prepare response
    char response[MAX_BUFFER];
    sprintf(response, "MSG,");
    int channels_found = 0;

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

        // Increment channels found
        channels_found++;

        // Concatenate response
        if (id > 1) strcat(response, " ");
        strcat(response, channel);
    }

    // Close file
    fclose(file);

    // Send response to client
    if (channels_found == 0) {
        sprintf(response, "MSG,No channels found");
        send(client_fd, response, strlen(response), 0);
    } else {
        send(client_fd, response, strlen(response), 0);
    }

    return;
}

//==============//
// JOIN CHANNEL //
//==============//

void join_channel(char *channel, client_data *client) {
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_BUFFER];
    sprintf(response, "MSG,");

    // Check if channel exists
    if (check_channel(channel, client) != -2) {
        // DEBUGGING
        printf("Error: Channel does not exist\n");

        // Send response to client
        sprintf(response, "MSG,Error: Channel does not exist");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Open auth of current channel
    char path_auth[MAX_BUFFER];
    sprintf(path_auth, "%s/%s/admin/auth.csv", cwd, channel);
    FILE *file = fopen(path_auth, "a+");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("Error: Unable to open auth file\n");

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open auth file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop through id and role
    int id; char role[6];
    while (fscanf(file, "%d,%*[^,],%s", &id, role) == 2) {
        // DEBUGGING
        printf("id: %d, role: %s\n", id, role);

        // If there is an id match or user is root
        if (client->id == id) {
            // Check if user is banned
            if (strcmp(role, "BANNED") == 0) {
                // DEBUGGING
                printf("Error: User is banned\n");

                // Close file
                fclose(file);

                // Send response to client
                sprintf(response, "MSG,Error: User is banned from channel");
                send(client_fd, response, strlen(response), 0);
                return;
            }

            // DEBUGGING
            printf("Success: %s joined channel\n", client->username);

            // Close file
            fclose(file);

            // Update client channel
            strcpy(client->channel, channel);

            // Send response to client
            if (strcmp(client->role, "ROOT") == 0)
                sprintf(response, "CHANNEL,Joined channel %s as ROOT,%s", channel, channel);
            else
                sprintf(response, "CHANNEL,Joined channel %s,%s", channel, channel);
            send(client_fd, response, strlen(response), 0);
            return;
        }
    }

    // Add user to channel if user is root and not listed
    if (strcmp(client->role, "ROOT") == 0) {
        // Open auth of current channel and add user
        fprintf(file, "%d,%s,ROOT\n", client->id, client->username);

        // DEBUGGING
        printf("Success: Root user joined channel\n");

        // Close file
        fclose(file);

        // Update client channel
        strcpy(client->channel, channel);

        // Send response to client
        sprintf(response, "CHANNEL,Joined channel %s as ROOT,%s", channel, channel);
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Verify key if user is not listed
    if (verify_key(client, channel) == 1) {
        // Open auth of current channel and add user
        fprintf(file, "%d,%s,USER\n", client->id, client->username);

        // DEBUGGING
        printf("Success: %s joined channel\n", client->username);

        // Close file
        fclose(file);

        // Update client channel
        strcpy(client->channel, channel);

        // Send response to client
        sprintf(response, "CHANNEL,Joined channel %s,%s", channel, channel);
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Close file
    fclose(file);

    // Send response to client
    sprintf(response, "MSG,Error: Key verification failure");
    send(client_fd, response, strlen(response), 0);
    return;
}

//=================//
// GET CHANNEL KEY //
//=================//

char* get_key(char *channel, client_data *client) {
    int client_fd = client->socket_fd;

    // Open channel list
    FILE *file = fopen(channels_csv, "r");

    // Prepare response
    char response[MAX_BUFFER];

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("Error: Unable to open file\n");

        // Return NULL if file cannot be opened
        return NULL;
    }

    // Loop through id, channel, and key
    int id; char channelcheck[MAX_BUFFER];
    char *keycheck = (char *)malloc(sizeof(char) * MAX_BUFFER);
    while (fscanf(file, "%d,%[^,],%s", &id, channelcheck, keycheck) == 3) {
        // DEBUGGING
        printf("id: %d, name: %s, key: %s\n", id, channelcheck, keycheck);

        // Return key if channel exists
        if (strcmp(channel, channelcheck) == 0) {
            // DEBUGGING
            printf("Success: Key found\n");

            // Close file and return
            fclose(file);
            return keycheck;
        }
    }

    // Close file
    fclose(file);

    // Return NULL if channel does not exist
    return NULL;
}

//================//
// VERIFY CHANNEL //
//================//

int verify_key(client_data *client, char *channel) {
    int client_fd = client->socket_fd;

    // Get key from channel
    char *key = get_key(channel, client);

    // Check if key is not null
    if (key == NULL) {
        // DEBUGGING
        printf("Error: Key from channel is NULL somehow\n");

        // Break if key is null
        return -1;
    }

    // Request key from client
    char request[MAX_BUFFER];
    sprintf(request, "KEY,%s", channel);
    send(client_fd, request, strlen(request), 0);

    // Prepare for key verification and response
    char buffer[MAX_BUFFER];
    memset(buffer, 0, MAX_BUFFER);

    // Receive data from client
    if (recv(client_fd, buffer, MAX_BUFFER, 0) < 0){
        perror("recv failed");
        exit(EXIT_FAILURE);
    }

    // Prepare response
    char response[MAX_BUFFER];

    // Parse data from client
    char *command = strtok(buffer, " ");
    char *keycheck = strtok(NULL, " ");

    // Check if key is not null
    if (keycheck == NULL) {
        // DEBUGGING
        printf("Error: Key from client is NULL\n");

        // Return -1 if key is null
        return -1;
    }

    // DEBUGGING
    printf("command: %s, key: %s\n", command, keycheck);

    // Hash key
    char hash[MAX_BUFFER];
    strcpy(hash,crypt(keycheck, HASHCODE));

    // Check if key matches
    if (strcmp(command, "KEY") == 0 && strcmp(key, hash) == 0) {
        // DEBUGGING
        printf("Success: Key matches\n");

        // Success if key matches
        return 1;
    }

    // DEBUGGING
    printf("Error: Key does not match\n");

    // Fail if key does not match
    return -1;    
}

//===========================================================================================//
//----------------------------------------- ROOMS -------------------------------------------//
//===========================================================================================//

//=============//
// CREATE ROOM //
//=============//

void create_room(char *room, client_data *client) {
    int client_fd = client->socket_fd;

    // Open auth of current channel
    char path_auth[MAX_BUFFER];
    sprintf(path_auth, "%s/%s/admin/auth.csv", cwd, client->channel);
    FILE *file = fopen(path_auth, "r");

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

    // Loop through id and role
    int id; char role[6];
    while (fscanf(file, "%d,%*[^,],%s", &id, role) == 2) {
        // DEBUGGING
        printf("id: %d, role: %s\n", id, role);

        // Fail if user is not admin/root
        if (client->id == id
            && strcmp(role, "ADMIN") != 0
            && strcmp(role, "ROOT") != 0){
            // DEBUGGING
            printf("Error: User has no privileges to make a room\n");

            // Close file
            fclose(file);

            // Send response to client
            sprintf(response, "MSG,Error: User has no privileges");
            send(client_fd, response, strlen(response), 0);
            return;
        }
    }

    // Close file
    fclose(file);

    // Check if room name is valid
    if (strcmp(room, "admin") == 0) {
        // DEBUGGING
        printf("Error: Invalid room name\n");

        // Send response to client
        sprintf(response, "MSG,Error: Invalid room name");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Create room directory 
    char path_room[MAX_BUFFER];
    sprintf(path_room, "%s/%s/%s", cwd, client->channel, room);
    make_directory(path_room);

    // Initialize chat.csv
    char path_chat[MAX_BUFFER];
    sprintf(path_chat, "%s/%s/%s/chat.csv", cwd, client->channel, room);
    FILE *file_chat = fopen(path_chat, "w");
    if (file_chat == NULL) {
        // DEBUGGING
        printf("Error: Unable to create chat file\n");

        // Send response to client
        sprintf(response, "MSG,Error: Unable to create chat file");
        send(client_fd, response, strlen(response), 0);
        return;
    }
    
    // Close file
    fclose(file_chat);

    // Send response to client
    sprintf(response, "MSG,Success: Room %s created", room);
    send(client_fd, response, strlen(response), 0);
    return;
}

void list_room(client_data *client) {
    int client_fd = client->socket_fd;

    // Check if user is in a channel
    if (strlen(client->channel) == 0) {
        // DEBUGGING
        printf("Error: User is not in a channel\n");

        // Prepare response
        char response[MAX_BUFFER];
        sprintf(response, "MSG,Error: User is not in a channel");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Prepare channel path
    char path_channel[MAX_BUFFER];
    sprintf(path_channel, "%s/%s", cwd, client->channel);

    // Prepare response
    char response[MAX_BUFFER];
    sprintf(response, "MSG,");
    int rooms_found = 0;

    // Open channel directory
    DIR *dir = opendir(path_channel);

    // Fail if directory cannot be opened
    if (dir == NULL) {
        // DEBUGGING
        printf("Error: Unable to open directory\n");

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open directory");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        // DEBUGGING
        printf("name: %s, type: %d\n", entry->d_name, entry->d_type);

        // Skip admin directory
        if (strcmp(entry->d_name, "admin") == 0) continue;

        // Prepare entry path
        char entry_path[MAX_BUFFER];
        sprintf(entry_path, "%s/%s", path_channel, entry->d_name);

        // Check if entry is a directory to be listed
        struct stat statbuf;
        if (stat(entry_path, &statbuf) == -1) continue;
        if (S_ISDIR(statbuf.st_mode)) {
            // Increment rooms found
            rooms_found++;

            // Concatenate response
            if (rooms_found > 1) strcat(response, " ");
            strcat(response, entry->d_name);
        }   
    }

    // Close directory
    closedir(dir);

    // Send response to client
    if (rooms_found == 0) {
        sprintf(response, "MSG,No rooms found");
        send(client_fd, response, strlen(response), 0);
    } else {
        send(client_fd, response, strlen(response), 0);
    }

    return;
}

//===========================================================================================//
//------------------------------------------- USERS -----------------------------------------//
//===========================================================================================//

//=================//
// PRINT USER DATA //
//=================//

void print_user(client_data *client) {
    // DEBUGGING
    printf("id: %d, name: %s, role: %s, channel: %s, room: %s\n", client->id, client->username, client->role, client->channel, client->room);

    // Prepare response
    char response[MAX_BUFFER];

    // Send response to client
    sprintf(response, "MSG,"
        "User ID\t: %d\nName\t: %s\nRole\t: %s\nChannel\t: %s\nRoom\t: %s",
        client->id, client->username, client->role, client->channel, client->room);  
    send(client->socket_fd, response, strlen(response), 0);
    return;
}


//=======================//
// LIST USERS IN CHANNEL //
//=======================//

void list_user(client_data *client) {
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_BUFFER];
    sprintf(response, "MSG,");

    // Check if user is in a channel
    if (strlen(client->channel) == 0) {
        // DEBUGGING
        printf("Error: User is not in a channel\n");

        // Send response to client
        sprintf(response, "MSG,Error: User is not in a channel");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Prepare channel and auth path
    char path_channel[MAX_BUFFER], path_auth[MAX_BUFFER];
    sprintf(path_channel, "%s/%s", cwd, client->channel);
    sprintf(path_auth, "%s/admin/auth.csv", path_channel);
    FILE *file = fopen(path_auth, "r");

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
    int id; char username[MAX_BUFFER];
    while (fscanf(file, "%d,%[^,],%*s", &id, username) == 2) {
        // DEBUGGING
        printf("id: %d, name: %s\n", id, username);

        // Concatenate response
        if (strlen(response) > 4) strcat(response, " ");
        strcat(response, username);
    }

    // Close file
    fclose(file);

    // Send response to client
    send(client_fd, response, strlen(response), 0);
    return;
}