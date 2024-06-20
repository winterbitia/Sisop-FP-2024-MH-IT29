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

// Misc variables
char cwd[MAX_BUFFER];

// Socket variables
int server_fd, client_fd;
struct sockaddr_in address;
socklen_t addrlen = sizeof(address);

// Save client information
typedef struct {
    // Client socket and address
    int socket_fd;
    struct sockaddr_in address;
    // Client details
    char username[MAX_BUFFER];
    char role[5];
} client_data;

//=================//
// PRE-DECLARATION //
//=================//

// Main server
void daemonize();
void start_server();
void *handle_client(void *arg);

// Account Handlers
void register_user(char *username, char *password);
void login_user(char *username, char *password);

//===========================================================================================//
//----------------------------------------- FUNCTIONS ---------------------------------------//
//===========================================================================================//

//======//
// MAIN //
//======//
int main(int argc, char *argv[]){
    // Get current directory
    getcwd(cwd, sizeof(cwd));    

    // Start server
    start_server();

    // Foreground handling
    if (argc > 1 && strcmp(argv[1], "-f") == 0){
        printf("Server: running in foreground!\n");
    } else daemonize();

    // Accept incoming connections
    while (1){
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0){
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        // Prepare client data
        client_data *client = (client_data *)malloc(sizeof(client_data));
        client->socket_fd = client_fd;
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
    pid_t pid, sid;
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);
    umask(0);
    sid = setsid();
    if (sid < 0) exit(EXIT_FAILURE);
    if ((chdir("/")) < 0) exit(EXIT_FAILURE);
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
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
    char *command = strtok(buffer, ",");
    char *username = strtok(NULL, ",");
    char *password = strtok(NULL, ",");

    // DEBUGGING
    printf("command: %s, username: %s, password: %s\n", command, username, password);

    // Register user
    if (strcmp(command, "REGISTER") == 0){
        register_user(username, password);
    }

    // Login user
    else if (strcmp(command, "LOGIN") == 0){
        login_user(username, password);
    }

    // Close client connection
    close(client_fd);
    free(client);
    pthread_exit(NULL);
}

//===========================================================================================//
//---------------------------------------- ACCOUNT HANDLERS ---------------------------------//
//===========================================================================================//

//===============//
// REGISTER USER //
//===============//

void register_user(char *username, char *password) {
    // Open file
    char filename[MAX_BUFFER];
    strcpy(filename, cwd);
    strcat(filename, "/users.csv");
    FILE *file = fopen(filename, "a+");

    // Fail if file cannot be opened
    if (file == NULL) {
        printf("Error: Unable to open file\n");
        return;
    }

    // Prepare response
    char response[MAX_BUFFER];

    // Loop through id and username
    int id = 0; char namecheck[MAX_BUFFER];
    while (fscanf(file, "%d,%[^,],%*s", &id, &namecheck) != EOF) {
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
            sprintf(response, "Error: Username %s already exists", username);
            send(client_fd, response, strlen(response), 0);
            return;
        }
    }

    // Hash password
    char hash[MAX_BUFFER];
    strcpy(hash,crypt(password, HASHCODE));

    // Set role to USER by default
    char role[6] = "USER";
    if (id == 0) strcpy(role, "ROOT");

    // DEBUGGING
    printf("id: %d, name: %s, pass: %s, role: %s\n", id+1, username, hash, role);
    
    // Write to file
    fprintf(file, "%d,%s,%s,%s\n", id+1, username, hash, role);
    fclose(file);

    // Send response to client
    sprintf(response, "Success: User %s registered", username);
    send(client_fd, response, strlen(response), 0);
}

//============//
// LOGIN UsER //
//============//

void login_user(char *username, char *password) {
    // Hash password from input
    char hash[MAX_BUFFER];
    strcpy(hash,crypt(password, HASHCODE));
    
    // Open file
    char filename[MAX_BUFFER];
    strcpy(filename, cwd);
    strcat(filename, "/users.csv");
    FILE *file = fopen(filename, "r");

    // Fail if file cannot be opened
    if (file == NULL) {
        printf("Error: Unable to open file\n");
        return;
    }

    // Prepare response
    char response[MAX_BUFFER];

    // Loop through id, username, and password
    int id = 0; char namecheck[MAX_BUFFER], passcheck[MAX_BUFFER], role[6];
    while (fscanf(file, "%d,%[^,],%[^,],%s", &id, &namecheck, &passcheck, &role) != EOF) {
        // DEBUGGING
        printf("id: %d, name: %s, pass: %s, role: %s\n", id, namecheck, passcheck, role);
        // sleep(1);

        // Fail if username and password do not match
        if (strcmp(namecheck, username) == 0) {
            if (strcmp(passcheck, hash) == 0) {
                // DEBUGGING
                printf("Success: Username and password match\n");

                // Close file
                fclose(file);

                // Send response to client
                sprintf(response, "Success: User %s logged in", username);
                send(client_fd, response, strlen(response), 0);
                return;
            }

            // DEBUGGING
            printf("Error: Password does not match\n");

            // Fail if password does not match
            sprintf(response, "Error: Password does not match");
            send(client_fd, response, strlen(response), 0);
            fclose(file);
            return;
        }
    }

    // DEBUGGING
    printf("Error: Username does not exist\n");

    // Send response to client
    sprintf(response, "Error: Username does not exist");
    send(client_fd, response, strlen(response), 0);
    fclose(file);
}