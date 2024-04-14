#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define MAX_IP_LENGTH 16
#define BUFFER_SIZE 1024


typedef struct
{
    char ip_address[MAX_IP_LENGTH];
    int port_number;
} mirrorInfo;

void receive_file(int server_socket)
{
    char filename[] = "temp.tar.gz";
    FILE *file = fopen(filename, "wb");
    if (!file)
    {
        perror("Error opening file");
        return;
    }

    long file_size;
    recv(server_socket, &file_size, sizeof(file_size), 0);
    printf("File size: %d\n", file_size);
    if (file_size == 0)
    {
        printf("No file found\n");
        return;
    }

    char buffer[BUFFER_SIZE];
    size_t total_bytes_received = 0;
    while (total_bytes_received < file_size)
    {
        size_t bytes_received = recv(server_socket, buffer, sizeof(buffer), 0);
        fwrite(buffer, 1, bytes_received, file);
        total_bytes_received += bytes_received;
    }

    fclose(file);
}

bool isValidDate(char *dateStr)
{
    int year, month, day;
    if (sscanf(dateStr, "%d-%d-%d", &year, &month, &day) != 3)
    {
        return false;
    }

    if (year < 1 || month < 1 || month > 12 || day < 1)
    {
        return false;
    }

    if ((month == 4 || month == 6 || month == 9 || month == 11) && (day > 30))
    {
        return false;
    }

    if (month == 2)
    {
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
        {
            if (day > 29)
            {
                return false;
            }
        }
        else
        {
            if (day > 28)
            {
                return false;
            }
        }
    }

    return true;
}
int isFutureDate(char *givenDate)
{
    const char *dateFormat = "%Y-%m-%d";

    // Get the current date
    time_t now = time(NULL);
    struct tm *currentDateStruct = localtime(&now);

    // Convert givenDate to time_t
    struct tm givenDateStruct = {0};
    strptime(givenDate, dateFormat, &givenDateStruct);
    time_t givenDateInSeconds = mktime(&givenDateStruct);

    // Compare givenDate with current date
    printf("Given date: %ld\n", givenDateInSeconds);
    printf("Current date: %ld\n", now);
    if (givenDateInSeconds > now)
    {
        return 1; // givenDate is a future date
    }
    else
    {
        return 0; // givenDate is not a future date
    }
}

int validateCommand(char *command)
{
    // Check if the command starts with "w24ft"
    if (strncmp(command, "w24ft", 5) == 0)
    {

        // Count the number of extensions after "w24ft"
        int extensions = 0;
        char *token = strtok(command, " ");
        token = strtok(NULL, " "); // Skip the first token ("w24ft")
        while (token != NULL)
        {
            printf("%s\n", token);
            extensions++;
            token = strtok(NULL, " ");
        }

        // Check if there are between 1 and 3 extensions
        if (extensions < 1 || extensions > 3)
        {
            return 0; // Incorrect number of extensions
        }
    }
    else if (strncmp(command, "w24fz", 5) == 0)
    {
        int size1, size2;
        char *extra;
        char *token = strtok(command, " ");
        token = strtok(NULL, " "); // Skip the first token ("w24fz")
        size1 = atoi(token);

        token = strtok(NULL, " "); // Get the second token
        size2 = atoi(token);

        token = strtok(NULL, " "); // Get the third token
        extra = token;
        if (extra != NULL)
        {
            return 0;
        }

        printf("%d %d\n", size1, size2);

        if (size1 <= 0 || size2 <= 0)
        {
            return 0;
        }
        else if (size1 > size2)
        {
            return 0;
        }
    }
    else if (strncmp(command, "w24fdb", 6) == 0)
    {
        char *date;
        char *extra;
        char *token = strtok(command, " ");
        printf("%s\n", token);
        token = strtok(NULL, " "); // Skip the first token ("w24fdb")
        date = token;
        printf("%s\n", date);
        token = strtok(NULL, " "); // Get the third token
        extra = token;
        if (extra != NULL)
        {
            printf("Invalid command format.\n");
            return 0;
        }

        struct tm tm;
        if (strptime(date, "%Y-%m-%d", &tm) == NULL)
        {
            printf("Invalid date format. Expected YYYY-MM-DD.\n");
            return 0;
        }

        // Convert the struct tm object into a time_t object
        time_t input_date = mktime(&tm);
        if (input_date == -1)
        {
            perror("Failed to convert date");
            return 0;
        }

        if (!isValidDate(date))
        {
            return 0;
        }

        if (isFutureDate(date))
        {
            return 0;
        }
    }

    else if (strncmp(command, "w24fda", 6) == 0)
    {
        char *date;
        char *extra;
        char *token = strtok(command, " ");
        printf("%s\n", token);
        token = strtok(NULL, " "); // Skip the first token ("w24fdb")
        date = token;
        printf("%s\n", date);
        token = strtok(NULL, " "); // Get the third token
        extra = token;
        if (extra != NULL)
        {
            printf("Invalid command format.\n");
            return 0;
        }

        struct tm tm;
        if (strptime(date, "%Y-%m-%d", &tm) == NULL)
        {
            printf("Invalid date format. Expected YYYY-MM-DD.\n");
            return 0;
        }

        // Convert the struct tm object into a time_t object
        time_t input_date = mktime(&tm);
        if (input_date == -1)
        {
            perror("Failed to convert date");
            return 0;
        }

        if (isFutureDate(date))
        {
            return 0;
        }
    }

    else if (strncmp(command, "w24fn", 5) == 0)
    {

        char *extra;
        char *token = strtok(command, " ");
        token = strtok(NULL, " "); // Skip the first token ("w24fn")
        char *fileName = token;
        token = strtok(NULL, " "); // Get the third token
        extra = token;
        if (extra != NULL)
        {
            printf("Invalid command format.\n");
            return 0;
        }
    } else if (strncmp(command, "dirlist -a", 10) == 0) {
        char *extra;
        char *token = strtok(command, " ");
        token = strtok(NULL, " "); // Skip the first token ("dirlist")
        token = strtok(NULL, " "); // Skip the second token ("-a")
        extra = token;
        if (extra != NULL)
        {
            printf("Invalid command format.\n");
            return 0;
        }
    } else if(strncmp(command, "dirlist -t", 10) == 0){
        char *extra;
        char *token = strtok(command, " ");
        token = strtok(NULL, " "); // Skip the first token ("dirlist")
        token = strtok(NULL, " "); // Skip the second token ("-t")
        extra = token;
        if (extra != NULL)
        {
            printf("Invalid command format.\n");
            return 0;
        }
    }
    else
    {
        return 0;
    }
    return 1; // Command is valid
}

