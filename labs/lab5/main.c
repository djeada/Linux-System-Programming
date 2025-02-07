#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

// Constants for buffer size and printer line width.
#define BUF_SIZE 4096
#define LINE_WIDTH 20

// Change this to point to your input file.
#define INPUT_FILE "input.txt"

// Shared buffer structure used in both shared memory segments.
typedef struct {
    char buffer[BUF_SIZE];  // Buffer to hold text data.
    int done;               // Flag to indicate completion of processing.
} SharedBuffer;

// Function prototypes for each process.
void readerProcess(SharedBuffer *shm1);
void transformerProcess(SharedBuffer *shm1, SharedBuffer *shm2);
void printerProcess(SharedBuffer *shm2);

int main(void) {
    pid_t pid_reader, pid_transformer, pid_printer;

    // Create first shared memory region for reader -> transformer.
    SharedBuffer *shm1 = mmap(NULL, sizeof(SharedBuffer),
                                PROT_READ | PROT_WRITE,
                                MAP_SHARED | MAP_ANONYMOUS,
                                -1, 0);
    if (shm1 == MAP_FAILED) {
        perror("mmap for shm1 failed");
        exit(EXIT_FAILURE);
    }

    // Create second shared memory region for transformer -> printer.
    SharedBuffer *shm2 = mmap(NULL, sizeof(SharedBuffer),
                                PROT_READ | PROT_WRITE,
                                MAP_SHARED | MAP_ANONYMOUS,
                                -1, 0);
    if (shm2 == MAP_FAILED) {
        perror("mmap for shm2 failed");
        exit(EXIT_FAILURE);
    }

    // Initialize both shared memory regions.
    shm1->done = 0;
    shm2->done = 0;
    memset(shm1->buffer, 0, BUF_SIZE);
    memset(shm2->buffer, 0, BUF_SIZE);

    /* --- Fork the Reader Process --- */
    pid_reader = fork();
    if (pid_reader < 0) {
        perror("fork for reader failed");
        exit(EXIT_FAILURE);
    }
    if (pid_reader == 0) {
        readerProcess(shm1);
        exit(EXIT_SUCCESS);
    }

    /* --- Fork the Transformer Process --- */
    pid_transformer = fork();
    if (pid_transformer < 0) {
        perror("fork for transformer failed");
        exit(EXIT_FAILURE);
    }
    if (pid_transformer == 0) {
        transformerProcess(shm1, shm2);
        exit(EXIT_SUCCESS);
    }

    /* --- Fork the Printer Process --- */
    pid_printer = fork();
    if (pid_printer < 0) {
        perror("fork for printer failed");
        exit(EXIT_FAILURE);
    }
    if (pid_printer == 0) {
        printerProcess(shm2);
        exit(EXIT_SUCCESS);
    }

    // Parent waits for all child processes to complete.
    wait(NULL);
    wait(NULL);
    wait(NULL);

    // Clean up shared memory regions.
    munmap(shm1, sizeof(SharedBuffer));
    munmap(shm2, sizeof(SharedBuffer));

    return EXIT_SUCCESS;
}

/**
 * readerProcess:
 *   Reads an input file character by character.
 *   Replaces any newline '\n' with "<EOL>".
 *   Writes the result into shared memory region shm1.
 */
void readerProcess(SharedBuffer *shm1) {
    FILE *fp = fopen(INPUT_FILE, "r");
    if (!fp) {
        perror("readerProcess: error opening input file");
        exit(EXIT_FAILURE);
    }

    int index = 0;
    int c;
    while ((c = fgetc(fp)) != EOF) {
        // Ensure there is room in the buffer.
        if (index >= BUF_SIZE - 10) {
            fprintf(stderr, "readerProcess: Buffer overflow, stopping input.\n");
            break;
        }

        if (c == '\n') {
            // Insert "<EOL>" instead of newline.
            const char *eol = "<EOL>";
            int len = strlen(eol);
            if (index + len >= BUF_SIZE) {
                fprintf(stderr, "readerProcess: Not enough buffer space for <EOL>.\n");
                break;
            }
            strcpy(&shm1->buffer[index], eol);
            index += len;
        } else {
            shm1->buffer[index++] = (char)c;
        }
    }
    // Null-terminate the string.
    shm1->buffer[index] = '\0';
    // Signal that reading is done.
    shm1->done = 1;
    fclose(fp);
}

/**
 * transformerProcess:
 *   Waits until the reader process is finished (shm1->done is set).
 *   Reads from shared memory shm1.
 *   Replaces every occurrence of adjacent asterisks "**" with a single '#'.
 *   Writes the transformed text into shared memory shm2.
 */
void transformerProcess(SharedBuffer *shm1, SharedBuffer *shm2) {
    // Wait until the reader process has completed its work.
    while (!shm1->done) {
        usleep(1000);  // Sleep for 1 ms to reduce busy waiting.
    }

    int in = 0;    // Index for reading from shm1.
    int out = 0;   // Index for writing to shm2.

    while (shm1->buffer[in] != '\0') {
        // Check for adjacent asterisks.
        if (shm1->buffer[in] == '*' && shm1->buffer[in + 1] == '*') {
            shm2->buffer[out++] = '#';
            in += 2;  // Skip both asterisks.
        } else {
            shm2->buffer[out++] = shm1->buffer[in++];
        }
        if (out >= BUF_SIZE - 1) {
            fprintf(stderr, "transformerProcess: Buffer overflow, stopping transformation.\n");
            break;
        }
    }
    // Null-terminate the transformed output.
    shm2->buffer[out] = '\0';
    // Signal that transformation is complete.
    shm2->done = 1;
}

/**
 * printerProcess:
 *   Waits until the transformer process is finished (shm2->done is set).
 *   Reads from shared memory shm2.
 *   Prints the text to standard output in lines of exactly 20 characters.
 */
void printerProcess(SharedBuffer *shm2) {
    // Wait until the transformer has finished processing.
    while (!shm2->done) {
        usleep(1000);  // Sleep for 1 ms.
    }

    int count = 0;
    for (int i = 0; shm2->buffer[i] != '\0'; i++) {
        putchar(shm2->buffer[i]);
        count++;
        if (count == LINE_WIDTH) {
            putchar('\n');
            count = 0;
        }
    }
    // If there are remaining characters, end with a newline.
    if (count > 0) {
        putchar('\n');
    }
}
