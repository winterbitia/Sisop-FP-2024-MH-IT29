#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <crypt.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/stat.h>

// Global variables
#define PORT 8080
#define MAX_BUFFER 1028
int server_fd;

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

void handle_server(const char *buffer, char *username) {
    if (buffer == NULL) {
        perror("buffer is empty");
        exit(EXIT_FAILURE);
    }

    if (send(server_fd, buffer, strlen(buffer), 0) < 0) {
        perror("send failed");
        exit(EXIT_FAILURE);
    }

    char response[MAX_BUFFER];
    memset(response, 0, MAX_BUFFER);
    if (recv(server_fd, response, MAX_BUFFER, 0) < 0) {
        perror("recv failed");
        exit(EXIT_FAILURE);
    }

    printf("%s\n", response);
}

//======//
// MAIN //
//======//
int main(int argc, char *argv[]) {
    // Check if arguments are valid
    if (argc < 5) {
        printf("Usage: ./discorit [REGISTER/LOGIN] <username> -p <password>", argv[0]);
        return 1;
    }

    // Exit if command is invalid
    if (strcmp(argv[1], "REGISTER") != 0 && strcmp(argv[1], "LOGIN") != 0) {
        printf("Usage: ./discorit [REGISTER/LOGIN] <username> -p <password>");
        return 1;
    }

    // Connect to server
    connect_server();

    // Prepare buffer
    char buffer[MAX_BUFFER];
    memset(buffer, 0, MAX_BUFFER);
    char username[MAX_BUFFER];
    strcpy(username, argv[2]);
    char password[MAX_BUFFER];
    strcpy(password, argv[4]);

    // Register user
    if (strcmp(argv[1], "REGISTER") == 0) {
        if (strcmp(argv[3], "-p") != 0) {
            printf("Usage: ./discorit REGISTER <username> -p <password>");
            return 1;
        }
        
        sprintf(buffer, "REGISTER,%s,%s", username, password);        

        // DEBUGGING
        printf("Sending: %s\n", buffer);
    }

    // Send to server
    handle_server(buffer, username);

    // Close and return success
    close(server_fd);
    return 0;
}