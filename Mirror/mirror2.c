#define _XOPEN_SOURCE 500 // Required for nftw function

#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ftw.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>

#define PORT 5050

#define MIRROR1_PORT 5051
#define MIRROR2_PORT 5052
#define BUFFER_SIZE 1024
#define TEMP_DIRECTORY_PREMISSIONS 0700
#define DEFAULT_UMASK 0022
#define NEW_UMASK 0000
#define DEFAULT_PERMISSIONS_FILE 0666

// Global variables
int sizeLessThan = 0;
bool zipOption = false;
char **filenames = NULL;
int sizeGreaterThan = 0;

int numFiles = 0;
bool isDirListOption = false;
bool isDirListAlpha = false;
bool isDirListTime = false;
bool isSizeOption = false;
bool isLessThanDateOption = false;
const char *tempNumberOfConnectionsFile = "/var/tmp/110128863numberOfConnections.txt";
bool isGreaterThanDateOption = false;
bool isExtensionOption = false;
char *extension[3];
char *date = NULL;
char fileDetailsString[1500];

int countNumberOfConnections;
// Function prototypes

void createAndWriteTempFile(int count);
void readTempFile();
int saveFileNamesInArray(const char *fpath);
int copyFile(const char *srcPath, const char *destPath);
static int nftwGetFileInfo(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf);
void fetchDirNamesFromPath(int socketId);
void fetchDirNamesFromTime(int socketId);
int compressFiles(const char *destDir);
void sendTarFileToClient(int client_socket);
void crequest(int client_socket);

// Structure to store the server address information
typedef struct
{
    char ip_address[INET_ADDRSTRLEN];
    int port_number;
} server_address_info;

// Function to create and Write temporary file
void createAndWriteTempFile(int count)
{
    int fd = open(tempNumberOfConnectionsFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }

    char buffer[20]; // Increased buffer size to handle larger numbers
    snprintf(buffer, sizeof(buffer), "%d", count);
    if (write(fd, buffer, strlen(buffer)) < 0)
    {
        perror("write");
        exit(EXIT_FAILURE);
    }

    close(fd);
}

// Function to read the temporary file
void readTempFile()
{
    char buffer[20]; // Increased buffer size to handle larger numbers
    int fd = open(tempNumberOfConnectionsFile, O_RDONLY);
    if (fd == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }
    int bytesRead = read(fd, buffer, sizeof(buffer) - 1);
    if (bytesRead < 0)
    {
        perror("read");
        exit(EXIT_FAILURE);
    }
    buffer[bytesRead] = '\0'; // Null-terminate the string
    countNumberOfConnections = atoi(buffer);
    close(fd);
}

// Function to save the filenames in an array
int saveFileNamesInArray(const char *fpath)
{
    // Reallocate memory for the filenames array
    filenames = realloc(filenames, (numFiles + 1) * sizeof(char *));
    // Check if the memory was allocated successfully
    if (filenames == NULL)
    {
        // Print an error message and exit
        perror("realloc");
        exit(EXIT_FAILURE);
    }
    // Allocates memory for a new filename (fpath) and copies the contents of fpath to the newly allocated memory.
    filenames[numFiles] = strdup(fpath);
    // Check if the memory was allocated successfully
    if (filenames[numFiles] == NULL)
    {
        perror("strdup");
        exit(EXIT_FAILURE);
    }
    // Increment the number of files
    numFiles++;
    return EXIT_SUCCESS;
}

