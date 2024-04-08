#define _XOPEN_SOURCE 500 // Required for nftw function

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

#define PORT 5050

#define MIRROR1_PORT 8081
#define MIRROR2_PORT 8082
#define BUFFER_SIZE 1024
#define TEMP_DIRECTORY_PREMISSIONS 0700
#define DEFAULT_UMASK 0022
#define NEW_UMASK 0000
#define DEFAULT_PERMISSIONS_FILE 0666

int sizeLessThan = 0;
bool zipOption = false;
char **filenames = NULL;
int sizeGreaterThan = 0;
char *TempCopyDestination = "/var/tmp/testZip110128863";
int numFiles = 0;
bool isSizeOption = false;
bool isLessThanDateOption = false;
bool isGreaterThanDateOption = false;
bool isExtensionOption = false;
bool getFileDetails = false;
char *getDetailsOffile = NULL;
char *extension[3];
char *date = NULL;


int countNumberOfConnections = 0;
// Function prototypes
int createTempDirectory();
int removeTempDirectory();
int saveFileNamesInArray(const char *fpath);
int copyFile(const char *srcPath, const char *destPath);
static int display_info(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf);
int compressFiles(const char *destDir);
void sendTarFileToClient(int client_socket);
void crequest(int client_socket);


typedef struct FileDetails
{
    char *path;
    int size;
    char *createdAtdate;
    char *permission;

} FileDetails;

FileDetails fileDetails;
typedef struct
{
    char ip_address[INET_ADDRSTRLEN];
    int port_number;
} server_address_info;


// Rest of your code...
// create a temporary directory
int createTempDirectory()
{
    // Check if the directory already exists
    if (access(TempCopyDestination, F_OK) != -1)
    { // Directory already exists, no need to create
        return EXIT_SUCCESS;
    }
    // Create the directory via mkdir
    int status = mkdir(TempCopyDestination, TEMP_DIRECTORY_PREMISSIONS);
    // Check if the directory was created successfully
    if (status == -1)
    {
        // Print an error message and exit
        perror("mkdir");
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}

int removeTempDirectory()
{
    if (access(TempCopyDestination, F_OK) == -1)
    {
        return EXIT_SUCCESS;
    }
    // Remove the directory and all its contents
    char command[1000];
    // Create the command to remove the directory
    sprintf(command, "rm -r %s", TempCopyDestination);
    // Execute the command
    int status = system(command);
    // Check if the command was executed successfully
    if (status == -1)
    {
        perror("system");
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}
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
                printf("---------> Found extension %s\n", fpath);

                return true;
            }
            i++;
        }
    }
    return false;
}
static int display_info(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf)
{
    if (tflag == FTW_F)
    {

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
        else if (getFileDetails && strcmp(fpath + ftwbuf->base, getDetailsOffile) == 0) {
            fileDetails.path = fpath;
            fileDetails.size = sb->st_size;
            struct tm *timeinfo;
            timeinfo = localtime(&sb->st_mtime);
            char dateStr[11];
            strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", timeinfo);   
            fileDetails.createdAtdate = dateStr;
            fileDetails.permission = malloc(10);
            sprintf(fileDetails.permission, "%o", sb->st_mode & 0777);
            printf("File path: %s\n", fileDetails.path);
            printf("File size: %d\n", fileDetails.size);
            printf("File created at: %s\n", fileDetails.createdAtdate);
            printf("File permission: %s\n", fileDetails.permission);
            numFiles++;
            return 0;
        }
    }
    return 0; // Continue traversal
}

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
    strcpy(command, "tar -czf ./temp.tar.gz ");

    // Loop through the filenames and append them to the command
    for (int i = 0; i < numFiles; i++)
    {
        strcat(command, filenames[i]);
        strcat(command, " ");
    }
    printf("Command: %s\n", command);

    int status = system(command);
    // Check if the command was executed successfully
    if (status == -1)
    {
        fprintf(stderr, "Failed to create tar file.\n");
        perror("system");
        return EXIT_FAILURE;
    }
    sleep(5);

    // Free the allocated memory
    free(command);
    return EXIT_SUCCESS;
}

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

