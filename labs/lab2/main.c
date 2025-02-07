#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#define FILE_PATH "/path/to/your/file.txt"  // <<<=== Change this path as needed.
#define MAX_LINE_LENGTH 256

int main(void) {
    pid_t pid;

    // Create a child process using fork()
    pid = fork();

    if (pid < 0) {
        // fork() failed; print an error message and exit.
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    
    // Both the parent and the child will execute the code below.
    // Each process opens the file independently.
    FILE *fp = fopen(FILE_PATH, "r");
    if (fp == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    
    char line[MAX_LINE_LENGTH];
    
    // Read the file line by line.
    while (fgets(line, sizeof(line), fp) != NULL) {
        // Print the process's PID along with the line that was read.
        printf("[PID %d] %s", getpid(), line);
    }
    
    fclose(fp);
    return EXIT_SUCCESS;
}