// Function to copy a file
int copyFile(const char *srcPath, const char *destPath)
{
    // Set the umask to 0000
    umask(NEW_UMASK);
    // Open the source file for reading
    int sourceFile = open(srcPath, O_RDONLY);
    if (sourceFile == -1)
    {
        perror("Failed to open source file for reading");
        return -1;
    }
    // Check if the destination file already exists
    if (access(destPath, F_OK) != -1 && zipOption == true)
    {
        return EXIT_SUCCESS;
    }
    // Open the destination file for writing
    int destFile = open(destPath, O_CREAT | O_RDWR, DEFAULT_PERMISSIONS_FILE);
    if (destFile == -1)
    {
        perror("Failed to open destination file for writing");
        close(sourceFile);
        return -1;
    }
    // Copy the contents of the source file to the destination file
    char buffer[1024];
    long int bytesRead;
    // Read from the source file and write to the destination file
    while ((bytesRead = read(sourceFile, buffer, sizeof(buffer))) > 0)
    {
        // Write to the destination file
        long int bytesWritten = write(destFile, buffer, bytesRead);
        if (bytesWritten != bytesRead)
        {
            perror("Error writing to destination file");
            close(sourceFile);
            close(destFile);
            return -1;
        }
    }
    // Check if an error occurred while reading from the source file
    if (bytesRead == -1)
    {
        perror("Error reading from source file");
        close(sourceFile);
        close(destFile);
        return -1;
    }
    // Close the source and destination files
    close(sourceFile);
    close(destFile);
    // Set the umask back to the default value
    umask(DEFAULT_UMASK);
    return EXIT_SUCCESS;
}

// Comparison function for qsort
int compare(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}

// Function to check the date format
int checkDate(char *givenDate, char *fileDate)
{
    const char *dateFormat = "%Y-%m-%d";

    // Convert givenDate to time_t
    struct tm givenDateStruct = {0};
    strptime(givenDate, dateFormat, &givenDateStruct);
    time_t givenDateInSeconds = mktime(&givenDateStruct);

    // Convert fileDate to time_t
    struct tm fileDateStruct = {0};
    strptime(fileDate, dateFormat, &fileDateStruct);
    time_t fileDateInSeconds = mktime(&fileDateStruct);

    // Calculate the difference in days
    double difference = difftime(givenDateInSeconds, fileDateInSeconds);
    int daysDifference = difference / (60 * 60 * 24);

    return daysDifference;
}

// Function to check if the file extension is in the list of extensions
bool isFileExtensionInExtensions(const char *fpath)
{

    char *fileExtension = strrchr(fpath, '.');
    if (fileExtension != NULL)
    {
        int i = 0;
        while (extension[i] != NULL)
        {

            if (strcmp(fileExtension, extension[i]) == 0)
            {
                return true;
            }
            i++;
        }
    }
    return false;
}

// Function to compress files in a directory by Time
void fetchDirNamesFromTime(int socketId)
{
    FILE *fp;
    char path[1035];
    char command[256];
    char *home_directory = getenv("HOME");

    // Construct the find command to list directories under the home directory
    sprintf(command, "find %s -mindepth 1 -maxdepth 2 -type d ! -path '*/.*' -printf '%%T@ %%p\\n' | sort -n -k1 | cut -d' ' -f2-", home_directory);

    // Open the command for reading
    fp = popen(command, "r");
    if (fp == NULL)
    {
        perror("popen");
        exit(EXIT_FAILURE);
    }

    // Read the output of the command and send it to the client
    while (fgets(path, sizeof(path), fp) != NULL)
    {
        // Remove the trailing newline character
        path[strcspn(path, "\n")] = '\0';
        if (send(socketId, path, strlen(path), 0) == -1)
        {
            perror("send");
            break;
        }
        if (send(socketId, "\n", 1, 0) == -1)
        {
            perror("send");
            break;
        }
    }

    // Close the file pointer
    pclose(fp);

    // Send the completion signal
    if (send(socketId, "COMPLETED_", strlen("COMPLETED_"), 0) == -1)
    {
        perror("send");
    }
}

