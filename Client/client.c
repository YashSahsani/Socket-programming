#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define MAX_IP_LENGTH 16
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <server IP address> <server port>\n", argv[0]);
        return 1;
    }

    char *serverIP = argv[1];
    int serverPort = atoi(argv[2]);

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
    while (true)
    {
    fflush(stdin);
    fflush(stdout);
    printf("Enter message to send: ");
    fgets(message, BUFFER_SIZE, stdin);
    // Remove newline character from the message
    message[strcspn(message, "\n")] = 0;

    if (send(sock, message, strlen(message), 0) < 0) {
        perror("Failed to send message");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE] = {0};
    if (recv(sock, buffer, BUFFER_SIZE, 0) < 0) {
        perror("Failed to receive response");
        exit(EXIT_FAILURE);
    }

    printf("Server response: %s\n", buffer);
    }

    close(sock);
    return 0;
}
