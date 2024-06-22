#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/stat.h>

//=========//
// GLOBALS //
//=========//

// Global variables
#define PORT 8080
#define MAX_BUFFER 1024
int server_fd;

// Client info variables
char username[100];
char password[100];
char channel[100]="";
char room[100]="";

//================//
// SERVER CONNECT //
//================//

// Server connect function
void connect_server() {
    // Connect to server
    struct sockaddr_in address;
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) <= 0) {
        perror("invalid address");
        exit(EXIT_FAILURE);
    }

    if (connect(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("connection failed");
        exit(EXIT_FAILURE);
    }

    // DEBUGGING
    printf("Connected to server\n");
}

//==================//
// COMMAND HANDLING //
//==================//

// Account handler function
int handle_account(const char *buffer) {
    // Check if buffer is empty
    if (buffer == NULL) {
        perror("buffer is empty");
        exit(EXIT_FAILURE);
    }

    // Send request
    if (send(server_fd, buffer, strlen(buffer), 0) < 0) {
        perror("send failed");
        exit(EXIT_FAILURE);
    }

    // Receive response
    char response[MAX_BUFFER];
    memset(response, 0, MAX_BUFFER);
    if (recv(server_fd, response, MAX_BUFFER, 0) < 0) {
        perror("recv failed");
        exit(EXIT_FAILURE);
    }

    // Parse response
    char *type = strtok(response, ",");
    char *message = strtok(NULL, ",");

    // Default message received
    if (strcmp(type, "MSG") == 0){
        printf("%s\n", message);
        return 0;
    }

    // Login status sending
    else if (strcmp(type, "LOGIN") == 0){
        printf("%s\n", message);
        return 1;
    }
}

// Command handler function
int handle_command(const char *buffer) {
    // Check if buffer is empty
    if (buffer == NULL) {
        perror("buffer is empty");
        exit(EXIT_FAILURE);
    }

    // Send request
    if (send(server_fd, buffer, strlen(buffer), 0) < 0) {
        perror("send failed");
        exit(EXIT_FAILURE);
    }

    // Receive response
    char response[MAX_BUFFER*8];
    memset(response, 0, MAX_BUFFER*8);
    if (recv(server_fd, response, MAX_BUFFER*8, 0) < 0) {
        perror("recv failed");
        exit(EXIT_FAILURE);
    }

    // Parse response
    char *type = strtok(response, ",");
    char *message = strtok(NULL, ",");

    // Default message received
    if (strcmp(type, "MSG") == 0){
        printf("%s\n", message);
        return 0;
    }

    // Channel/room variable sending
    else if (strcmp(type, "CHANNEL") == 0){
        // Parse channel name
        char *channel_name = strtok(NULL, ",");

        // Check if parsing is correct
        if (channel_name == NULL) {
            perror("channel name is empty");
            exit(EXIT_FAILURE);
        }

        // Copy channel name
        strcpy(channel, channel_name);
        printf("%s\n", message);
        return 0;
    } else if (strcmp(type, "ROOM") == 0){
        // Parse room name
        char *room_name = strtok(NULL, ",");

        // Check if parsing is correct
        if (room_name == NULL) {
            perror("room name is empty");
            exit(EXIT_FAILURE);
        }

        // Copy room name
        strcpy(room, room_name);
        printf("%s\n", message);
        return 0;
    }

    // Key request
    else if (strcmp(type, "KEY") == 0){
        printf("%s\n", message);
        return 1;
    }

    // Quit message
    else if (strcmp(type, "QUIT") == 0){
        printf("%s\n", message);
        return 2;
    }
}

//================//
// KEY REQUESTING //
//================//

void key_request(char *buffer) {
    // Ask for key
    printf("Enter key: ");

    // Get key from user
    char key[100];
    memset(key, 0, 100);
    fgets(key, 100, stdin);
    key[strcspn(key, "\n")] = '\0';

    // Prepare buffer
    sprintf(buffer, "KEY %s", key);

    // DEBUGGING
    // printf("Sending: %s\n", buffer);

    // Send to server
    handle_command(buffer);
}

//======//
// MAIN //
//======//

int main(int argc, char *argv[]) {
    // Validate arguments:
    if (argc < 5) {
        printf("Usage: ./discorit [REGISTER/LOGIN] <username> -p <password>"
               "\n(not enough arguments)");
        return 1;
  } if (strcmp(argv[1], "REGISTER") != 0 && strcmp(argv[1], "LOGIN") != 0) {
        printf("Usage: ./discorit [REGISTER/LOGIN] <username> -p <password>"
               "\n(invalid command)");
        return 1;
  } if (strcmp(argv[3], "-p") != 0) {
        printf("Usage: ./discorit REGISTER <username> -p <password>"
                "\n(missing -p flag)");
        return 1;
    }

    // Connect to server
    connect_server();

    // Prepare buffer
    char buffer[MAX_BUFFER];
    memset(buffer, 0, MAX_BUFFER);
    strcpy(username, argv[2]);
    strcpy(password, argv[4]);

    // Register user
    if (strcmp(argv[1], "REGISTER") == 0) {
        // Prepare buffer
        sprintf(buffer, "REGISTER %s %s", username, password);        

        // DEBUGGING
        // printf("Sending: %s\n", buffer);

        // Send to server and exit
        handle_account(buffer);
        close(server_fd);
        return 0;
    }

    // Login user
    if (strcmp(argv[1], "LOGIN") == 0) {
        // Prepare buffer
        sprintf(buffer, "LOGIN %s %s", username, password);

        // DEBUGGING
        // printf("Sending: %s\n", buffer);

        // Login check
        if (handle_account(buffer) == 1)
        while(1){
            // Print user prompt
            if (strlen(room) > 0) 
                printf("[%s/%s/%s] ", username, channel, room);
            else if (strlen(channel) > 0) 
                printf("[%s/%s] ", username, channel);
            else 
                printf("[%s] ", username);

            // Get user input
            memset(buffer, 0, MAX_BUFFER);
            fgets(buffer, MAX_BUFFER, stdin);
            buffer[strcspn(buffer, "\n")] = '\0';

            // DEBUGGING
            // printf("Sending: %s\n", buffer);

            // Handle command branching
            int res = handle_command(buffer);

            // Check if user wants to quit
            if (res == 2)
                return 0;

            // Check if user needs to input key
            if (res == 1)
                key_request(buffer);
        }
    }
}
