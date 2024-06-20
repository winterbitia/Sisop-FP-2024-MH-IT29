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

#define PORT 8080
#define MAX_CLIENTS 10
#define MAX_BUFFER 1028
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

// Handlers
void *handle_client(void *arg);
void register_user(char *username, char *password);

//======//
// MAIN //
//======//
int main(){
    // Start daemon
    // daemonize();

    // Start server
    start_server();

    // Accept incoming connections
    while (1){
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0){
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        client_data *client = (client_data *)malloc(sizeof(client_data));
        client->socket_fd = client_fd;
        memset(client->username, 0, MAX_BUFFER);
        memset(client->role, 0, 5);

        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, (void *)client);
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
    printf("Server started on port %d\n", PORT);
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
        // login_user(username, password);
    }

    // Close client connection
    close(client_fd);
    free(client);
    pthread_exit(NULL);
}

//===============//
// REGISTER USER //
//===============//

void register_user(char *username, char *password) {
    char *filename = "users.csv";
    FILE *file = fopen(filename, "a+");

    // Fail if file cannot be opened
    if (file == NULL) {
        printf("Error: Unable to open file\n");
        return;
    }

    // Loop through id and username
    int id = 0; char namecheck[MAX_BUFFER];
    while (fscanf(file, "%d,%[^,],%*s", &id, &namecheck) != EOF) {
        // DEBUGGING
        printf("id: %d, name: %s\n", id, namecheck);

        // Fail if username already exists
        if (strcmp(namecheck, username) == 0) {
            printf("Error: Username already exists\n");
            return;
        }
    }

    // Set role to USER by default
    char role[5] = "USER";
    if (id == 0) strcpy(role, "ROOT");
    
    // Write to file
    fprintf(file, "%d,%s,%s,%s\n", id+1, username, password, role);
    fclose(file);
}