void crequest(int client_socket)
{
    char buffer[BUFFER_SIZE] = {0};
    char *hello = "Hello from server";
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
            // Create a temporary directory
            createTempDirectory();

            // Traverse the home directory and display the file paths
            if (nftw(homePath, display_info, 20, FTW_PHYS) == -1)
            {
                perror("nftw");
                exit(EXIT_FAILURE);
            }

            createTarFile();

            sendTarFileToClient(client_socket);
            // Remove the temporary directory
            removeTempDirectory();
            unlink("temp.tar.gz");
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

            if (nftw(homePath, display_info, 20, FTW_PHYS) == -1)
            {
                perror("nftw");
                exit(EXIT_FAILURE);
            }
            createTarFile();

            sendTarFileToClient(client_socket);
            // Remove the temporary directory
            removeTempDirectory();
            unlink("temp.tar.gz");
            filenames = NULL;
            numFiles = 0;
            isExtensionOption = false;
            continue;
        }
        else if (strstr(buffer, "w24fdb") != NULL)
        {
            printf("Buffer: %s\n", buffer);
            sleep(1);
            char *token = strtok(buffer, " ");
            token = strtok(NULL, " ");
            date = token;
            isLessThanDateOption = true;
            sleep(1);
            if (nftw(homePath, display_info, 20, FTW_PHYS) == -1)
            {
                perror("nftw");
                exit(EXIT_FAILURE);
            }
            createTarFile();
            printf("---->Tar file created\n");
            sleep(5);

            sendTarFileToClient(client_socket);
            // Remove the temporary directory
            removeTempDirectory();
            unlink("temp.tar.gz");
            filenames = NULL;
            numFiles = 0;
            isLessThanDateOption = false;
            continue;
        }
        else if (strstr(buffer, "w24fda") != NULL)
        {
            char *token = strtok(buffer, " ");
            token = strtok(NULL, " ");
            date = token;
            isGreaterThanDateOption = true;
            if (nftw(homePath, display_info, 20, FTW_PHYS) == -1)
            {
                perror("nftw");
                exit(EXIT_FAILURE);
            }
            createTarFile();

            sendTarFileToClient(client_socket);
            // Remove the temporary directory
            removeTempDirectory();
            unlink("temp.tar.gz");
            filenames = NULL;
            numFiles = 0;
            isGreaterThanDateOption = false;
            continue;
        }
        else if (strstr(buffer, "w24fn") != NULL)
        {
            char *token = strtok(buffer, " ");
            token = strtok(NULL, " ");
            getDetailsOffile = token;
            printf("File to get details: %s\n", getDetailsOffile);
            getFileDetails = true;
            if(nftw(homePath, display_info, 20, FTW_PHYS) == -1)
            {
                perror("nftw");
                exit(EXIT_FAILURE);
            }

            if(numFiles == 1){
                send(client_socket,&fileDetails,sizeof(fileDetails),0);

            }
            getFileDetails = false;
            getDetailsOffile = NULL;
            numFiles = 0;
            fileDetails.path = NULL;
            fileDetails.size = 0;
            fileDetails.createdAtdate = NULL;
            fileDetails.permission = NULL;
            continue;
        }

        // Check for quitc command
        if (strcmp(buffer, "quitc") == 0)
        {
            break;
        }
    }

    close(client_socket);
}


char* redirectToMirror(){
     if (countNumberOfConnections <= 3)
        {
           return "Server";
          
        }
        else if (countNumberOfConnections > 3 && countNumberOfConnections <= 6)
        {
            return "Mirror1";
           
        }
        else if (countNumberOfConnections > 6 && countNumberOfConnections <= 9)
        {
            return "Mirror2";
           
        }
        else
        {
            int remiander = countNumberOfConnections % 3;
            if (remiander == 1)
            {
                return "Server";
                
            }
            else if (remiander == 2)
            {
                return "Mirror1";
                
            }
            else
            {
                return "Mirror2";
              
            }
           
        }
        

}
void connectClientToMirror(int client_sock, int mirror_port) {
    server_address_info address_info;
    long redirect = 1;
    send(client_sock, &redirect, sizeof(redirect), 0);
    // Send address of mirror and transfer client to that location
    strcpy(address_info.ip_address, "127.0.0.1");
    address_info.port_number = mirror_port;
    send(client_sock, &address_info, sizeof(server_address_info), 0);
    printf("Client number: %d has been redirected to Mirror%d\n", countNumberOfConnections, mirror_port == MIRROR1_PORT ? 1 : 2);
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
    address.sin_port = htons(PORT);

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

    printf("Server listening on port %d\n", PORT);
    while (1)
    {
        // Accept incoming connections
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {

            perror("accept failed");
            exit(EXIT_FAILURE);
        }
        // Get the client IP address
 
 
        char* destination = redirectToMirror();
        countNumberOfConnections++;
        printf("Connection number: %d\n", countNumberOfConnections);
        printf("Destination: %s\n", destination);
        if(strcmp(destination, "Mirror1") == 0){
            printf("Redirecting to Mirror1\n");
            connectClientToMirror(new_socket, MIRROR1_PORT);
            close(new_socket);
            continue;
        }else if(strcmp(destination, "Mirror2") == 0){
            printf("Redirecting to Mirror2\n");
            connectClientToMirror(new_socket, MIRROR2_PORT);
            close(new_socket);
            continue;
        }else{
            int server = 0;
            send(new_socket, &server,sizeof(server) , 0);
        }
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