// Function to compress files in a directory by Name
void fetchDirNamesFromPath(int socketId)
{
    FILE *fp;
    char path[1035];
    char command[256];
    char *home_directory = getenv("HOME");

    // Construct the find command to list directories under the home directory
    sprintf(command, "find %s -mindepth 1 -maxdepth 2 -type d ! -path '*/.*'  | sort -f", home_directory);

    // Open the command for reading
    fp = popen(command, "r");
    if (fp == NULL)
    {
        perror("popen");
        exit(EXIT_FAILURE);
    }

    // Read the output of the command and send it to the client
    while (fgets(path, sizeof(path), fp) != NULL)
    {
        // Remove the trailing newline character
        path[strcspn(path, "\n")] = '\0';
        if (send(socketId, path, strlen(path), 0) == -1)
        {
            perror("send");
            break;
        }
        if (send(socketId, "\n", 1, 0) == -1)
        {
            perror("send");
            break;
        }
    }

    // Close the file pointer
    pclose(fp);

    // Send the completion signal
    if (send(socketId, "COMPLETED_", strlen("COMPLETED_"), 0) == -1)
    {
        perror("send");
    }
}

// Function to compress files in a directory
static int nftwGetFileInfo(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf)
{
    if (tflag == FTW_F)
    {
        // Check if the file meets the specified criteria
        if (isSizeOption && sb->st_size >= sizeGreaterThan && sb->st_size <= sizeLessThan)
        {

            saveFileNamesInArray(fpath);
            printf("%s\n", filenames[numFiles - 1]);

            return EXIT_SUCCESS;
        }
        else if (isExtensionOption && isFileExtensionInExtensions(fpath + ftwbuf->base))
        {
            saveFileNamesInArray(fpath);
        }
        else if (isLessThanDateOption)
        {
            struct tm *timeinfo;
            timeinfo = localtime(&sb->st_mtime);
            char dateStr[11];
            strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", timeinfo);
            int daysDifference = checkDate(date, dateStr);
            if (daysDifference >= 0)
            {
                saveFileNamesInArray(fpath);
            }
        }
        else if (isGreaterThanDateOption)
        {
            struct tm *timeinfo;
            timeinfo = localtime(&sb->st_mtime);
            char dateStr[11];
            strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", timeinfo);
            int daysDifference = checkDate(date, dateStr);
            if (daysDifference <= 0)
            {
                saveFileNamesInArray(fpath);
            }
        }
    }
    return 0; // Continue traversal
}

// Function to compress files in a directory
void getFileDetails(const char *filename)
{
    // Find the first occurrence of the file and get its path
    char find_command[256];
    snprintf(find_command, sizeof(find_command), "find ~ -name %s | head -n 1", filename);

    FILE *find_fp = popen(find_command, "r");
    if (find_fp == NULL)
    {
        perror("Failed to execute find command");
        return;
    }

    char file_path[256];
    if (fgets(file_path, sizeof(file_path), find_fp) == NULL)
    {
        sprintf(fileDetailsString, "%s", "File not found\n");
        printf("File not found\n");
        pclose(find_fp);
        return;
    }
    pclose(find_fp);

    // Get the file details using ls command
    char ls_command[256];
    snprintf(ls_command, sizeof(ls_command), "ls -l --time-style=long-iso %s", file_path);

    FILE *ls_fp = popen(ls_command, "r");
    if (ls_fp == NULL)
    {
        perror("Failed to execute ls command");
        return;
    }

    char line[256];
    if (fgets(line, sizeof(line), ls_fp) != NULL)
    {
        char permissions[10], size[20], date_created[20], file_name[256];
        sscanf(line, "%s %*d %*s %*s %s %s %s %s", permissions, size, date_created, file_name);
        printf("File path: %s\n", file_path);
        printf("Permissions: %s\n", permissions);
        printf("Size: %s\n", size);
        printf("Date created: %s\n", date_created);
        sprintf(fileDetailsString, "%s;%s;%s;%s", file_path, size, date_created, permissions);
    }
    else
    {
        printf("Failed to get file details\n");
    }

    pclose(ls_fp);
}

