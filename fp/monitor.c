#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <pthread.h>

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

// Pre-declaration
void *input_handler(void *arg);
void connect_server();
void clear_terminal();
void parse_command(char *buffer);
int handle_account(const char *buffer);
int handle_command(const char *buffer);

//======//
// MAIN //
//======//

// Main function
int main(int argc, char *argv[]) {
    // Validate arguments:
    if (argc < 5) {
        printf("Usage: ./discorit LOGIN <username> -p <password>"
               "\n(not enough arguments)");
        return 1;
  } if (strcmp(argv[1], "LOGIN") != 0) {
        printf("Usage: ./discorit LOGIN <username> -p <password>"
               "\n(invalid command)");
        return 1;
  } if (strcmp(argv[3], "-p") != 0) {
        printf("Usage: ./discorit LOGIN <username> -p <password>"
                "\n(missing -p flag)");
        return 1;
    }

    // Connect to server
    connect_server();

    // Prepare buffer
    char buffer[MAX_BUFFER];
    memset(buffer, 0, sizeof(buffer));
    strcpy(username, argv[2]);
    strcpy(password, argv[4]);
    sprintf(buffer, "LOGIN %s %s", username, password);

    // DEBUGGING
    // printf("Sending: %s\n", buffer);

    // Login check
    if (handle_account(buffer) != 1) {
        close(server_fd);
        return 0;
    }

    // Print user prompt
    while(1){
        // Get chat if client is in a room
        while (strlen(room) > 0) {
            clear_terminal();
            printf("==========================================\n");
            handle_command("SEE CHAT");
            printf("==========================================\n"
                   "[!] Monitor refreshes every 3 seconds\n"
                   "[!] Type EXIT to leave the room\n"
                   "==========================================\n");  

            // Handle user input
            pthread_t tid;
            pthread_create(&tid, NULL, input_handler, NULL);

            // Refresh every 3 seconds
            sleep(3); pthread_cancel(tid);
        }

        // Handle user input
        pthread_t tid;
        pthread_create(&tid, NULL, input_handler, NULL);
        pthread_join(tid, NULL);
    }
}

//=========//
// METHODS //
//=========//

// Input handler function
void *input_handler(void *arg) {
    // Print user prompt
    if (strlen(room) > 0) 
        printf("[%s/%s/%s] ", username, channel, room);
    else if (strlen(channel) > 0) 
        printf("[%s/%s] ", username, channel);
    else 
        printf("[%s] ", username);

    // Get user input
    char buffer[MAX_BUFFER];
    memset(buffer, 0, sizeof(buffer));
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = 0;

    // DEBUGGING
    // printf("Sending: %s\n", buffer);

    // Parse command
    parse_command(buffer);
}

// Connect to server
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

// Clear terminal
void clear_terminal() {
    printf("\033[H\033[J");
}

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

// Parse command
void parse_command(char *buffer) {
    // Parse command
    char *command1 = strtok(buffer, " ");

    // Check if command1 is NULL
    if (command1 == NULL) {
        printf("Error: command is invalid (empty)\n");
        return;
    }

    // Check command1 not EXIT to skip filtering
    if (strcmp(command1, "EXIT") != 0) {
        // Parse command again
        char *cname = strtok(NULL, " ");
        char *command2 = strtok(NULL, " ");
        char *rname = strtok(NULL, " ");

        // DEBUGGING
        // printf("Command1: %s\n", command1);
        // printf("Channel: %s\n", cname);
        // printf("Command2: %s\n", command2);
        // printf("Room: %s\n", rname);
        
        // Check if command is valid
        if (command2 == NULL || cname == NULL || rname == NULL) {
            printf("Error: command is invalid (missing arguments)\n");
            return;
        }

        // Check if command is correct
        if (strcmp(command1, "-channel") != 0
         || strcmp(command2, "-room") != 0) {
            printf("Error: command is invalid (wrong arguments)\n");
            return;
        }

        // Prepare two requests
        char buffer1[MAX_BUFFER];
        char buffer2[MAX_BUFFER];
        memset(buffer1, 0, sizeof(buffer1));
        memset(buffer2, 0, sizeof(buffer2));
        sprintf(buffer1, "JOIN %s", cname);
        sprintf(buffer2, "JOIN %s", rname);

        // Handle two requests
        if (handle_command(buffer1) == 0)
            handle_command(buffer2);
        return;
    }

    // Check if user is in a channel or room
    if (strlen(room) > 0 || strlen(channel) > 0) {
        while (strlen(room) > 0 || strlen(channel) > 0) 
            handle_command("EXIT");
        return; 
    }

    // Handle EXIT command
    if (handle_command(buffer) == 2) {
        close(server_fd);
        exit(EXIT_SUCCESS);
    }
    return;
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

    // Channel/room exiting
    else if (strcmp(type, "EXIT") == 0){
        // Parse exit type
        char *exit_type = strtok(NULL, ",");

        // Check if parsing is correct
        if (exit_type == NULL) {
            perror("exit type is empty");
            exit(EXIT_FAILURE);
        }

        // Check if exiting channel
        if (strcmp(exit_type, "CHANNEL") == 0) {
            memset(channel, 0, 100);
            // also reset room in case user is in a room
            memset(room, 0, 100);

            // DEBUGGING
            // printf("%s\n", message);
            return 0;
        }

        // Check if exiting room
        else if (strcmp(exit_type, "ROOM") == 0) {
            memset(room, 0, 100);

            // DEBUGGING
            // printf("%s\n", message);
            return 0;
        }
    }

    // When asked for key
    else if (strcmp(type, "KEY") == 0){
        // Send wrong key
        handle_command("X X");
        return 0;
    }

    // Quit message
    else if (strcmp(type, "QUIT") == 0){
        printf("%s\n", message);
        return 2;
    }
}