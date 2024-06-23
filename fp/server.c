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
#include <sys/wait.h>
#include <fcntl.h>
#include <crypt.h>
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
#define MAX_CHAT 8192
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

// Channel Join Handlers
void  join_channel(char *channel, client_data *client);
char* get_key(char *channel, client_data *client);
int   verify_key(char *channel, client_data *client);

// Channel Handlers
void admin_init_channel(char *path_auth, client_data *client);
int  check_channel(char *channel, client_data *client);
void create_channel(char *channel, char *key, client_data *client);
void list_channel(client_data *client);
void edit_channel(char *changed, char *new, client_data *client);
void delete_channel(char *channel, client_data *client);

// Room Handlers
void create_room(char *room, client_data *client);
void list_room(client_data *client);
void join_room(char *room, client_data *client);
void edit_room(char *changed, char *new, client_data *client);
void delete_room(char *room, client_data *client);
void delete_all_rooms(client_data *client);

// User Handlers
void see_user(client_data *client);
void list_user(client_data *client);
void exit_user(client_data *client);

// User Management Handlers
int  check_user(client_data *client);
int  check_ban(client_data *client);
int  check_channel_perms(char *target, client_data *client);
void edit_username_auth(char *username, char *newusername, client_data *client);
void edit_username(char *username, char *newusername, client_data *client);
void edit_password(char *username, char *newpassword, client_data *client);
void del_username_auth(char *username, client_data *client);
void remove_user(char *username, client_data *client);
void ban_user(char *username, client_data *client);
void unban_user(char *username, client_data *client);
void kick_user(char *username, client_data *client);

// Chat Handlers
void send_chat(char *message, client_data *client);
void see_chat(client_data *client);
void edit_chat(int target, char *message, client_data *client);
void del_chat(int target, client_data *client);

