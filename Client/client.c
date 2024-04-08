#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_IP_LENGTH 16
#define BUFFER_SIZE 1024

void receive_file(int server_socket) {
    char filename[] = "temp.tar.gz";
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Error opening file");
        return;
    }

    long file_size;
    recv(server_socket, &file_size, sizeof(file_size), 0);
    printf("File size: %d\n", file_size);
    if(file_size == 0){
        printf("No file found\n");
        return;
    }

    char buffer[BUFFER_SIZE];
    size_t total_bytes_received = 0;
    while (total_bytes_received < file_size) {
        size_t bytes_received = recv(server_socket, buffer, sizeof(buffer), 0);
        fwrite(buffer, 1, bytes_received, file);
        total_bytes_received += bytes_received;
    }

    fclose(file);
}


int validateCommand(char *command) {
    // Check if the command starts with "w24ft"
    if (strncmp(command, "w24ft", 5) == 0) {
        
    // Count the number of extensions after "w24ft"
    int extensions = 0;
    char *token = strtok(command, " ");
    token = strtok(NULL, " "); // Skip the first token ("w24ft")
    while (token != NULL) {
        printf("%s\n", token);
        extensions++;
        token = strtok(NULL, " ");
    }

    // Check if there are between 1 and 3 extensions
    if (extensions < 1 || extensions > 3) {
        return 0; // Incorrect number of extensions
    }
    }else if(strncmp(command, "w24fz", 5) == 0){
        int size1, size2;
        char *extra;
        char *token = strtok(command, " ");
        token = strtok(NULL, " "); // Skip the first token ("w24fz")
        size1 = atoi(token);

        token = strtok(NULL, " "); // Get the second token
        size2 = atoi(token);

        token = strtok(NULL, " "); // Get the third token
        extra = token;
        if(extra != NULL){
            return 0;
        }


        printf("%d %d\n", size1, size2);

        if(size1 <= 0 || size2 <= 0 ){
            return 0;
        }
        else if(size1 > size2){
            return 0;
        }

    }

    return 1; // Command is valid
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <server IP address> <server port>\n", argv[0]);
        return 1;
    }

    char *serverIP = argv[1];
    int serverPort = atoi(argv[2]);

    struct in_addr addr;
    if (inet_pton(AF_INET, serverIP, &addr) != 1) {
        perror("Invalid IP address");
        return 1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(serverIP);
    server.sin_port = htons(serverPort);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Failed to connect to server");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server\n");

    char message[BUFFER_SIZE];
    while (true) {
        fflush(stdout);
        printf("Enter message to send: ");
        fgets(message, BUFFER_SIZE, stdin);
        // Remove newline character from the message
        message[strcspn(message, "\n")] = 0;

        char *CommandCopy = strdup(message);
        if (!validateCommand(CommandCopy)) {
            printf("Invalid command\n");
            continue;
        }
        
        
        if (send(sock, message, strlen(message), 0) < 0) {
            perror("Failed to send message");
            exit(EXIT_FAILURE);
        }

        if (strstr(message, "w24fz") != NULL) {
          
            receive_file(sock);
            continue;
        }
         if (strstr(message, "w24ft") != NULL) {

            receive_file(sock);
            continue;
        }

        char buffer[BUFFER_SIZE] = {0};
        ssize_t bytes_received;
        if ((bytes_received = recv(sock, buffer, BUFFER_SIZE, 0)) <= 0) {
            if (bytes_received == 0) {
                printf("Connection closed by peer\n");
            } else {
                perror("Failed to receive response");
            }
            exit(EXIT_FAILURE);
        }

        printf("Server response: %s\n", buffer);
    }

    close(sock);
    return 0;
}
