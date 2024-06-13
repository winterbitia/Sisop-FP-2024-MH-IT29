#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_BUFFER 1028

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

//======//
// MAIN //
//======//
int main(int argc, char *argv[]) {
    // Check if arguments are valid
    if (argc < 5) {
        printf("Usage: ./discorit [REGISTER/LOGIN] <username> -p <password>", argv[0]);
        return 1;
    }

    // Register user
    if (strcmp(argv[1], "REGISTER") == 0) {
        if ((argc != 5)
          ||(strcmp(argv[3], "-p") != 0)){
            printf("Usage: ./discorit REGISTER <username> -p <password>");
            return 1;
        }

        char *username = argv[2];
        char *password = argv[4];
        register_user(username, password);
    }

    // Return success
    return 0;
}