// Function to create a tar file
int createTarFile()
{
    // Calculate the total length of the command
    if (numFiles == 0)
    {
        printf("No files found\n");
        int fd = open("temp.tar.gz", O_CREAT | O_RDWR, DEFAULT_PERMISSIONS_FILE);
        close(fd);
        return 0;
    }

    size_t total_length = strlen("tar -czf ./temp.tar.gz ") + 1; // +1 for null terminator
    for (int i = 0; i < numFiles; i++)
    {
        total_length += strlen(filenames[i]) + 1; // +1 for space separator
    }

    // Allocate memory for the command
    char *command = (char *)malloc(total_length);
    if (command == NULL)
    {
        fprintf(stderr, "Failed to allocate memory\n");
        return 1;
    }

    // Initialize the command string
    strcpy(command, "tar -czf ./temp.tar.gz --files-from=/var/tmp/110128863zipFileList");
    int fd = open("/var/tmp/110128863zipFileList", O_CREAT | O_RDWR, 0666);
    if (fd == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }
    // Loop through the filenames and append them to the command
    for (int i = 0; i < numFiles; i++)
    {
       write(fd, filenames[i], strlen(filenames[i]));
       write(fd, "\n", 1);
    }
    close(fd);
    printf("Command: %s\n", command);

    pid_t pid = fork();
    if (pid == -1)
    {
        fprintf(stderr, "Failed to fork\n");
        return EXIT_FAILURE;
    }
    else if (pid == 0)
    {
        // Child process
        char *args[] = {"bash", "-c", command, NULL};
        execvp(args[0], args);
        fprintf(stderr, "Failed to execute command\n");
        return EXIT_FAILURE;
    }
    else
    {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        {
            // Command executed successfully
            // Continue with the rest of the code
        }
        else
        {
            fprintf(stderr, "Failed to execute command\n");
            return EXIT_FAILURE;
        }
    }

    // Free the allocated memory
    free(command);
    unlink("/var/tmp/110128863zipFileList");
    return EXIT_SUCCESS;
}

// Function to send the tar file to the client
void sendTarFileToClient(int client_socket)
{
    printf("Sending tar file to client\n");
    char tarPath[strlen(getcwd(NULL, 0)) + 15];
    sprintf(tarPath, "%s/temp.tar.gz", getcwd(NULL, 0));
    FILE *file = fopen(tarPath, "rb");
    if (!file)
    {
        perror("Error opening file");
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    printf("File size: %d\n", file_size);
    // Send the file size first
    send(client_socket, &file_size, sizeof(file_size), 0);

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        send(client_socket, buffer, bytes_read, 0);
    }

    fclose(file);
}

