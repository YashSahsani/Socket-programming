#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void crequest(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    char *hello = "Hello from server";
    int valread;

    while (1) {
        // Read data from the client
        valread = read(client_socket, buffer, BUFFER_SIZE);
        printf("Received: %s\n", buffer);

        // Check for quitc command
        if (strcmp(buffer, "quitc") == 0) {
            break;
        }

        // Perform action required to process the command (not specified in the description)
        // For now, just send a response to the client
        send(client_socket, hello, strlen(hello), 0);
        printf("Hello message sent\n");
    }

    close(client_socket);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the specified IP and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Accept incoming connections
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        // Fork a child process to handle the client request
        if (fork() == 0) {
            close(server_fd);  // Close the server socket in the child process
            crequest(new_socket);
            exit(0);
        } else {
            close(new_socket);  // Close the client socket in the parent process
        }
    }

    return 0;
}