// Misc Handlers
void make_directory(char *path);
void rename_directory(char *path, char *newpath);
void remove_directory(char *path);
char* get_timestamp();
void write_log(char *channel, char *message);


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
        printf("[SERVER] running in foreground!\n");
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
    printf("[SERVER] Started on port %d\n", PORT);
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
        printf("[%s][LOGIN]\n", username);

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
        printf("[%s] Waiting for command...\n", client->username);

        // Clear buffer
        memset(buffer, 0, MAX_BUFFER);

        // Receive data from client
        if (recv(client_fd, buffer, MAX_BUFFER, 0) < 0){
            perror("recv failed");
            exit(EXIT_FAILURE);
        }

        // Check if user still exists
        if (!check_user(client)){
            // DEBUGGING
            printf("[%s] User does not exist\n", client->username);

            // Send response to client
            memset(response, 0, MAX_BUFFER);
            sprintf(response, "QUIT,Error: User does not exist");
            send(client_fd, response, strlen(response), 0);

            // Close client connection
            close(client_fd);
            free(client);
            pthread_exit(NULL);
            return;
        }

        // Check if user is banned
        int ban = check_ban(client);
        if (ban == 1){
            // DEBUGGING
            printf("[%s] User is banned\n", client->username);

            // Update client channel and room
            memset(client->channel, 0, 100);
            memset(client->room, 0, 100);

            // Send response to client
            memset(response, 0, MAX_BUFFER);
            sprintf(response, "EXIT,Error: User is banned,CHANNEL");
            send(client_fd, response, strlen(response), 0);

            // Accept next command
            continue;
        } else if (ban < 0){
            // DEBUGGING
            printf("[%s] Error: Unable to check ban\n", client->username);

            // Update client channel and room
            memset(client->channel, 0, 100);
            memset(client->room, 0, 100);

            // Send response to client
            memset(response, 0, MAX_BUFFER);
            sprintf(response, "EXIT,Error: Unable to check ban,CHANNEL");
            send(client_fd, response, strlen(response), 0);

            // Accept next command
            continue;
        }

        // Prepare parse data from client
        char *command = strtok(buffer, " ");

        // Start command handling
        if (strcmp(command, "EXIT") == 0){
            // DEBUGGING
            printf("[%s][EXIT]\n", client->username);

            // Call exit user function
            exit_user(client);

 } else if (strcmp(command, "SEE") == 0){
            // Parse data from client
            char *type = strtok(NULL, " ");

            // Check if command is valid
            if (type == NULL){
                // DEBUGGING
                printf("[%s] Error: Invalid see (missing type)\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER);
                sprintf(response, "MSG,Error: Invalid command (missing see type)");
                send(client_fd, response, strlen(response), 0);
                continue;
            }

            // DEBUGGING
            printf("[%s] Command: %s, type: %s\n", client->username, command, type);

            // See types
            if (strcmp(type, "USER") == 0){
                // Call print user function
                see_user(client);
            } else if (strcmp(type, "CHAT") == 0){
                // Call print chat function
                see_chat(client);
            } else {
                // DEBUGGING
                printf("[%s] Error: See type not found\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER);
                sprintf(response, "MSG,Error: See type not found");
                send(client_fd, response, strlen(response), 0);
            }

 } else if (strcmp(command, "CREATE") == 0){
            // Parse data from client
            char *type = strtok(NULL, " ");

            // Check if command is valid
            if (type == NULL){
                // DEBUGGING
                printf("[%s] Error: Invalid create (missing type)\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER);
                sprintf(response, "MSG,Error: Invalid command (missing create type)");
                send(client_fd, response, strlen(response), 0);
                continue;
            }

            // DEBUGGING
            printf("[%s] Command: %s, type: %s\n", client->username, command, type);

            // Create types
            if (strcmp(type, "CHANNEL") == 0){
                // Parse data from client
                char *channel = strtok(NULL, " ");
                char *flag = strtok(NULL, " ");
                char *key = strtok(NULL, " ");

                // Check if command is valid
                if (channel == NULL || key == NULL){
                    // DEBUGGING
                    printf("[%s] Error: Invalid command (missing name/key)\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER);
                    sprintf(response, "MSG,Error: Invalid command (missing channel name/key)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                } else if (strcmp(flag, "-k") != 0){
                    // DEBUGGING
                    printf("[%s] Error: Invalid flag statement\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER);
                    sprintf(response, "MSG,Error: Invalid command (missing -k flag)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // DEBUGGING
                printf("[%s] Channel: %s, Key: %s\n", client->username, channel, key);

                // Call create channel function
                create_channel(channel, key, client);
            } else if (strcmp(type, "ROOM") == 0){
                // Parse data from client
                char *room = strtok(NULL, " ");

                // Check if command is valid
                if (room == NULL){
                    // DEBUGGING
                    printf("[%s] Error: Invalid command (missing room name)\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER);
                    sprintf(response, "MSG,Error: Invalid command (missing room name)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // DEBUGGING
                printf("[%s] Room: %s\n", client->username, room);

                // Call create room function
                create_room(room, client);
            } else {
                // DEBUGGING
                printf("[%s] Error: Create type not found\n", client->username);

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
                printf("[%s] Error: Invalid listing (missing type)\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER);
                sprintf(response, "MSG,Error: Invalid command (missing list type)");
                send(client_fd, response, strlen(response), 0);
                continue;
            }

            // DEBUGGING
            printf("[%s] Command: %s, type: %s\n", client->username, command, type);

            // Listing types
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
                printf("[%s] Error: List type not found\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER);
                sprintf(response, "MSG,Error: List type not found");
                send(client_fd, response, strlen(response), 0);
            }

 } else if (strcmp(command, "EDIT") == 0){
            // Parse data from client
            char *type = strtok(NULL, " ");

            // Check if command is valid
            if (type == NULL){
                // DEBUGGING
                printf("[%s] Error: Invalid edit (missing type)\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER);
                sprintf(response, "MSG,Error: Invalid command (missing edit type)");
                send(client_fd, response, strlen(response), 0);
                continue;
            }

            // DEBUGGING
            printf("[%s] Command: %s, type: %s\n", client->username, command, type);

            // Edit types
            if (strcmp(type, "WHERE") == 0){
                char *target = strtok(NULL, " ");
                char *flag = strtok(NULL, " ");
                char *new = strtok(NULL, " ");

                // Check if command is valid
                if (target == NULL || new == NULL){
                    // DEBUGGING
                    printf("[%s] Error: Invalid command (missing target/new)\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER);
                    sprintf(response, "MSG,Error: Invalid command (missing target or new)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // DEBUGGING
                printf("[%s] Flag:%s Target: %s, New: %s\n", client->username, flag, target, new);

                // Branch flag types
                if (strcmp(flag, "-u") == 0){
                    // Call edit username function
                    edit_username(target, new, client);
                } else if (strcmp(flag, "-p") == 0){
                    // Call edit password function
                    edit_password(target, new, client);
                } else {
                    // DEBUGGING
                    printf("[%s] Error: Flag type not found\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER);
                    sprintf(response, "MSG,Error: Flag type not found");
                    send(client_fd, response, strlen(response), 0);
                }
            } else if (strcmp(type, "CHAT") == 0) {
                // Parse data from client
                char *target = strtok(NULL, " ");
                char *message = strtok(NULL, "\n");

                // Check if command is valid
                if (target == NULL || message == NULL){
                    // DEBUGGING
                    printf("[%s] Error: Invalid command (missing target/message)\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER);
                    sprintf(response, "MSG,Error: Invalid command (missing target or message)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // Check if message is correctly encased in quotes
                if (message[0] != '"' || message[strlen(message)-1] != '"'){
                    // DEBUGGING
                    printf("[%s] Error: Invalid message (missing quotes)\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER);
                    sprintf(response, "MSG,Error: Invalid command (missing quotes)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // DEBUGGING
                printf("[%s] Target: %s, Message: %s\n", client->username, target, message);

                // call edit chat function
                edit_chat(atoi(target), message, client);
            } else if (strcmp(type, "PROFILE") == 0){
                // Parse data from client
                char *target = strtok(NULL, " ");
                char *flag = strtok(NULL, " ");
                char *new = strtok(NULL, " ");

                // Check if command is valid
                if (target == NULL || new == NULL){
                    // DEBUGGING
                    printf("[%s] Error: Invalid command (missing target/new)\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER);
                    sprintf(response, "MSG,Error: Invalid command (missing target or new)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // DEBUGGING
                printf("[%s] Flag:%s Target: %s, New: %s\n", client->username, flag, target, new);

                // Branch flag types
                if (strcmp(flag, "-u") == 0){
                    // Call edit username function
                    edit_username(client->username, new, client);
                } else if (strcmp(flag, "-p") == 0){
                    // Call edit password function
                    edit_password(client->username, new, client);
                } else {
                    // DEBUGGING
                    printf("[%s] Error: Flag type not found\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER);
                    sprintf(response, "MSG,Error: Flag type not found");
                    send(client_fd, response, strlen(response), 0);
                }

            } else if (strcmp(type, "CHANNEL") == 0){
                // Parse data from client
                char *changed = strtok(NULL, " ");
                char *flag = strtok(NULL, " ");
                char *new = strtok(NULL, " ");

                // Check if command is valid
                if (changed == NULL || new == NULL){
                    // DEBUGGING
                    printf("[%s] Error: Invalid command (missing changed/new)\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER);
                    sprintf(response, "MSG,Error: Invalid command (missing changed or new)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // Check if flag is valid
                if (strcmp(flag, "TO") != 0){
                    // DEBUGGING
                    printf("[%s] Error: Invalid command (missing flag)\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER);
                    sprintf(response, "MSG,Error: Invalid command (missing flag)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // DEBUGGING
                printf("[%s] Flag:%s Changed: %s, New: %s\n", client->username, flag, changed, new);

                // Call edit channel function
                edit_channel(changed, new, client);

            } else if (strcmp(type, "ROOM") == 0){
                // Parse data from client
                char *changed = strtok(NULL, " ");
                char *flag = strtok(NULL, " ");
                char *new = strtok(NULL, " ");

                // Check if command is valid
                if (changed == NULL || new == NULL){
                    // DEBUGGING
                    printf("[%s] Error: Invalid command (missing changed/new)\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER);
                    sprintf(response, "MSG,Error: Invalid command (missing changed or new)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // Check if flag is valid
                if (strcmp(flag, "TO") != 0){
                    // DEBUGGING
                    printf("[%s] Error: Invalid command (missing flag)\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER);
                    sprintf(response, "MSG,Error: Invalid command (missing flag)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // DEBUGGING
                printf("[%s] Flag:%s Changed: %s, New: %s\n", client->username, flag, changed, new);

                // Call edit room function
                edit_room(changed, new, client);
                
            } else {
                // DEBUGGING
                printf("[%s] Error: Edit type not found\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER);
                sprintf(response, "MSG,Error: Edit type not found");
                send(client_fd, response, strlen(response), 0);
            
            }

 } else if (strcmp(command, "DEL") == 0){
            // Parse data from client
            char *type = strtok(NULL, " ");

            // Check if command is valid
            if (type == NULL){
                // DEBUGGING
                printf("[%s] Error: Invalid delete (missing type)\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER);
                sprintf(response, "MSG,Error: Invalid command (missing delete type)");
                send(client_fd, response, strlen(response), 0);
                continue;
            }

            // DEBUGGING
            printf("[%s] Command: %s, type: %s\n", client->username, command, type);

            // Delete types
            if (strcmp(type, "CHAT") == 0){
                // Parse data from client
                char *target = strtok(NULL, " ");

                // Check if command is valid
                if (target == NULL){
                    // DEBUGGING
                    printf("[%s] Error: Invalid command (missing target)\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER);
                    sprintf(response, "MSG,Error: Invalid command (missing target)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // DEBUGGING
                printf("[%s] Target: %s\n", client->username, target);

                // Call delete chat function
                del_chat(atoi(target), client);

            } else if (strcmp(type, "CHANNEL") == 0){
                // Parse data from client
                char *channel = strtok(NULL, " ");

                // Check if command is valid
                if (channel == NULL){
                    // DEBUGGING
                    printf("[%s] Error: Invalid command (missing channel)\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER);
                    sprintf(response, "MSG,Error: Invalid command (missing channel)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // DEBUGGING
                printf("[%s] Channel: %s\n", client->username, channel);

                // Call delete channel function
                delete_channel(channel, client);

            } else if (strcmp(type, "ROOM") == 0){
                // Parse data from client
                char *room = strtok(NULL, " ");

                // Check if command is valid
                if (room == NULL){
                    // DEBUGGING
                    printf("[%s] Error: Invalid command (missing room)\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER);
                    sprintf(response, "MSG,Error: Invalid command (missing room)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // Check if ALL is used
                if (strcmp(room, "ALL") == 0){
                    // DEBUGGING
                    printf("[%s] Room: %s\n", client->username, room);

                    // Call delete all room function
                    delete_all_rooms(client);
                    continue;
                }

                // DEBUGGING
                printf("[%s] Room: %s\n", client->username, room);

                // Call delete room function
                delete_room(room, client);

            } else {
                // DEBUGGING
                printf("[%s] Error: Delete type not found\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER);
                sprintf(response, "MSG,Error: Delete type not found");
                send(client_fd, response, strlen(response), 0);
            }

 } else if (strcmp(command, "JOIN") == 0){
            // Parse data from client
            char *target = strtok(NULL, " ");

            // Check if command is valid
            if (target == NULL){
                // DEBUGGING
                printf("[%s] Error: Invalid target\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER);
                sprintf(response, "MSG,Error: Invalid command (missing target)");
                send(client_fd, response, strlen(response), 0);
                continue;
            }

            // DEBUGGING
            printf("[%s] Command: %s, target: %s\n", client->username, command, target);

            // Join channel if user is not in a channel
            if (strlen(client->channel) == 0){
                // Call join channel function
                join_channel(target, client);
            } else if (strlen(client->room) == 0){
                // Call join room function
                join_room(target, client);
            } else {
                // DEBUGGING
                printf("[%s] Error: User is already in a room\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER);
                sprintf(response, "MSG,Error: User is already in a channel");
                send(client_fd, response, strlen(response), 0);
            }

 } else if (strcmp(command, "CHAT") == 0){
            // Parse data from client
            char *message = strtok(NULL, "\n");

            // Check if command is valid
            if (message == NULL){
                // DEBUGGING
                printf("[%s] Error: Invalid message (missing message)\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER);
                sprintf(response, "MSG,Error: Invalid command (missing message)");
                send(client_fd, response, strlen(response), 0);
                continue;
            }

            // Check if message is correctly encased in quotes
            if (message[0] != '"' || message[strlen(message)-1] != '"'){
                // DEBUGGING
                printf("[%s] Error: Invalid message (missing quotes)\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER);
                sprintf(response, "MSG,Error: Invalid command (missing quotes)");
                send(client_fd, response, strlen(response), 0);
                continue;
            }

            // DEBUGGING
            printf("[%s] Command: %s, message: %s\n", client->username, command, message);

            // Call send chat function
            send_chat(message, client);

 } else if (strcmp(command, "REMOVE") == 0){
            // Parse data from client
            char *type = strtok(NULL, " ");

            // Check if command is valid
            if (type == NULL){
                // DEBUGGING
                printf("[%s] Error: Invalid remove (missing type)\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER);
                sprintf(response, "MSG,Error: Invalid command (missing remove type)");
                send(client_fd, response, strlen(response), 0);
                continue;
            }

            // DEBUGGING
            printf("[%s] Command: %s, type: %s\n", client->username, command, type);

            // Check if type is USER
            if (strcmp(type, "USER") == 0){
                // Parse data from client
                char *target = strtok(NULL, " ");

                // Check if command is valid
                if (target == NULL){
                    // DEBUGGING
                    printf("[%s] Error: Invalid remove (missing target)\n", client->username);

                    // Send response to client
                    memset(response, 0, MAX_BUFFER);
                    sprintf(response, "MSG,Error: Invalid command (missing remove target)");
                    send(client_fd, response, strlen(response), 0);
                    continue;
                }

                // DEBUGGING
                printf("[%s] Target: %s\n", client->username, type);

                // Call kick user function
                kick_user(target, client);
            } else {
                // DEBUGGING
                printf("[%s] Target: %s\n", client->username, type);

                // Call remove user function
                remove_user(type, client);
            }

 } else if (strcmp(command, "BAN") == 0){
            // Parse data from client
            char *target = strtok(NULL, " ");

            // Check if command is valid
            if (target == NULL){
                // DEBUGGING
                printf("[%s] Error: Invalid ban (missing target)\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER);
                sprintf(response, "MSG,Error: Invalid command (missing ban target)");
                send(client_fd, response, strlen(response), 0);
                continue;
            }

            // DEBUGGING
            printf("[%s] Command: %s, type: %s\n", client->username, command, target);

            // Call ban user function
            ban_user(target, client);

 } else if (strcmp(command, "UNBAN") == 0){
            // Parse data from client
            char *target = strtok(NULL, " ");

            // Check if command is valid
            if (target == NULL){
                // DEBUGGING
                printf("[%s] Error: Invalid unban (missing target)\n", client->username);

                // Send response to client
                memset(response, 0, MAX_BUFFER);
                sprintf(response, "MSG,Error: Invalid command (missing unban target)");
                send(client_fd, response, strlen(response), 0);
                continue;
            }

            // DEBUGGING
            printf("[%s] Command: %s, type: %s\n", client->username, command, target);

            // Call unban user function
            unban_user(target, client);

        } else {
            // DEBUGGING
            printf("[%s] Error: Command not found\n", client->username);

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

    // Prepare response
    char response[MAX_BUFFER];

    // Check if username has comma or is called USER
    if (strchr(username, ',') != NULL
        || strcmp(username, "USER") == 0) {
        // DEBUGGING
        printf("[REGISTER] Error: Username not allowed\n");

        // Send response to client
        sprintf(response, "MSG,Error: Username not allowed");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Open file
    FILE *file = fopen(users_csv, "a+");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[REGISTER] Error: Unable to open file\n");

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop through id and username
    int id = 0; char namecheck[MAX_BUFFER];
    while (fscanf(file, "%d,%[^,],%*s", &id, namecheck) == 2) {
        // DEBUGGING
        printf("[REGISTER] id: %d, name: %s\n", id, namecheck);
        // sleep(1);

        // Fail if username already exists
        if (strcmp(namecheck, username) == 0) {
            // DEBUGGING
            printf("[REGISTER] Error: Username already exists\n");

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
    char role[8];
    if (id == 0) strcpy(role, "ROOT");
    else         strcpy(role, "USER");

    // DEBUGGING
    printf("[REGISTER] id: %d, name: %s, pass: %s, role: %s\n", id+1, username, hash, role);
    
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

    // Prepare response
    char response[MAX_BUFFER];

    // Hash password from input
    char hash[MAX_BUFFER];
    strcpy(hash,crypt(password, HASHCODE));
    
    // Open file
    FILE *file = fopen(users_csv, "r");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[LOGIN] Error: Unable to open file\n");

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return 0;
    }

    // Loop through id, username, and password
    int id = 0; char namecheck[MAX_BUFFER], passcheck[MAX_BUFFER], role[8];
    while (fscanf(file, "%d,%[^,],%[^,],%s", &id, namecheck, passcheck, role) == 4) {
        // DEBUGGING
        printf("[LOGIN] id: %d, name: %s, pass: %s, role: %s\n", id, namecheck, passcheck, role);

        // Fail if username and password do not match
        if (strcmp(namecheck, username) == 0) {
            if (strcmp(passcheck, hash) == 0) {
                // DEBUGGING
                printf("[LOGIN] Success: Username and password match\n");

                // Finishing up
                client->id = id;
                strcpy(client->username, username);
                strcpy(client->role, role);
                fclose(file);

                // DEBUGGING
                printf("[LOGIN] client_id: %d, client_name: %s, client_role: %s\n", client->id, client->username, client->role);

                // Send response to client
                sprintf(response, "LOGIN,Welcome to DiscorIT!\n");
                send(client_fd, response, strlen(response), 0);
                return 1;
            }

            // DEBUGGING
            printf("[LOGIN] Error: Password does not match\n");

            // Fail if password does not match
            sprintf(response, "MSG,Error: Password does not match");
            send(client_fd, response, strlen(response), 0);
            fclose(file);
            return 0;
        }
    }

    // DEBUGGING
    printf("[LOGIN] Error: Username does not exist\n");

    // Send response to client
    sprintf(response, "MSG,Error: Username does not exist");
    send(client_fd, response, strlen(response), 0);
    fclose(file);
    return 0;
}

//===========================================================================================//
//----------------------------------------- CHANNELS ----------------------------------------//
//===========================================================================================//

//====================//
// INIT CHANNEL ADMIN //
//====================//

void admin_init_channel(char *path_auth, client_data *client){
    // Write to auth.csv
    FILE *file_auth = fopen(path_auth, "a");

    // Fail if file cannot be opened
    if (file_auth == NULL) {
        // DEBUGGING
        printf("[%s][INIT AUTH] Error: Unable to open file\n", client->username);
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
    int id = 0;

    // Open channel list
    FILE *file = fopen(channels_csv, "r");
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][CHECK CHANNEL] Error: Unable to open file\n", client->username);

        // Return 0 if file does not exist
        return id;
    }

    // Loop through channel list
    char channelcheck[100];
    while (fscanf(file, "%d,%[^,],%*s", &id, channelcheck) == 2) {
        if (strcmp(channel, channelcheck) == 0) {
            // DEBUGGING
            printf("[%s][CHECK CHANNEL] Channel exists\n", client->username);

            // Return -2 if channel exists (dont ask why i picked -2)
            return -2;
        }
    }

    // Close file
    fclose(file);

    // DEBUGGING
    printf("[%s][CHECK CHANNEL] Channel does not exist\n", client->username);

    // Return 0 if channel does not exist
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

    // DEBUGGING
    printf("[%s][CREATE CHANNEL] id: %d\n", client->username);

    // Fail if channel already exists
    if (id == -2) {
        // DEBUGGING
        printf("[%s][CREATE CHANNEL] Error: Channel already exists\n", client->username);

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

    // DEBUGGING
    printf("[%s][CREATE CHANNEL] Success: Channel %s created\n", client->username, channel);

    // Write to log
    char message[MAX_BUFFER];
    sprintf(message, "%s created channel \"%s\"\n", client->username, channel);
    write_log(channel, message);

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
        printf("[%s][LIST CHANNEL] Error: Unable to open file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop through id and channel
    int id = 1; char channel[MAX_BUFFER];
    while (fscanf(file, "%d,%[^,],%*s", &id, channel) == 2) {
        // DEBUGGING
        printf("[%s][LIST CHANNEL] id: %d, channel: %s\n", client->username, id, channel);

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
        // DEBUGGING
        printf("[%s][LIST CHANNEL] No channels found\n", client->username);

        sprintf(response, "MSG,No channels found");
        send(client_fd, response, strlen(response), 0);
    } else {
        // DEBUGGING
        printf("[%s][LIST CHANNEL] Channels found: %d\n", client->username, channels_found);

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
        printf("[%s][JOIN CHANNEL] Error: Channel does not exist\n", client->username);

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
        printf("[%s][JOIN CHANNEL] Error: Unable to open auth file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open auth file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop through username and role
    char username[MAX_BUFFER]; char role[8];
    while (fscanf(file, "%*d,%[^,],%s", username, role) == 2) {
        // DEBUGGING
        printf("[%s][JOIN CHANNEL] username: %s, role: %s\n", client->username, username, role);

        // If there is a username match or user is root
        if (strcmp(client->username, username) == 0) {
            // Check if user is banned
            if (strcmp(role, "BANNED") == 0) {
                // DEBUGGING
                printf("[%s][JOIN CHANNEL] Error: User is banned from %s\n", client->username, channel);

                // Close file
                fclose(file);

                // Send response to client
                sprintf(response, "MSG,Error: User is banned from channel");
                send(client_fd, response, strlen(response), 0);
                return;
            }

            // DEBUGGING
            printf("[%s][JOIN CHANNEL] Success: User joined %s\n", client->username, channel);

            // Close file
            fclose(file);

            // Update client channel
            strcpy(client->channel, channel);

            // Write to log
            char message[MAX_BUFFER];
            sprintf(message, "%s joined channel \"%s\"\n", client->username, channel);
            write_log(channel, message);

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
        printf("[%s][JOIN CHANNEL] Success: Root joined %s\n", client->username, channel);

        // Close file
        fclose(file);

        // Update client channel
        strcpy(client->channel, channel);

        // Write to log
        char message[MAX_BUFFER];
        sprintf(message, "%s joined channel \"%s\" as ROOT\n", client->username, channel);

        // Send response to client
        sprintf(response, "CHANNEL,Joined channel %s as ROOT,%s", channel, channel);
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Verify key if user is not listed
    if (verify_key(channel, client) == 1) {
        // Open auth of current channel and add user
        fprintf(file, "%d,%s,USER\n", client->id, client->username);

        // DEBUGGING
        printf("[%s][JOIN CHANNEL] Success: User joined %s\n", client->username, channel);

        // Close file
        fclose(file);

        // Update client channel
        strcpy(client->channel, channel);

        // Write to log
        char message[MAX_BUFFER];
        sprintf(message, "%s verified for channel \"%s\"\n", client->username, channel);
        write_log(channel, message);

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
        printf("[%s][GET KEY] Error: Unable to open file\n", client->username);

        // Return NULL if file cannot be opened
        return NULL;
    }

    // Loop through id, channel, and key
    int id; char channelcheck[MAX_BUFFER];
    char *keycheck = (char *)malloc(sizeof(char) * MAX_BUFFER);
    while (fscanf(file, "%d,%[^,],%s", &id, channelcheck, keycheck) == 3) {
        // DEBUGGING
        printf("[%s][GET KEY] id: %d, channel: %s, key: %s\n", client->username, id, channelcheck, keycheck);

        // Return key if channel exists
        if (strcmp(channel, channelcheck) == 0) {
            // DEBUGGING
            printf("[%s][GET KEY] Success: Key found\n", client->username);

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

int verify_key(char *channel, client_data *client) {
    int client_fd = client->socket_fd;

    // Get key from channel
    char *key = get_key(channel, client);

    // Check if key is not null
    if (key == NULL) {
        // DEBUGGING
        printf("[%s][VERIFY KEY] Error: Key is NULL\n", client->username);

        // Break if key is null
        return -1;
    }

    // Request key from client
    char request[MAX_BUFFER];
    sprintf(request, "KEY,Request to join %s", channel);
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

    // Check if sent from monitor
    if (strcmp(command, "KEY") != 0) {
        // DEBUGGING
        printf("[%s][VERIFY KEY] Error: Monitor send\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Cant verify key from monitor");
        send(client_fd, response, strlen(response), 0);
        return -1;
    }

    // Check if key is not null
    if (keycheck == NULL) {
        // DEBUGGING
        printf("[%s][VERIFY KEY] Error: Key is NULL\n", client->username);

        // Return -1 if key is null
        return -1;
    }

    // DEBUGGING
    printf("[%s][VERIFY KEY] Command: %s, Key: %s\n", client->username, command, keycheck);

    // Hash key
    char hash[MAX_BUFFER];
    strcpy(hash,crypt(keycheck, HASHCODE));

    // Check if key matches
    if (strcmp(command, "KEY") == 0 && strcmp(key, hash) == 0) {
        // DEBUGGING
        printf("[%s][VERIFY KEY] Success: Key matches\n", client->username);

        // Success if key matches
        return 1;
    }

    // DEBUGGING
    printf("[%s][VERIFY KEY] Error: Key does not match\n", client->username);

    // Fail if key does not match
    return -1;    
}

//==============//
// EDIT CHANNEL //
//==============//

void edit_channel(char *changed, char *new, client_data *client){
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_BUFFER];

    // Check permissions
    int perms = check_channel_perms(changed, client);
    if (perms == -1) {
        // DEBUGGING
        printf("[%s][EDIT CHANNEL] Error: Unable to check permissions\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to check permissions");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 0) {
        // DEBUGGING
        printf("[%s][EDIT CHANNEL] Error: User is not privileged\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not privileged");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 1) {
        // DEBUGGING
        printf("[%s][EDIT CHANNEL] User is privileged\n", client->username);
    }

    // Open channel file
    FILE *file = fopen(channels_csv, "r");

    // Open temp channel file
    char temp[MAX_BUFFER];
    sprintf(temp, "%s/.temp_channel.csv", cwd);
    FILE *file_temp = fopen(temp, "w");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][EDIT CHANNEL] Error: Unable to open file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop through id, channel, and key
    int id; char channel[MAX_BUFFER], key[MAX_BUFFER];
    int found = 0;
    while (fscanf(file, "%d,%[^,],%s", &id, channel, key) == 3) {
        // DEBUGGING
        printf("[%s][EDIT CHANNEL] id: %d, channel: %s, key: %s\n", client->username, id, channel, key);

        // Write to temp file
        if (strcmp(changed, channel) == 0) {
            found = 1;
            fprintf(file_temp, "%d,%s,%s\n", id, new, key);
        } else {
            fprintf(file_temp, "%d,%s,%s\n", id, channel, key);
        }
    }

    // Close files
    fclose(file);
    fclose(file_temp);

    // Fail if channel is not found
    if (found == 0) {
        // DEBUGGING
        printf("[%s][EDIT CHANNEL] Error: Channel not found\n", client->username);

        // Remove temp file
        remove(temp);

        // Send response to client
        sprintf(response, "MSG,Error: Channel not found");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Remove old channel file and rename temp file
    remove(channels_csv);
    rename(temp, channels_csv);

    // Update folder name
    char path_channel[MAX_BUFFER];
    sprintf(path_channel, "%s/%s", cwd, changed);
    char path_new[MAX_BUFFER];
    sprintf(path_new, "%s/%s", cwd, new);
    rename_directory(path_channel, path_new);

    // Check if edited channel is the current channel
    if (strcmp(client->channel, changed) == 0) {
        // DEBUGGING
        printf("[%s][EDIT CHANNEL] Channel name self-changed\n", client->username);

        // Update client channel
        strcpy(client->channel, new);

        // Write to log
        char message[MAX_BUFFER];
        sprintf(message, "%s changed channel \"%s\" name to \"%s\"\n", client->username, changed, new);
        write_log(new, message);

        // Send response to client
        sprintf(response, "CHANNEL,Channel name changed to %s,%s", new, new);
        send(client_fd, response, strlen(response), 0);
        return;
    } 
    
    // DEBUGGING
    printf("[%s][EDIT CHANNEL] Channel name changed\n", client->username);

    // Send response to client
    sprintf(response, "MSG,Success: Channel name changed to %s", new);
    send(client_fd, response, strlen(response), 0);
    return;
}

//================//
// DELETE CHANNEL //
//================//

void delete_channel(char *channel, client_data *client) {
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_BUFFER];

    // Check permissions
    int perms = check_channel_perms(channel, client);
    if (perms == -1) {
        // DEBUGGING
        printf("[%s][DELETE CHANNEL] Error: Unable to check permissions\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to check permissions");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 0) {
        // DEBUGGING
        printf("[%s][DELETE CHANNEL] Error: User is not privileged\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not privileged");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 1) {
        // DEBUGGING
        printf("[%s][DELETE CHANNEL] User is privileged\n", client->username);
    }

    // Open channel file
    FILE *file = fopen(channels_csv, "r");

    // Open temp channel file
    char temp[MAX_BUFFER];
    sprintf(temp, "%s/.temp_channel.csv", cwd);
    FILE *file_temp = fopen(temp, "w");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][DELETE CHANNEL] Error: Unable to open file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop through id, channel, and key
    int id; char channelcheck[MAX_BUFFER], key[MAX_BUFFER];
    int found = 0;
    while (fscanf(file, "%d,%[^,],%s", &id, channelcheck, key) == 3) {
        // DEBUGGING
        printf("[%s][DELETE CHANNEL] id: %d, channel: %s, key: %s\n", client->username, id, channelcheck, key);

        // Write to temp file
        if (strcmp(channel, channelcheck) != 0) {
            fprintf(file_temp, "%d,%s,%s\n", id, channelcheck, key);
        } else {
            found = 1;
        }
    }

    // Close files
    fclose(file);
    fclose(file_temp);

    // Fail if channel is not found
    if (found == 0) {
        // DEBUGGING
        printf("[%s][DELETE CHANNEL] Error: Channel not found\n", client->username);

        // Remove temp file
        remove(temp);

        // Send response to client
        sprintf(response, "MSG,Error: Channel not found");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Remove old channel file and rename temp file
    remove(channels_csv);
    rename(temp, channels_csv);

    // Remove channel directory
    char path_channel[MAX_BUFFER];
    sprintf(path_channel, "%s/%s", cwd, channel);
    remove_directory(path_channel);

    // Check if deleted channel is the current channel
    if (strcmp(client->channel, channel) == 0) {
        // DEBUGGING
        printf("[%s][DELETE CHANNEL] Channel current deleted\n", client->username);

        // Update client channel
        strcpy(client->channel, "");

        // Update client room in case client is in a room
        strcpy(client->room, "");

        // Send response to client
        sprintf(response, "EXIT,Channel %s deleted,CHANNEL", channel, channel);
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // DEBUGGING
    printf("[%s][DELETE CHANNEL] Channel deleted\n", client->username);

    // Send response to client
    sprintf(response, "MSG,Success: Channel %s deleted", channel);
    send(client_fd, response, strlen(response), 0);
    return;
}


//===========================================================================================//
//----------------------------------------- ROOMS -------------------------------------------//
//===========================================================================================//

//=============//
// CREATE ROOM //
//=============//

void create_room(char *room, client_data *client) {
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is in a channel
    if (strlen(client->channel) == 0) {
        // DEBUGGING
        printf("[%s][CREATE ROOM] Error: User is not in a channel\n", client->username);

        // Prepare response
        sprintf(response, "MSG,Error: User is not in a channel");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Check if room name is valid 
    if (strcmp(room, "ALL") == 0) {
        // DEBUGGING
        printf("[%s][CREATE ROOM] Error: Invalid room name\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Invalid room name");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Open auth of current channel
    char path_auth[MAX_BUFFER];
    sprintf(path_auth, "%s/%s/admin/auth.csv", cwd, client->channel);
    FILE *file = fopen(path_auth, "r");

    // Fail if file cannot be opened    
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][CREATE ROOM] Error: Unable to open auth file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop through id and role
    int id; char role[8];
    while (fscanf(file, "%d,%*[^,],%s", &id, role) == 2) {
        // DEBUGGING
        printf("[%s][CREATE ROOM] id: %d, role: %s\n", client->username, id, role);

        // Fail if user is not admin/root
        if (client->id == id
            && strcmp(role, "ADMIN") != 0
            && strcmp(role, "ROOT") != 0){
            // DEBUGGING
            printf("[%s][CREATE ROOM] Error: User has no privileges\n", client->username);

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
        printf("[%s][CREATE ROOM] Error: Invalid room name\n", client->username);

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
        printf("[%s][CREATE ROOM] Error: Unable to create chat file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to create chat file");
        send(client_fd, response, strlen(response), 0);
        return;
    }
    
    // Close file
    fclose(file_chat);

    // Write to log
    char message[MAX_BUFFER];
    sprintf(message, "%s created room \"%s\"\n", client->username, room);
    write_log(client->channel, message);

    // Send response to client
    sprintf(response, "MSG,Success: Room %s created", room);
    send(client_fd, response, strlen(response), 0);
    return;
}

//============//
// LIST ROOMS //
//============//

void list_room(client_data *client) {
    int client_fd = client->socket_fd;

    // Check if user is in a channel
    if (strlen(client->channel) == 0) {
        // DEBUGGING
        printf("[%s][LIST ROOM] Error: User is not in a channel\n", client->username);

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
        printf("[%s][LIST ROOM] Error: Unable to open directory\n", client->username);

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
        printf("[%s][LIST ROOM] Room: %s\n", client->username, entry->d_name);

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

    // DEBUGGING
    printf("[%s][LIST ROOM] Rooms found: %d\n", client->username, rooms_found);

    // Send response to client
    if (rooms_found == 0) {
        sprintf(response, "MSG,No rooms found");
        send(client_fd, response, strlen(response), 0);
    } else {
        send(client_fd, response, strlen(response), 0);
    }

    return;
}

//===========//
// JOIN ROOM //
//===========//

void join_room(char *room, client_data *client) {
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is trying to join admin
    if (strcmp(room, "admin") == 0) {
        // DEBUGGING
        printf("[%s][JOIN ROOM] Error: Invalid room name\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Invalid room name");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Check if user is in a channel
    if (strlen(client->channel) == 0) {
        // DEBUGGING
        printf("[%s][JOIN ROOM] Error: User is not in a channel\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not in a channel");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Check if room exists
    char path_room[MAX_BUFFER];
    sprintf(path_room, "%s/%s/%s", cwd, client->channel, room);
    struct stat statbuf;
    if (stat(path_room, &statbuf) == -1) {
        // DEBUGGING
        printf("[%s][JOIN ROOM] Error: Room does not exist\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Room does not exist");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Join room if user is not in a room
    if (strlen(client->room) == 0) {
        // DEBUGGING
        printf("[%s][JOIN ROOM] Success: User joined %s\n", client->username, room);

        // Update client room
        strcpy(client->room, room);

        // Write to log
        char message[MAX_BUFFER];
        sprintf(message, "%s joined room \"%s\"\n", client->username, room);
        write_log(client->channel, message);

        // Send response to client
        sprintf(response, "ROOM,Joined room %s,%s", room, room);
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // DEBUGGING
    printf("[%s][JOIN ROOM] Error: User is already in a room\n", client->username);

    // Send response to client
    sprintf(response, "MSG,Error: User is already in a room");
    send(client_fd, response, strlen(response), 0);
}

//===========//
// EDIT ROOM //
//===========//

void edit_room(char *changed, char *new, client_data *client){
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is in a channel
    if (strlen(client->channel) == 0) {
        // DEBUGGING
        printf("[%s][EDIT ROOM] Error: User is not in a channel\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not in a channel");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Check permissions
    int perms = check_channel_perms(client->channel, client);
    if (perms == -1) {
        // DEBUGGING
        printf("[%s][EDIT ROOM] Error: Unable to check permissions\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to check permissions");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 0) {
        // DEBUGGING
        printf("[%s][EDIT ROOM] Error: User is not privileged\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not privileged");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 1) {
        // DEBUGGING
        printf("[%s][EDIT ROOM] User is privileged\n", client->username);
    }

    // Prepare room path
    char path_room[MAX_BUFFER];
    sprintf(path_room, "%s/%s/%s", cwd, client->channel, changed);

    // Check if room exists
    struct stat statbuf;
    if (stat(path_room, &statbuf) == -1) {
        // DEBUGGING
        printf("[%s][EDIT ROOM] Error: Room does not exist\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Room does not exist");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Rename room directory
    char path_new[MAX_BUFFER];
    sprintf(path_new, "%s/%s/%s", cwd, client->channel, new);
    rename_directory(path_room, path_new);

    // Check if edited room is the current room
    if (strcmp(client->room, changed) == 0) {
        // DEBUGGING
        printf("[%s][EDIT ROOM] Room name changed\n", client->username);

        // Update client room
        strcpy(client->room, new);

        // Write to log
        char message[MAX_BUFFER];
        sprintf(message, "%s changed room name to \"%s\"\n", client->username, new);
        write_log(client->channel, message);

        // Send response to client
        sprintf(response, "ROOM,Room name changed to %s,%s", new, new);
        send(client_fd, response, strlen(response), 0);
        return;
    } 
    
    // DEBUGGING
    printf("[%s][EDIT ROOM] Room name changed\n", client->username);

    // Write to log
    char message[MAX_BUFFER];
    sprintf(message, "%s changed room name \"%s\" to \"%s\"\n", client->username, changed, new);
    write_log(client->channel, message);

    // Send response to client
    sprintf(response, "MSG,Success: Room name changed to %s", new);
    send(client_fd, response, strlen(response), 0);
    return;
}

//=============//
// DELETE ROOM //
//=============//

void delete_room(char *room, client_data *client) {
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is in a channel
    if (strlen(client->channel) == 0) {
        // DEBUGGING
        printf("[%s][DELETE ROOM] Error: User is not in a channel\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not in a channel");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Check permissions
    int perms = check_channel_perms(client->channel, client);
    if (perms == -1) {
        // DEBUGGING
        printf("[%s][DELETE ROOM] Error: Unable to check permissions\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to check permissions");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 0) {
        // DEBUGGING
        printf("[%s][DELETE ROOM] Error: User is not privileged\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not privileged");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 1) {
        // DEBUGGING
        printf("[%s][DELETE ROOM] User is privileged\n", client->username);
    }

    // Prepare room path
    char path_room[MAX_BUFFER];
    sprintf(path_room, "%s/%s/%s", cwd, client->channel, room);

    // Check if room exists
    struct stat statbuf;
    if (stat(path_room, &statbuf) == -1) {
        // DEBUGGING
        printf("[%s][DELETE ROOM] Error: Room does not exist\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Room does not exist");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Remove room directory
    remove_directory(path_room);

    // Check if deleted room is the current room
    if (strcmp(client->room, room) == 0) {
        // DEBUGGING
        printf("[%s][DELETE ROOM] Room current deleted %s\n", client->username);

        // Update client room
        strcpy(client->room, "");

        // Write to log
        char message[MAX_BUFFER];
        sprintf(message, "%s deleted room \"%s\"\n", client->username, room);
        write_log(client->channel, message);

        // Send response to client
        sprintf(response, "EXIT,Room %s deleted,ROOM", room, room);
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // DEBUGGING
    printf("[%s][DELETE ROOM] Room deleted\n", client->username);

    // Write to log
    char message[MAX_BUFFER];
    sprintf(message, "%s deleted room \"%s\"\n", client->username, room);
    write_log(client->channel, message);

    // Send response to client
    sprintf(response, "MSG,Success: Room %s deleted", room);
    send(client_fd, response, strlen(response), 0);
    return;
}

void delete_all_rooms(client_data *client) {
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is in a channel
    if (strlen(client->channel) == 0) {
        // DEBUGGING
        printf("[%s][DELETE ALL ROOMS] Error: User is not in a channel\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not in a channel");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Check permissions
    int perms = check_channel_perms(client->channel, client);
    if (perms == -1) {
        // DEBUGGING
        printf("[%s][DELETE ALL ROOMS] Error: Unable to check permissions\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to check permissions");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 0) {
        // DEBUGGING
        printf("[%s][DELETE ALL ROOMS] Error: User is not privileged\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not privileged");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 1) {
        // DEBUGGING
        printf("[%s][DELETE ALL ROOMS] User is privileged\n", client->username);
    }

    // Prepare channel path
    char path_channel[MAX_BUFFER];
    sprintf(path_channel, "%s/%s", cwd, client->channel);


    // Open channel directory
    DIR *dir = opendir(path_channel);

    // Fail if directory cannot be opened
    if (dir == NULL) {
        // DEBUGGING
        printf("[%s][DELETE ALL ROOMS] Error: Unable to open directory\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open directory");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop through directory
    int rooms_found = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        // DEBUGGING
        printf("[%s][DELETE ALL ROOMS] Room: %s\n", client->username, entry->d_name);

        // Skip admin directory
        if (strcmp(entry->d_name, "admin") == 0) continue;

        // Prepare entry path
        char entry_path[MAX_BUFFER];
        sprintf(entry_path, "%s/%s", path_channel, entry->d_name);

        // Check if entry is a directory to be deleted
        struct stat statbuf;
        if (stat(entry_path, &statbuf) == -1) continue;
        if (S_ISDIR(statbuf.st_mode)) {
            // Increment rooms found
            rooms_found++;

            // Remove room directory
            remove_directory(entry_path);
        }
    }

    // Close directory
    closedir(dir);

    // DEBUGGING
    printf("[%s][DELETE ALL ROOMS] Rooms found: %d\n", client->username, rooms_found);

    // Write to log
    char message[MAX_BUFFER];
    sprintf(message, "%s deleted all rooms of channel \"%s\"\n", client->username, client->channel);
    write_log(client->channel, message);

    // Check if user is in a room
    if (strlen(client->room) != 0) {
        // DEBUGGING
        printf("[%s][DELETE ALL ROOMS] User is in a room\n", client->username);

        // Update client room
        strcpy(client->room, "");

        // Send response to client
        sprintf(response, "EXIT,All rooms deleted,ROOM");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Send response to client
    if (rooms_found == 0) {
        sprintf(response, "MSG,No rooms found");
        send(client_fd, response, strlen(response), 0);
    } else {
        sprintf(response, "MSG,Success: All rooms deleted");
        send(client_fd, response, strlen(response), 0);
    }

    return;
}

//===========================================================================================//
//------------------------------------------- USERS -----------------------------------------//
//===========================================================================================//

//======================//
// CHECK USER EXISTENCE //
//======================//

int check_user(client_data *client) {
    int client_fd = client->socket_fd;

    // Open file
    FILE *file = fopen(users_csv, "r");

    // Prepare response
    char response[MAX_BUFFER];

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][CHECK USER] Error: Unable to open file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return 0;
    }

    // Loop through id and username
    int id; char namecheck[MAX_BUFFER];
    while (fscanf(file, "%d,%[^,],%*s", &id, namecheck) == 2) {
        // DEBUGGING
        printf("[%s][CHECK USER] id: %d, name: %s\n", client->username, id, namecheck);

        // Return 1 if user exists
        if (strcmp(namecheck, client->username) == 0) {
            // DEBUGGING
            printf("[%s][CHECK USER] Success: User exists\n", client->username);

            // Close file
            fclose(file);
            return 1;
        }
    }

    // Close file
    fclose(file);

    // DEBUGGING
    printf("[%s][CHECK USER] Error: User does not exist\n", client->username);

    // Return 0 if user does not exist
    return 0;
}


//=================//
// PRINT USER DATA //
//=================//

void see_user(client_data *client) {
    // DEBUGGING
    printf("[%s] USER data fetched\n", client->username);

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
        printf("[%s][LIST USER] Error: User is not in a channel\n", client->username);

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
        printf("[%s][LIST USER] Error: Unable to open file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop through id and username
    int id; char username[MAX_BUFFER];
    while (fscanf(file, "%d,%[^,],%*s", &id, username) == 2) {
        // DEBUGGING
        printf("[%s][LIST USER] id: %d, username: %s\n", client->username, id, username);

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

//===========//
// EXIT USER //
//===========//

void exit_user(client_data *client){
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is in a channel
    if (strlen(client->channel) == 0) {
        // DEBUGGING
        printf("[%s][EXIT DISCORIT]\n", client->username);

        // Send response to client
        sprintf(response, "QUIT,Gracefully disconnecting from server...");
        send(client_fd, response, strlen(response), 0);

        // Close client connection
        close(client_fd);
        free(client);
        pthread_exit(NULL);
        return;
    }

    // Check if user is in a room
    if (strlen(client->room) == 0) {
        // DEBUGGING
        printf("[%s][EXIT CHANNEL] Exiting channel\n", client->username);

        // Exit channel since user is not in a room
        strcpy(client->channel, "");
        sprintf(response, "EXIT,Exited channel %s,CHANNEL", client->channel);
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // DEBUGGING
    printf("[%s][EXIT ROOM] Exiting room\n", client->username);

    // Exit room since user is in a room
    strcpy(client->room, "");
    sprintf(response, "EXIT,Exited room %s,ROOM", client->room);
    send(client_fd, response, strlen(response), 0);
    return;
}

//================//
// EDIT AUTH USER //
//================//

void edit_username_auth(char *username, char *newusername, client_data *client){
    int client_fd = client->socket_fd;

    // DEBUGGING
    printf("[%s][EDIT USERNAME AUTH] username: %s, newusername: %s\n", client->username, username, newusername);

    // Prepare response
    char response[MAX_BUFFER];

    // Loop through all channels
    FILE *file_channel = fopen(channels_csv, "r");

    // Fail if file cannot be opened
    if (file_channel == NULL) {
        // DEBUGGING
        printf("[%s][EDIT USERNAME AUTH] Error: Unable to open file\n", client->username);

        // Send response to client
        char response[MAX_BUFFER];
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop through channel name
    char channel[MAX_BUFFER];
    while (fscanf(file_channel, "%*d,%[^,],%*s", channel) == 1) {
        // Prepare auth path
        char path_auth[MAX_BUFFER];
        sprintf(path_auth, "%s/%s/admin/auth.csv", cwd, channel);

        // Open auth file
        FILE *file_auth = fopen(path_auth, "r");

        // Fail if file cannot be opened
        if (file_auth == NULL) {
            // DEBUGGING
            printf("[%s][EDIT USERNAME AUTH] Error: Unable to open file\n", client->username);

            // Send response to client
            char response[MAX_BUFFER];
            sprintf(response, "MSG,Error: Unable to open file");
            send(client_fd, response, strlen(response), 0);
            return;
        }

        // Open temp file
        char temp_csv[MAX_BUFFER];
        sprintf(temp_csv, "%s/.temp_auth.csv", cwd);
        FILE *temp = fopen(temp_csv, "w");

        // Loop through id, username, and role
        int id; char namecheck[MAX_BUFFER], role[8];
        while (fscanf(file_auth, "%d,%[^,],%s", &id, namecheck, role) == 3) {
            // DEBUGGING
            printf("[%s][EDIT USERNAME AUTH] id: %d, name: %s, role: %s\n", client->username, id, namecheck, role);

            // Check if username matches
            if (strcmp(namecheck, username) == 0) {
                // DEBUGGING
                printf("[%s][EDIT USERNAME AUTH] Success: Username found\n", client->username);

                // Write to log
                char message[MAX_BUFFER];
                sprintf(message, "%s changed username %s to %s\n", client->username, username, newusername);
                write_log(channel, message);

                // Write new username to temp file
                fprintf(temp, "%d,%s,%s\n", id, newusername, role);
            } else {
                // Write old username to temp file
                fprintf(temp, "%d,%s,%s\n", id, namecheck, role);
            }
        }

        // Close files
        fclose(file_auth);
        fclose(temp);

        // Remove old file and rename temp file
        remove(path_auth);
        rename(temp_csv, path_auth);
    }

    // Check if user is editing self
    if (strcmp(client->username, username) == 0) {
        // Update client username
        strcpy(client->username, newusername);

        // DEBUGGING
        printf("[%s][EDIT USERNAME AUTH] Success: Username self-edit\n", client->username);

        // Send response to client
        sprintf(response, "USERNAME,Success: Username edited,%s", newusername);
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // DEBUGGING
    printf("[%s][EDIT USERNAME AUTH] Success: Username edited\n", client->username);

    // Send response to client
    sprintf(response, "MSG,Success: Username edited");
    send(client_fd, response, strlen(response), 0);
}


//================//
// EDIT USER NAME //
//================//

void edit_username(char *username, char *newusername, client_data *client) {
    int client_fd = client->socket_fd;

    // DEBUGGING
    printf("[%s][EDIT USERNAME] username: %s, newusername: %s\n", client->username, username, newusername);

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is admin or root
    if (strcmp(client->role, "USER") == 0)
    // Check if user is editing self
    if (strcmp(client->username, username) != 0) {
        // DEBUGGING
        printf("[%s][EDIT USERNAME] Error: User is not root\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not root");
        send(client_fd, response, strlen(response), 0);
        return;
    }
    // Open file
    FILE *file = fopen(users_csv, "r");

    // Open temp file
    char temp_csv[MAX_BUFFER];
    sprintf(temp_csv, "%s/.temp_users.csv", cwd);
    FILE *temp = fopen(temp_csv, "w");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][EDIT USERNAME] Error: Unable to open file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop until username matches
    int id; char namecheck[MAX_BUFFER], passcheck[MAX_BUFFER], role[8];
    int found = 0;
    while (fscanf(file, "%d,%[^,],%[^,],%s", &id, namecheck, passcheck, role) == 4) {
        // DEBUGGING
        printf("[%s][EDIT USERNAME] id: %d, name: %s, pass: %s, role: %s\n", client->username, id, namecheck, passcheck, role);

        // Check if username matches
        if (strcmp(namecheck, username) == 0) {
            // DEBUGGING
            printf("[%s][EDIT USERNAME] Success: Username found\n", client->username);

            // Write new username to temp file
            fprintf(temp, "%d,%s,%s,%s\n", id, newusername, passcheck, role);
            found = 1;
        } else {
            // Write old username to temp file
            fprintf(temp, "%d,%s,%s,%s\n", id, namecheck, passcheck, role);
        }
    }

    // Close files
    fclose(file);
    fclose(temp);

    // Fail if username does not match
    if (found == 0) {
        // DEBUGGING
        printf("[%s][EDIT USERNAME] Error: Username not found\n", client->username);

        // Remove temp file
        remove(temp_csv);

        // Send response to client
        sprintf(response, "MSG,Error: Username not found");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Remove old file and rename temp file
    remove(users_csv);
    rename(temp_csv, users_csv);

    // DEBUGGING
    printf("[%s][EDIT USERNAME] Success: Main username edited\n", client->username);

    // Call edit_username_auth
    edit_username_auth(username, newusername, client);
    return;
}

//================//
// EDIT USER PASS //
//================//

void edit_password(char *username, char *newpassword, client_data *client) {
    int client_fd = client->socket_fd;

    // DEBUGGING
    printf("[%s][EDIT PASSWORD] username: %s, newpassword: %s\n", client->username, username, newpassword);

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is admin or root
    if (strcmp(client->role, "USER") == 0)
    // Check if user is editing self
    if (strcmp(client->username, username) != 0) {
        // DEBUGGING
        printf("[%s][EDIT PASSWORD] Error: User is not root\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not root");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Open file
    FILE *file = fopen(users_csv, "r+");

    // Open temp file
    char temp_csv[MAX_BUFFER];
    sprintf(temp_csv, "%s/.temp_users.csv", cwd);
    FILE *temp = fopen(temp_csv, "w");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][EDIT PASSWORD] Error: Unable to open file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop until username matches
    int id; char namecheck[MAX_BUFFER], passcheck[MAX_BUFFER], role[8];
    int found = 0;
    while (fscanf(file, "%d,%[^,],%[^,],%s", &id, namecheck, passcheck, role) == 4) {
        // DEBUGGING
        printf("[%s][EDIT PASSWORD] id: %d, name: %s, pass: %s, role: %s\n", client->username, id, namecheck, passcheck, role);

        // Check if username matches
        if (strcmp(namecheck, username) == 0) {
            // DEBUGGING
            printf("[%s][EDIT PASSWORD] Success: Username found\n", client->username);

            // Hash new password
            char hash[MAX_BUFFER];
            strcpy(hash,crypt(newpassword, HASHCODE));

            // Write new password to temp file
            fprintf(temp, "%d,%s,%s,%s\n", id, namecheck, hash, role);
            found = 1;
        } else {
            // Write old password to temp file
            fprintf(temp, "%d,%s,%s,%s\n", id, namecheck, passcheck, role);
        }
    }

    // Close files
    fclose(file);
    fclose(temp);

    // Fail if username does not match
    if (found == 0) {
        // DEBUGGING
        printf("[%s][EDIT PASSWORD] Error: Username not found\n", client->username);

        // Remove temp file
        remove(temp_csv);

        // Send response to client
        sprintf(response, "MSG,Error: Username not found");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Remove old file and rename temp file
    remove(users_csv);
    rename(temp_csv, users_csv);

    // DEBUGGING
    printf("[%s][EDIT PASSWORD] Success: Password edited\n", client->username);

    // Send response to client
    sprintf(response, "MSG,Success: Password edited");
    send(client_fd, response, strlen(response), 0);
    return;    
}

//===============//
// DEL AUTH USER //
//===============//

void del_username_auth(char *username, client_data *client) {
    int client_fd = client->socket_fd;

    // DEBUGGING
    printf("[%s][DEL USERNAME AUTH] username: %s\n", client->username, username);

    // Prepare response
    char response[MAX_BUFFER];

    // Loop through all channels
    FILE *file_channel = fopen(channels_csv, "r");

    // Fail if file cannot be opened
    if (file_channel == NULL) {
        // DEBUGGING
        printf("[%s][DEL USERNAME AUTH] Error: Unable to open file\n", client->username);

        // Send response to client
        char response[MAX_BUFFER];
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop through channel name
    char channel[MAX_BUFFER];
    while (fscanf(file_channel, "%*d,%[^,],%*s", channel) == 1) {
        // Prepare auth path
        char path_auth[MAX_BUFFER];
        sprintf(path_auth, "%s/%s/admin/auth.csv", cwd, channel);

        // Open auth file
        FILE *file_auth = fopen(path_auth, "r");

        // Fail if file cannot be opened
        if (file_auth == NULL) {
            // DEBUGGING
            printf("[%s][DEL USERNAME AUTH] Error: Unable to open file\n", client->username);

            // Send response to client
            char response[MAX_BUFFER];
            sprintf(response, "MSG,Error: Unable to open file");
            send(client_fd, response, strlen(response), 0);
            return;
        }

        // Open temp file
        char temp_csv[MAX_BUFFER];
        sprintf(temp_csv, "%s/.temp_auth.csv", cwd);
        FILE *temp = fopen(temp_csv, "w");

        // Loop through id, username, and role
        int id; char namecheck[MAX_BUFFER], role[8];
        while (fscanf(file_auth, "%d,%[^,],%s", &id, namecheck, role) == 3) {
            // DEBUGGING
            printf("[%s][DEL USERNAME AUTH] id: %d, name: %s, role: %s\n", client->username, id, namecheck, role);

            // Check if username matches
            if (strcmp(namecheck, username) == 0) {
                // DEBUGGING
                printf("[%s][DEL USERNAME AUTH] Success: Username found\n", client->username);

                // Write to log
                char message[MAX_BUFFER];
                sprintf(message, "%s removed user %s\n", client->username, username);
                write_log(channel, message);
            } else {
                // Write old username to temp file
                fprintf(temp, "%d,%s,%s\n", id, namecheck, role);
            }
        }

        // Close files
        fclose(file_auth);
        fclose(temp);

        // Remove old file and rename temp file
        remove(path_auth);
        rename(temp_csv, path_auth);
    }

    // DEBUGGING
    printf("[%s][DEL USERNAME AUTH] Success: User removed\n", client->username);

    // Send response to client
    sprintf(response, "MSG,Success: User removed");
    send(client_fd, response, strlen(response), 0);
    return;
}


//=============//
// REMOVE USER //
//=============//

void remove_user(char *username, client_data *client) {
    int client_fd = client->socket_fd;

    // DEBUGGING
    printf("[%s][REMOVE USER] username: %s\n", client->username, username);

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is trying to remove self
    if (strcmp(client->username, username) == 0) {
        // DEBUGGING
        printf("[%s][REMOVE USER] Error: User is trying to remove self\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is trying to remove self");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Check if user is admin or root
    if (strcmp(client->role, "USER") == 0) {
        // DEBUGGING
        printf("[%s][REMOVE USER] Error: User is not root\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not root");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Open file
    FILE *file = fopen(users_csv, "r");

    // Open temp file
    char temp_csv[MAX_BUFFER];
    sprintf(temp_csv, "%s/.temp_users.csv", cwd);
    FILE *temp = fopen(temp_csv, "w");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][REMOVE USER] Error: Unable to open file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop until username matches
    int id; char namecheck[MAX_BUFFER], passcheck[MAX_BUFFER], role[8];
    int found = 0;
    while (fscanf(file, "%d,%[^,],%[^,],%s", &id, namecheck, passcheck, role) == 4) {
        // DEBUGGING
        printf("[%s][REMOVE USER] id: %d, name: %s, pass: %s, role: %s\n", client->username, id, namecheck, passcheck, role);

        // Check if username matches
        if (strcmp(namecheck, username) == 0) {
            // Check if role is not root
            if (strcmp(role, "ROOT") == 0) {
                // DEBUGGING
                printf("[%s][REMOVE USER] Error: User is root\n", client->username);

                // Close files
                fclose(file);
                fclose(temp);

                // Remove temp file
                remove(temp_csv);

                // Send response to client
                sprintf(response, "MSG,Error: User is root");
                send(client_fd, response, strlen(response), 0);
                return;
            }

            // DEBUGGING
            printf("[%s][REMOVE USER] Success: Username found\n", client->username);
            found = 1;
        } else {
            // Write old username to temp file
            fprintf(temp, "%d,%s,%s,%s\n", id, namecheck, passcheck, role);
        }
    }

    // Close files
    fclose(file);
    fclose(temp);

    // Fail if username does not match
    if (found == 0) {
        // DEBUGGING
        printf("[%s][REMOVE USER] Error: Username not found\n", client->username);

        // Remove temp file
        remove(temp_csv);

        // Send response to client
        sprintf(response, "MSG,Error: Username not found");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Remove old file and rename temp file
    remove(users_csv);
    rename(temp_csv, users_csv);

    // DEBUGGING
    printf("[%s][REMOVE USER] Success: User removed\n", client->username);

    // Call del_username_auth
    del_username_auth(username, client);
    return;
}

//===========//
// CHECK BAN //
//===========//

int check_ban(client_data *client) {
    int client_fd = client->socket_fd;

    // Check if user is in a channel
    if (strlen(client->channel) == 0) {
        // DEBUGGING
        printf("[%s][CHECK BAN] Error: User is not in a channel\n", client->username);

        // Return 0 if user is not in a channel
        return 0;
    }

    // Open auth file
    char path_auth[MAX_BUFFER];
    sprintf(path_auth, "%s/%s/admin/auth.csv", cwd, client->channel);
    FILE *file = fopen(path_auth, "r");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][CHECK BAN] Error: Unable to open file\n", client->username);

        // Return -1 if file cannot be opened
        return -1;
    }

    // Loop through id and username
    int id; char role[8];
    while (fscanf(file, "%d,%*[^,],%s", &id, role) == 2) {
        // DEBUGGING
        printf("[%s][CHECK BAN] id: %d, role: %s\n", client->username, id, role);

        // Check if username matches
        if (id == client->id) 
        // Check if role is banned
        if (strcmp(role, "BANNED") == 0) {
            // DEBUGGING
            printf("[%s][CHECK BAN] Error: User is banned\n", client->username);

            // Close file
            fclose(file);

            // Return 1 if user is banned
            return 1;
        } else break;
    }

    // Close file
    fclose(file);

    // DEBUGGING
    printf("[%s][CHECK BAN] Success: User is not banned\n", client->username);

    // Return 0 if user is not banned
    return 0;

}

//=====================//
// CHECK CHANNEL PERMS //
//=====================//

int check_channel_perms(char *target, client_data *client) {
    int client_fd = client->socket_fd;

    // Check channel permissions
    FILE *file_channel = fopen(channels_csv, "r");

    // Fail if file cannot be opened
    if (file_channel == NULL) {
        // DEBUGGING
        printf("[%s][CHECK PERMS] Error: Unable to open file\n", client->username);

        // Return -1 if file cannot be opened
        return -1;
    }

    // Loop through channel name to find permissions
    char channel[MAX_BUFFER];
    while (fscanf(file_channel, "%*d,%[^,],%*s", channel) == 1) {
        // DEBUGGING
        printf("[%s][CHECK PERMS] Compare: %s-%s\n", client->username, target, channel);

        // Check channel name
        if (strcmp(channel, target) == 0) {
            // Prepare auth path
            char path_auth[MAX_BUFFER];
            sprintf(path_auth, "%s/%s/admin/auth.csv", cwd, channel);

            // Open auth file
            FILE *file_auth = fopen(path_auth, "r");

            // Fail if file cannot be opened
            if (file_auth == NULL) {
                // DEBUGGING
                printf("[%s][CHECK PERMS] Error: Unable to open file\n", client->username);

                // Return -1 if file cannot be opened
                return -1;
            }

            // Loop through id, username, and role
            char namecheck[MAX_BUFFER], role[8];
            while (fscanf(file_auth, "%*d,%[^,],%s", namecheck, role) == 2) {
                // DEBUGGING
                printf("[%s][CHECK PERMS] Client username: %s, role: %s\n", client->username, namecheck, role);

                // Check if username matches
                if (strcmp(namecheck, client->username) == 0)
                // Check if role is root/admin
                if (strcmp(role, "ROOT") == 0 || strcmp(role, "ADMIN") == 0) {
                    // DEBUGGING
                    printf("[%s][CHECK PERMS] Error: User is privileged\n", client->username);

                    // Close files
                    fclose(file_auth);
                    fclose(file_channel);

                    // Return 1 if user is privileged
                    return 1;
                }
            }

            // Close auth file
            fclose(file_auth);
        }
    }

    // Close channel file
    fclose(file_channel);

    // Return 0 if user is not privileged
    return 0;
}

//=======================//
// BAN USER FROM CHANNEL //
//=======================//

void ban_user(char *username, client_data *client) {
    int client_fd = client->socket_fd;

    // DEBUGGING
    printf("[%s][BAN USER] username: %s\n", client->username, username);

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is in a channel
    if (strlen(client->channel) == 0) {
        // DEBUGGING
        printf("[%s][BAN USER] Error: User is not in a channel\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not in a channel");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Check permissions
    int perms = check_channel_perms(client->channel, client);
    if (perms == -1) {
        // DEBUGGING
        printf("[%s][BAN USER] Error: Unable to check permissions\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to check permissions");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 0) {
        // DEBUGGING
        printf("[%s][BAN USER] Error: User is not privileged\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not privileged");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 1) {
        // DEBUGGING
        printf("[%s][BAN USER] User is privileged\n", client->username);
    }

    // Check if user trying to ban self
    if (strcmp(client->username, username) == 0) {
        // DEBUGGING
        printf("[%s][BAN USER] Error: User is trying to ban self\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is trying to ban self");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Open auth of channel
    char path_auth[MAX_BUFFER];
    sprintf(path_auth, "%s/%s/admin/auth.csv", cwd, client->channel);
    FILE *file_auth = fopen(path_auth, "r");

    // Open temp file
    char temp_csv[MAX_BUFFER];
    sprintf(temp_csv, "%s/%s/admin/.temp_auth.csv", cwd, client->channel);
    FILE *temp = fopen(temp_csv, "w");

    // Fail if file cannot be opened
    if (file_auth == NULL) {
        // DEBUGGING
        printf("[%s][BAN USER] Error: Unable to open auth file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open auth file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop through username and role
    int id; char namecheck[MAX_BUFFER], role[8];
    int found = 0;
    while (fscanf(file_auth, "%d,%[^,],%s", &id, namecheck, role) == 3) {
        // DEBUGGING
        printf("[%s][BAN USER] name: %s, role: %s\n", client->username, namecheck, role);

        // Check if username matches
        if (strcmp(namecheck, username) == 0) {
            // DEBUGGING
            printf("[%s][BAN USER] Success: Username found\n", client->username);

            // Check if role is not root/admin
            if (strcmp(role, "ROOT") == 0 || strcmp(role, "ADMIN") == 0) {
                // DEBUGGING
                printf("[%s][BAN USER] Error: User is root/admin\n", client->username);

                // Close files
                fclose(file_auth);
                fclose(temp);

                // Remove temp file
                remove(temp_csv);

                // Send response to client
                sprintf(response, "MSG,Error: User is root/admin");
                send(client_fd, response, strlen(response), 0);
                return;
            }

            // Write new role to temp file
            fprintf(temp, "%d,%s,BANNED\n", id, namecheck);
            found = 1;
        } else {
            // Write old role to temp file
            fprintf(temp, "%d,%s,%s\n", id, namecheck, role);
        }
    }

    // Close files
    fclose(file_auth);
    fclose(temp);

    // Fail if username does not match
    if (found == 0) {
        // DEBUGGING
        printf("[%s][BAN USER] Error: Username not found\n", client->username);

        // Remove temp file
        remove(temp_csv);

        // Send response to client
        sprintf(response, "MSG,Error: Username not found");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Remove old file and rename temp file
    remove(path_auth);
    rename(temp_csv, path_auth);

    // DEBUGGING
    printf("[%s][BAN USER] Success: %s banned\n", client->username, username);

    // Write to log
    char message[MAX_BUFFER];
    sprintf(message, "%s banned user %s from %s\n", client->username, username, client->channel);
    write_log(client->channel, message);

    // Send response to client
    sprintf(response, "MSG,Success: %s banned", username);
    send(client_fd, response, strlen(response), 0);
    return;
}

//========================//
// KICK USER FROM CHANNEL //
//========================//

void kick_user(char *username, client_data *client) {
    int client_fd = client->socket_fd;

    // DEBUGGING
    printf("[%s][KICK USER] username: %s\n", client->username, username);

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is in a channel
    if (strlen(client->channel) == 0) {
        // DEBUGGING
        printf("[%s][KICK USER] Error: User is not in a channel\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not in a channel");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Check permissions
    int perms = check_channel_perms(client->channel, client);
    if (perms == -1) {
        // DEBUGGING
        printf("[%s][KICK USER] Error: Unable to check permissions\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to check permissions");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 0) {
        // DEBUGGING
        printf("[%s][KICK USER] Error: User is not privileged\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not privileged");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 1) {
        // DEBUGGING
        printf("[%s][KICK USER] User is privileged\n", client->username);
    }

    // Open auth of channel
    char path_auth[MAX_BUFFER];
    sprintf(path_auth, "%s/%s/admin/auth.csv", cwd, client->channel);
    FILE *file_auth = fopen(path_auth, "r");

    // Open temp file
    char temp_csv[MAX_BUFFER];
    sprintf(temp_csv, "%s/%s/admin/.temp_auth.csv", cwd, client->channel);
    FILE *temp = fopen(temp_csv, "w");

    // Fail if file cannot be opened
    if (file_auth == NULL) {
        // DEBUGGING
        printf("[%s][KICK USER] Error: Unable to open auth file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open auth file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop through username and role
    int id; char namecheck[MAX_BUFFER], role[8];
    int found = 0;

    while (fscanf(file_auth, "%d,%[^,],%s", &id, namecheck, role) == 3) {
        // DEBUGGING
        printf("[%s][KICK USER] name: %s, role: %s\n", client->username, namecheck, role);

        // Check if username matches
        if (strcmp(namecheck, username) == 0) {
            // DEBUGGING
            printf("[%s][KICK USER] Success: Username found\n", client->username);
            found = 1;
        } else {
            // Write old role to temp file
            fprintf(temp, "%d,%s,%s\n", id, namecheck, role);
        }
    }

    // Close files
    fclose(file_auth);
    fclose(temp);

    // Fail if username does not match
    if (found == 0) {
        // DEBUGGING
        printf("[%s][KICK USER] Error: Username not found\n", client->username);

        // Remove temp file
        remove(temp_csv);

        // Send response to client
        sprintf(response, "MSG,Error: Username not found");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Remove old file and rename temp file
    remove(path_auth);
    rename(temp_csv, path_auth);

    // DEBUGGING
    printf("[%s][KICK USER] Success: %s kicked\n", client->username, username);

    // Write to log
    char message[MAX_BUFFER];
    sprintf(message, "%s kicked user %s from %s\n", client->username, username, client->channel);
    write_log(client->channel, message);

    // Send response to client
    sprintf(response, "MSG,Success: %s kicked", username);
    send(client_fd, response, strlen(response), 0);
    return;
}

//=========================//
// UNBAN USER FROM CHANNEL //
//=========================//

void unban_user(char *username, client_data *client) {
    int client_fd = client->socket_fd;

    // DEBUGGING
    printf("[%s][UNBAN USER] username: %s\n", client->username, username);

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is in a channel
    if (strlen(client->channel) == 0) {
        // DEBUGGING
        printf("[%s][UNBAN USER] Error: User is not in a channel\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not in a channel");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Check permissions
    int perms = check_channel_perms(client->channel, client);
    if (perms == -1) {
        // DEBUGGING
        printf("[%s][UNBAN USER] Error: Unable to check permissions\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to check permissions");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 0) {
        // DEBUGGING
        printf("[%s][UNBAN USER] Error: User is not privileged\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not privileged");
        send(client_fd, response, strlen(response), 0);
        return;
    } else if (perms == 1) {
        // DEBUGGING
        printf("[%s][UNBAN USER] User is privileged\n", client->username);
    }

    // Open auth of channel
    char path_auth[MAX_BUFFER];
    sprintf(path_auth, "%s/%s/admin/auth.csv", cwd, client->channel);
    FILE *file_auth = fopen(path_auth, "r");

    // Open temp file
    char temp_csv[MAX_BUFFER];
    sprintf(temp_csv, "%s/%s/admin/.temp_auth.csv", cwd, client->channel);
    FILE *temp = fopen(temp_csv, "w");

    // Fail if file cannot be opened
    if (file_auth == NULL) {
        // DEBUGGING
        printf("[%s][UNBAN USER] Error: Unable to open auth file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open auth file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop through username and role
    int id; char namecheck[MAX_BUFFER], role[8];
    int found = 0;
    while (fscanf(file_auth, "%d,%[^,],%s", &id, namecheck, role) == 3) {
        // DEBUGGING
        printf("[%s][UNBAN USER] name: %s, role: %s\n", client->username, namecheck, role);

        // Check if username matches
        if (strcmp(namecheck, username) == 0) {
            // DEBUGGING
            printf("[%s][UNBAN USER] Success: Username found\n", client->username);

            // Check if role is not root/admin
            if (strcmp(role, "ROOT") == 0 || strcmp(role, "ADMIN") == 0) {
                // DEBUGGING
                printf("[%s][UNBAN USER] Error: User is root/admin\n", client->username);

                // Close files
                fclose(file_auth);
                fclose(temp);

                // Remove temp file
                remove(temp_csv);

                // Send response to client
                sprintf(response, "MSG,Error: User is root/admin");
                send(client_fd, response, strlen(response), 0);
                return;
            }

            // Write new role to temp file
            fprintf(temp, "%d,%s,USER\n", id, namecheck);
            found = 1;
        } else {
            // Write old role to temp file
            fprintf(temp, "%d,%s,%s\n", id, namecheck, role);
        }
    }

    // Close files
    fclose(file_auth);
    fclose(temp);

    // Fail if username does not match
    if (found == 0) {
        // DEBUGGING
        printf("[%s][UNBAN USER] Error: Username not found\n", client->username);

        // Remove temp file
        remove(temp_csv);

        // Send response to client
        sprintf(response, "MSG,Error: Username not found");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Remove old file and rename temp file
    remove(path_auth);
    rename(temp_csv, path_auth);

    // DEBUGGING
    printf("[%s][UNBAN USER] Success: %s unbanned\n", client->username, username);

    // Write to log
    char message[MAX_BUFFER];
    sprintf(message, "%s unbanned user %s from \"%s\"\n", client->username, username, client->channel);

    // Send response to client
    sprintf(response, "MSG,Success: %s unbanned", username);
    send(client_fd, response, strlen(response), 0);
    return;
}

//===========================================================================================//
//------------------------------------------- CHAT ------------------------------------------//
//===========================================================================================//

//================//
// SEND CHAT DATA //
//================//

void send_chat(char *message, client_data *client) {
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_CHAT];

    // Check if user is in a room
    if (strlen(client->room) == 0) {
        // DEBUGGING
        printf("[%s][SEND CHAT] Error: User is not in a room\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not in a room");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Prepare chat path
    char path_chat[MAX_BUFFER];
    sprintf(path_chat, "%s/%s/%s/chat.csv", cwd, client->channel, client->room);

    // Open chat file
    FILE *file = fopen(path_chat, "a+");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][SEND CHAT] Error: Unable to open file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Loop through csv to get ID
    int id = 0;
    while (fscanf(file, "%*[^,],%d,%*[^,],%*[^\n]", &id) == 1) {
        // DEBUGGING
        printf("[%s][SEND CHAT] id: %d\n", client->username, id);
    }

    // Get timestamp
    char *timestamp = get_timestamp();
    fprintf(file, "%s,%d,%s,%s\n", timestamp, id+1, client->username, message);

    // Close file
    fclose(file);

    // DEBUGGING
    printf("[%s][SEND CHAT] Success: Chat sent\n", client->username);

    // Send response to client
    sprintf(response, "MSG,[%s][%d][%s][SENT]", timestamp, id+1, client->username);
    send(client_fd, response, strlen(response), 0);
    return;
}

//===============//
// SEE CHAT DATA //
//===============//

void see_chat(client_data *client) {
    int client_fd = client->socket_fd;

    // Prepare larger response for chat
    char chat[MAX_BUFFER+256];
    char response[MAX_CHAT];
    sprintf(response, "MSG,");

    // Check if user is in a room
    if (strlen(client->room) == 0) {
        // DEBUGGING
        printf("[%s][SEE CHAT] Error: User is not in a room\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not in a room");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Prepare chat path
    char path_chat[MAX_BUFFER];
    sprintf(path_chat, "%s/%s/%s/chat.csv", cwd, client->channel, client->room);

    // Open chat file
    FILE *file = fopen(path_chat, "r");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][SEE CHAT] Error: Unable to open file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Check if chat is empty
    fseek(file, 0, SEEK_END);
    if (ftell(file) == 0) {
        // DEBUGGING
        printf("[%s][SEE CHAT] Error: Chat is empty\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Chat is empty");
        send(client_fd, response, strlen(response), 0);
        return;
    }
    fseek(file, 0, SEEK_SET);

    // Loop through csv to get messages
    int id; char timestamp[20], username[100], message[MAX_BUFFER];
    while (fscanf(file, " %[^,],%d,%[^,],%[^\n]",
           timestamp, &id, username, message) == 4) {
        // DEBUGGING
        printf("[%s][SEE CHAT] [%s][%d][%s] %s\n", client->username, timestamp, id, username, message);

        // Prepare chat send
        snprintf(chat, sizeof(chat), "[%s][%d][%s] %s", timestamp, id, username, message);

        // Break if response is too large
        if (strlen(response) + strlen(chat) + 2 > sizeof(response)) break;

        // Concatenate response with chat
        if (strlen(response) > 4) strcat(response, "\n");
        strcat(response, chat);
    }

    // Close file
    fclose(file);

    // Send response to client
    send(client_fd, response, strlen(response), 0);
    return;
}

//================//
// EDIT CHAT DATA //
//================//

void edit_chat(int edit, char *message, client_data *client){
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is in a room
    if (strlen(client->room) == 0) {
        // DEBUGGING
        printf("[%s][EDIT CHAT] Error: User is not in a room\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not in a room");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Prepare chat path
    char path_chat[MAX_BUFFER];
    sprintf(path_chat, "%s/%s/%s/chat.csv", cwd, client->channel, client->room);
    FILE *file = fopen(path_chat, "r");

    // Prepare temp path
    char path_temp[MAX_BUFFER];
    sprintf(path_temp, "%s/%s/%s/.chat_temp.csv", cwd, client->channel, client->room);
    FILE *file_temp = fopen(path_temp, "w");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][EDIT CHAT] Error: Unable to open file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // DEBUGGING
    printf("[%s][EDIT CHAT] id: %d, message: %s\n", client->username, edit);

    // Loop until id matches
    int id; char timestamp[20], username[100], message_old[MAX_BUFFER];
    char buffer[MAX_CHAT];
    int found = 0;
    while (fgets(buffer, MAX_BUFFER, file) != NULL) {
        // Get data from buffer
        sscanf(buffer, "%[^,],%d,%[^,],%[^\n]", timestamp, &id, username, message_old);

        // DEBUGGING
        printf("[%s][EDIT CHAT] id: %d, username: %s, message: %s\n", client->username, id, username, message_old);

        // Edit if id matches
        if (id == edit){
            // Check if user is not admin/root
            if (strcmp(client->role, "USER") == 0)
            // Check if user is not the author
            if (strcmp(client->username, username) != 0) {
                // DEBUGGING
                printf("[%s][EDIT CHAT] Error: User is not admin/root\n", client->username);

                // Close files
                fclose(file);
                fclose(file_temp);

                // Unlink temp file
                remove(path_temp);

                // Send response to client
                sprintf(response, "MSG,Error: User is not admin/root");
                send(client_fd, response, strlen(response), 0);
                return;
            }

            found = 1;
            fprintf(file_temp, "%s,%d,%s,%s\n", timestamp, id, username, message);
            continue;
        }

        // Write to temp file
        fprintf(file_temp, "%s", buffer);
    }

    // Close files
    fclose(file);
    fclose(file_temp);

    // Check if id is not found
    if (found == 0) {
        // DEBUGGING
        printf("[%s][EDIT CHAT] Error: ID not found\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: ID not found");
        send(client_fd, response, strlen(response), 0);
        return;
    } 

    // DEBUGGING
    printf("[%s][EDIT CHAT] Success: ID edited\n", client->username);

    // Remove original file
    remove(path_chat);

    // Rename temp file
    rename(path_temp, path_chat);

    // Write to log
    char log[MAX_BUFFER];
    sprintf(log, "%s edited chat id %d in \"%s\": %s\n", client->username, edit, client->room, message);
    write_log(client->channel, log);

    // Send response to client
    sprintf(response, "MSG,Success: ID edited");
    send(client_fd, response, strlen(response), 0);
    return;
}

//================//
// DELETE CHAT ID //
//================//

void del_chat(int target, client_data *client) {
    int client_fd = client->socket_fd;

    // Prepare response
    char response[MAX_BUFFER];

    // Check if user is in a room
    if (strlen(client->room) == 0) {
        // DEBUGGING
        printf("[%s][DEL CHAT] Error: User is not in a room\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: User is not in a room");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Prepare chat path
    char path_chat[MAX_BUFFER];
    sprintf(path_chat, "%s/%s/%s/chat.csv", cwd, client->channel, client->room);
    FILE *file = fopen(path_chat, "r");

    // Prepare temp path
    char path_temp[MAX_BUFFER];
    sprintf(path_temp, "%s/%s/%s/.chat_temp.csv", cwd, client->channel, client->room);
    FILE *file_temp = fopen(path_temp, "w");

    // Fail if file cannot be opened
    if (file == NULL) {
        // DEBUGGING
        printf("[%s][DEL CHAT] Error: Unable to open file\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: Unable to open file");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // DEBUGGING
    printf("[%s][DEL CHAT] id: %d\n", client->username, target);

    // Loop until id matches
    int id; char username[MAX_BUFFER], message[MAX_BUFFER];
    char buffer[MAX_CHAT];
    int found = 0;
    while (fgets(buffer, MAX_BUFFER, file) != NULL) {
        // Get data from buffer
        sscanf(buffer, "%*[^,],%d,%[^,],%[^\n]", &id, username, message);

        // DEBUGGING
        printf("[%s][DEL CHAT] id: %d, username: %s\n", client->username, id, username);

        // Skip if id matches
        if (id == target){
            // Check if user is not admin/root
            if (strcmp(client->role, "USER") == 0)
            // Check if user is not the author
            if (strcmp(client->username, username) != 0) {
                // DEBUGGING
                printf("[%s][DEL CHAT] Error: User is not admin/root\n", client->username);

                // Close files
                fclose(file);
                fclose(file_temp);

                // Unlink temp file
                remove(path_temp);

                // Send response to client
                sprintf(response, "MSG,Error: User is not admin/root");
                send(client_fd, response, strlen(response), 0);
                return;
            }

            found = 1;
            continue;
        }

        // Write to temp file
        fprintf(file_temp, "%s", buffer);
    }

    // Close files
    fclose(file);
    fclose(file_temp);

    // Check if id is not found
    if (found == 0) {
        // DEBUGGING
        printf("[%s][DEL CHAT] Error: ID not found\n", client->username);

        // Send response to client
        sprintf(response, "MSG,Error: ID not found");
        send(client_fd, response, strlen(response), 0);
        return;
    } 

    // DEBUGGING
    printf("[%s][DEL CHAT] Success: ID deleted\n", client->username);

    // Remove original file
    remove(path_chat);

    // Rename temp file
    rename(path_temp, path_chat);

    // Write to log
    char log[MAX_BUFFER];
    sprintf(log, "%s deleted chat id %d in \"%s\": \"%s\"\n", client->username, target, client->room, message);
    write_log(client->channel, log);

    // Send response to client
    sprintf(response, "MSG,Success: ID deleted");
    send(client_fd, response, strlen(response), 0);
    return;
}

//===========================================================================================//
//------------------------------------------- MISC ------------------------------------------//
//===========================================================================================//

//================//
// MAKE DIRECTORY //
//================//

void make_directory(char *path) {
    // Create directory
    if (mkdir(path, 0777) == -1)
    // Fail if directory cannot be created
    if (errno != EEXIST) {
        printf("[MKDIR] Error: Unable to create directory\n");
        return;
    }
}

//==================//
// RENAME DIRECTORY //
//==================//

void rename_directory(char *path, char *newpath) {
    // Create fork
    int pid = fork();
    // Return if fork fails
    if (pid < 0) {
        printf("[RENAME] Error: Unable to fork\n");
        return;
    }

    // Execute mv
    if (pid == 0) {
        execlp("mv", "mv", path, newpath, NULL);
    }

    // Wait for child process
    wait(NULL);
    return;
}

//==================//
// REMOVE DIRECTORY //
//==================//

void remove_directory(char *path) {
    // Create fork
    int pid = fork();
    // Return if fork fails
    if (pid < 0) {
        printf("[RMDIR] Error: Unable to fork\n");
        return;
    }

    // Execute rm -rf
    if (pid == 0) {
        execlp("rm", "rm", "-rf", path, NULL);
    }

    // Wait for child process
    wait(NULL);
    return;
}

//===============//
// GET TIMESTAMP //
//===============//

char* get_timestamp() {
    time_t rawtime;
    struct tm *timeinfo;
    char *timestamp = (char *)malloc(sizeof(char) * MAX_BUFFER);

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timestamp, MAX_BUFFER, "%d/%m/%Y %H:%M:%S", timeinfo);

    return timestamp;
}

//==============//
// WRITE TO LOG //
//==============//

void write_log(char* channel, char* message) {
    // Prepare log path
    char path_log[MAX_BUFFER];
    sprintf(path_log, "%s/%s/admin/log.csv", cwd, channel);

    // Open log file
    FILE *file = fopen(path_log, "a+");

    // Fail if file cannot be opened
    if (file == NULL) {
        printf("[LOG] Error: Unable to open file\n");
        return;
    }

    // Get timestamp
    char *timestamp = get_timestamp();

    // Write to log file
    fprintf(file, "[%s] %s", timestamp, message);

    // Close file
    fclose(file);
    return;
}