// Function to handle the client request
void crequest(int client_socket)
{
    char buffer[BUFFER_SIZE] = {0};
    int valread;
    char *homePath = getenv("HOME");
    if (homePath == NULL)
    {
        perror("getenv");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        // Read data from the client
        valread = read(client_socket, buffer, BUFFER_SIZE);
        if (strstr(buffer, "w24fz") != NULL)
        {
            isSizeOption = true;
            char *token = strtok(buffer, " ");
            token = strtok(NULL, " ");
            sizeGreaterThan = atoi(token);
            token = strtok(NULL, " ");
            sizeLessThan = atoi(token);

            // Traverse the home directory and display the file paths
            if (nftw(homePath, nftwGetFileInfo, 20, FTW_PHYS) == -1)
            {
                perror("nftw");
                exit(EXIT_FAILURE);
            }

            createTarFile();

            sendTarFileToClient(client_socket);
            // Remove the temporary directory
            unlink("temp.tar.gz");
            memset(buffer, 0, sizeof(buffer)); // Clear the buffer
            filenames = NULL;
            numFiles = 0;
            isSizeOption = false;
            continue;
        }
        else if (strstr(buffer, "w24ft") != NULL)
        {
            isExtensionOption = true;
            char *token = strtok(buffer, " ");
            token = strtok(NULL, " ");
            int i = 0;
            while (token != NULL)
            {
                extension[i] = malloc(strlen(token) + 2); // Allocate memory for extension[i]
                if (extension[i] == NULL)
                {
                    fprintf(stderr, "Failed to allocate memory\n");
                    exit(EXIT_FAILURE);
                }
                strcpy(extension[i], ".");
                strcat(extension[i], token);
                printf("Extension %d: %s\n", i + 1, extension[i]);
                token = strtok(NULL, " ");
                i++;
            }

            if (nftw(homePath, nftwGetFileInfo, 20, FTW_PHYS) == -1)
            {
                perror("nftw");
                exit(EXIT_FAILURE);
            }
            createTarFile();

            sendTarFileToClient(client_socket);
            // Remove the temporary directory
            unlink("temp.tar.gz");
            filenames = NULL;
            numFiles = 0;
            isExtensionOption = false;
            for(int i = 0; i < 3; i++)
            {
                extension[i] = NULL;
            }
            memset(buffer, 0, sizeof(buffer));
            continue;
        }
        else if (strstr(buffer, "w24fdb") != NULL)
        {
            printf("Buffer: %s\n", buffer);

            char *token = strtok(buffer, " ");
            token = strtok(NULL, " ");
            date = token;
            isLessThanDateOption = true;

            if (nftw(homePath, nftwGetFileInfo, 20, FTW_PHYS) == -1)
            {
                perror("nftw");
                exit(EXIT_FAILURE);
            }
            createTarFile();
            printf("---->Tar file created\n");

            sendTarFileToClient(client_socket);
            // Remove the temporary directory
            unlink("temp.tar.gz");
            filenames = NULL;
            numFiles = 0;
            isLessThanDateOption = false;
            memset(buffer, 0, sizeof(buffer));
            continue;
        }
        else if (strstr(buffer, "w24fda") != NULL)
        {
            char *token = strtok(buffer, " ");
            token = strtok(NULL, " ");
            date = token;
            isGreaterThanDateOption = true;
            if (nftw(homePath, nftwGetFileInfo, 20, FTW_PHYS) == -1)
            {
                perror("nftw");
                exit(EXIT_FAILURE);
            }
            createTarFile();

            sendTarFileToClient(client_socket);
            // Remove the temporary directory
            unlink("temp.tar.gz");
            memset(buffer, 0, sizeof(buffer));
            filenames = NULL;
            numFiles = 0;
            isGreaterThanDateOption = false;
            continue;
        }
        else if (strstr(buffer, "w24fn") != NULL)
        {
            char *token = strtok(buffer, " ");
            token = strtok(NULL, " ");
            char *getDetailsOffile = token;
            getFileDetails(getDetailsOffile);
            send(client_socket, &fileDetailsString, strlen(fileDetailsString), 0);
            memset(buffer, 0, sizeof(buffer));
            memset(fileDetailsString, 0, sizeof(fileDetailsString));
            continue;
        }
        else if (strstr(buffer, "dirlist") != NULL)
        {
            isDirListOption = true;
            char *token = strtok(buffer, " ");
            token = strtok(NULL, " ");
            char *dirOperation = token;
            if (strcmp(dirOperation, "-a") == 0)
            {
                fetchDirNamesFromPath(client_socket);
            }
            else if (strcmp(dirOperation, "-t") == 0)
            {
                fetchDirNamesFromTime(client_socket);
            }
            memset(buffer, 0, sizeof(buffer));
            continue;
        }

        // Check for quitc command
        if (strcmp(buffer, "quitc") == 0)
        {
            readTempFile();
            printf("Current number of connections: %d\n", countNumberOfConnections);
            countNumberOfConnections--;
            createAndWriteTempFile(countNumberOfConnections);
            break;
        }
    }

    close(client_socket);
}

int main()
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(MIRROR2_PORT);

    // Bind the socket to the specified IP and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    

    printf("Server listening on port %d\n", MIRROR2_PORT);
    while (1)
    {
        // Accept incoming connections
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }
        // Get the client IP address
        printf("Connection Established with mirror\n");
        // Fork a child process to handle the client request
        if (fork() == 0)
        {
            close(server_fd); // Close the server socket in the child process
            crequest(new_socket);
            exit(0);
        }
        else
        {
            close(new_socket); // Close the client socket in the parent process
        }
    }

    return 0;
}
