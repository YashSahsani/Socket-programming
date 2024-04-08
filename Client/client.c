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

    char buffer[BUFFER_SIZE];
    size_t total_bytes_received = 0;
    while (total_bytes_received < file_size) {
        size_t bytes_received = recv(server_socket, buffer, sizeof(buffer), 0);
        fwrite(buffer, 1, bytes_received, file);
        total_bytes_received += bytes_received;
    }

    fclose(file);
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
        
        if (send(sock, message, strlen(message), 0) < 0) {
            perror("Failed to send message");
            exit(EXIT_FAILURE);
        }

        if (strstr(message, "w24fz") != NULL) {
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