int ConnectToMirrorServer(const char *mirrorIp, int mirrorPort)
{
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0)
    {
        printf("Cannot create client socket\n");
        exit(1);
    }

    struct sockaddr_in mirrorAddress;
    mirrorAddress.sin_family = AF_INET;
    mirrorAddress.sin_port = htons(mirrorPort);
    if (inet_pton(AF_INET, mirrorIp, &mirrorAddress.sin_addr) <= 0)
    {
        printf("Err: Wrong Mirror Address\n");
        exit(1);
    }

    if (connect(clientSocket, (struct sockaddr *)&mirrorAddress, sizeof(mirrorAddress)) < 0)
    {
        printf("Err: Cannot conect to mirror.\n");
        exit(1);
    }

    return clientSocket;
}

int main(int argc, char *argv[])
{
    char buffer[BUFFER_SIZE] = {0};
    if (argc != 3)
    {
        printf("Usage: %s <server IP address> <server port>\n", argv[0]);
        return 1;
    }

    char *serverIP = argv[1];
    int serverPort = atoi(argv[2]);

    struct in_addr addr;
    if (inet_pton(AF_INET, serverIP, &addr) != 1)
    {
        perror("Invalid IP address");
        return 1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(serverIP);
    server.sin_port = htons(serverPort);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Failed to connect to server");
        exit(EXIT_FAILURE);
    }
    long res = 0;
    recv(sock, &res, sizeof(res), 0);
    if (res == 1)
    {
        printf("Will be redirected to mirror.\n");
        mirrorInfo transferToMirror;
        recv(sock, &transferToMirror, sizeof(mirrorInfo), 0);
        close(sock);
        sock = ConnectToMirrorServer(transferToMirror.ip_address, transferToMirror.port_number);
        printf("Connected to mirror server now!\n");
    }
    else
    {
        printf("Connected to Server!\n");
    }

    char message[BUFFER_SIZE];
    while (true)
    {
        fflush(stdout);
        printf("Enter message to send: ");
        fgets(message, BUFFER_SIZE, stdin);
        // Remove newline character from the message
        message[strcspn(message, "\n")] = 0;

        char *CommandCopy = strdup(message);
        if (!validateCommand(CommandCopy))
        {
            printf("Invalid command\n");
            continue;
        }
        printf("Command: %s \n", message);
        if (send(sock, message, strlen(message), 0) < 0)
        {
            perror("Failed to send message");
            exit(EXIT_FAILURE);
        }

        if (strstr(message, "w24fz") != NULL)
        {

            receive_file(sock);
            continue;
        }
        if (strstr(message, "w24ft") != NULL)
        {

            receive_file(sock);
            continue;
        }
        if (strstr(message, "w24fdb") != NULL)
        {

            receive_file(sock);
            continue;
        }
        if (strstr(message, "w24fda") != NULL)
        {

            receive_file(sock);
            continue;
        }
        if (strstr(message, "w24fn") != NULL)
        {
            char fileDetailsString[1500]; // Assuming a maximum size for the string
            int bytesReceived = recv(sock, &fileDetailsString, sizeof(fileDetailsString), 0);
            if (bytesReceived < 0)
            {
                perror("recv failed");
                exit(EXIT_FAILURE);
            }
            if(bytesReceived == 0){
                printf("No file found\n");
                continue;
            }

            // Null-terminate the received string
            fileDetailsString[bytesReceived] = '\0';

            // Parse the received string to extract file details
            char *token = strtok(fileDetailsString, ";");
            if(strcmp(token, "File not found\n") == 0){
                printf("File not found\n");
                continue;
            }
            printf("File Path: %s", token);
            token = strtok(NULL, ";");
            printf("File Size: %s\n", token);
            token = strtok(NULL, ";");
            printf("File Created At: %s\n", token);
            token = strtok(NULL, ";");
            printf("File Permission: %s\n", token);

            continue;
        }
        if(strstr(message, "dirlist -a") != NULL){
            int valread;
            // Receiving directory list from server
            while ((valread = read(sock, buffer, BUFFER_SIZE - 1)) > 0) {
                buffer[valread] = '\0'; // Null-terminate the received data
                printf("%s", buffer);
            }

            if (valread < 0) {
                printf("Failed to receive directory list from server\n");
            } else if(valread == 0) {
                continue;
            }
            continue;
        }
        if(strstr(message, "dirlist -t") != NULL){
            int valread;
            // Receiving directory list from server
            while ((valread = read(sock, buffer, BUFFER_SIZE - 1)) > 0) {
                buffer[valread] = '\0'; // Null-terminate the received data
                printf("%s", buffer);
            }

            if (valread < 0) {
                printf("Failed to receive directory list from server\n");
            } else if(valread == 0) {
                continue;
            }
            continue;
        }
    }

    close(sock);
    return 0;
}
