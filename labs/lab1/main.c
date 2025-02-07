#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>

#define MAX_ROWS 100
#define MAX_COLS 100
#define TIMEOUT_SECONDS 5

// Global maze variables
char maze[MAX_ROWS][MAX_COLS];
int revealed[MAX_ROWS][MAX_COLS]; // 0: hidden, 1: revealed
int rows = 0, cols = 0;
int playerRow = -1, playerCol = -1;

// Flag to indicate game over (win)
int gameCompleted = 0;

// Function prototypes
void loadMaze(const char *filename);
void printMaze(void);
void revealCell(int r, int c);
void revealAdjacentHint(void);
void cheatHandler(int signo);
void alarmHandler(int signo);
void quitHandler(int signo);
void revealRandomSafeCell(void);

int main(int argc, char *argv[]) {
    // Ensure a maze file is provided.
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <maze_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    // Load the maze from the file.
    loadMaze(argv[1]);
    
    // Seed the random number generator.
    srand((unsigned int) time(NULL));
    
    // Set up signal handlers.
    signal(SIGALRM, alarmHandler); // Timeout if no input within TIMEOUT_SECONDS.
    signal(SIGINT, quitHandler);     // Quit if user presses Ctrl+C.
    signal(SIGTSTP, cheatHandler);   // Cheat mode if user presses Ctrl+Z.
    
    char inputBuffer[10];
    char move;
    
    while (!gameCompleted) {
        // Display the maze.
        printMaze();
        printf("\nUse W (up), A (left), S (down), D (right) to move: ");
        
        // Set an alarm for timeout.
        alarm(TIMEOUT_SECONDS);
        if (fgets(inputBuffer, sizeof(inputBuffer), stdin) == NULL) {
            // If input fails, exit.
            printf("\nInput error. Exiting...\n");
            exit(EXIT_FAILURE);
        }
        // Cancel the alarm once input is received.
        alarm(0);
        
        // Process only the first non-whitespace character.
        move = '\0';
        for (int i = 0; inputBuffer[i] != '\0'; i++) {
            if (!isspace(inputBuffer[i])) {
                move = toupper(inputBuffer[i]);
                break;
            }
        }
        
        // Determine the new player position based on input.
        int newRow = playerRow, newCol = playerCol;
        if (move == 'W') {
            newRow = playerRow - 1;
        } else if (move == 'A') {
            newCol = playerCol - 1;
        } else if (move == 'S') {
            newRow = playerRow + 1;
        } else if (move == 'D') {
            newCol = playerCol + 1;
        } else {
            printf("\nInvalid command. Please use W, A, S, or D.\n");
            continue;
        }
        
        // Check bounds.
        if (newRow < 0 || newRow >= rows || newCol < 0 || newCol >= cols) {
            printf("\nYou can't move outside the maze! Here's a hint.\n");
            revealAdjacentHint();
            continue;
        }
        
        // Check if moving into a wall.
        if (maze[newRow][newCol] == '#') {
            printf("\nYou hit a wall! Here's a hint.\n");
            revealAdjacentHint();
            continue;
        }
        
        // Valid move: update player's position.
        playerRow = newRow;
        playerCol = newCol;
        // Reveal the new cell.
        revealCell(playerRow, playerCol);
        // Also reveal adjacent cells (simulate exploration).
        if (playerRow > 0) revealCell(playerRow - 1, playerCol);
        if (playerRow < rows - 1) revealCell(playerRow + 1, playerCol);
        if (playerCol > 0) revealCell(playerRow, playerCol - 1);
        if (playerCol < cols - 1) revealCell(playerRow, playerCol + 1);
        
        // Check if the exit is reached.
        if (maze[playerRow][playerCol] == 'E') {
            printMaze();
            printf("\nCongratulations! You have reached the exit!\n");
            gameCompleted = 1;
        }
    }
    
    return EXIT_SUCCESS;
}

/**
 * loadMaze - Reads the maze from a file.
 * The maze file must have consistent line lengths.
 */
void loadMaze(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Error opening maze file");
        exit(EXIT_FAILURE);
    }
    
    char line[MAX_COLS + 2]; // +2 to account for newline and null terminator.
    rows = 0;
    while (fgets(line, sizeof(line), fp)) {
        // Remove newline character if present.
        line[strcspn(line, "\n")] = '\0';
        // On the first line, set the number of columns.
        if (rows == 0) {
            cols = strlen(line);
            if (cols == 0) {
                fprintf(stderr, "Maze file is empty or invalid.\n");
                exit(EXIT_FAILURE);
            }
        } else if ((int)strlen(line) != cols) {
            fprintf(stderr, "Inconsistent row lengths in maze file.\n");
            exit(EXIT_FAILURE);
        }
        // Copy the line into the maze.
        for (int i = 0; i < cols; i++) {
            maze[rows][i] = line[i];
            revealed[rows][i] = 0; // Initially hidden.
            // Find the starting position.
            if (line[i] == 'S') {
                playerRow = rows;
                playerCol = i;
            }
        }
        rows++;
        if (rows >= MAX_ROWS) break;
    }
    fclose(fp);
    
    if (playerRow == -1 || playerCol == -1) {
        fprintf(stderr, "Maze file must contain a starting position marked with 'S'.\n");
        exit(EXIT_FAILURE);
    }
    
    // Reveal the starting cell and its neighbors.
    revealCell(playerRow, playerCol);
    if (playerRow > 0) revealCell(playerRow - 1, playerCol);
    if (playerRow < rows - 1) revealCell(playerRow + 1, playerCol);
    if (playerCol > 0) revealCell(playerRow, playerCol - 1);
    if (playerCol < cols - 1) revealCell(playerRow, playerCol + 1);
}

/**
 * printMaze - Displays the maze on screen.
 * Only revealed cells are shown; unrevealed cells appear as '?'.
 * The player's current position is marked with 'P'.
 */
void printMaze(void) {
    system("clear");
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            if (r == playerRow && c == playerCol) {
                printf("P");
            } else if (revealed[r][c]) {
                printf("%c", maze[r][c]);
            } else {
                printf("?");
            }
        }
        printf("\n");
    }
}

/**
 * revealCell - Marks a cell as revealed.
 */
void revealCell(int r, int c) {
    if (r >= 0 && r < rows && c >= 0 && c < cols) {
        revealed[r][c] = 1;
    }
}

/**
 * revealAdjacentHint - When an invalid move is made,
 * reveals one adjacent unrevealed safe (non-wall) cell.
 */
void revealAdjacentHint(void) {
    // Check the four adjacent cells.
    int dr[4] = {-1, 1, 0, 0};
    int dc[4] = {0, 0, -1, 1};
    int found = 0;
    for (int i = 0; i < 4 && !found; i++) {
        int nr = playerRow + dr[i];
        int nc = playerCol + dc[i];
        if (nr >= 0 && nr < rows && nc >= 0 && nc < cols &&
            !revealed[nr][nc] && maze[nr][nc] != '#') {
            revealCell(nr, nc);
            printf("Hint: Revealed cell at (%d, %d).\n", nr + 1, nc + 1);
            found = 1;
        }
    }
    if (!found) {
        printf("No adjacent hint available.\n");
    }
}

/**
 * revealRandomSafeCell - Reveals a random unrevealed cell that is not a wall.
 * This function is used by the cheat mode.
 */
void revealRandomSafeCell(void) {
    int r, c, attempts = 0;
    // Try up to 100 times to find an unrevealed non-wall cell.
    while (attempts < 100) {
        r = rand() % rows;
        c = rand() % cols;
        if (!revealed[r][c] && maze[r][c] != '#') {
            revealCell(r, c);
            printf("Cheat: Revealed cell at (%d, %d) containing '%c'.\n", r + 1, c + 1, maze[r][c]);
            return;
        }
        attempts++;
    }
    printf("Cheat mode: No unrevealed safe cell found.\n");
}

/**
 * cheatHandler - Signal handler for SIGTSTP (Ctrl+Z).
 * Reveals a random safe unrevealed cell.
 */
void cheatHandler(int signo) {
    (void)signo;  // Unused parameter
    printf("\nCheat mode activated!\n");
    revealRandomSafeCell();
    printMaze();
}

/**
 * alarmHandler - Called when the user takes too long to input a command.
 */
void alarmHandler(int signo) {
    (void)signo;  // Unused parameter
    printf("\nTime's up! Please respond faster.\n");
    // Note: The game continues; the alarm simply notifies the user.
}

/**
 * quitHandler - Called when the user presses Ctrl+C to quit.
 */
void quitHandler(int signo) {
    (void)signo;  // Unused parameter
    printf("\nExiting the Maze Navigator. Goodbye!\n");
    exit(EXIT_SUCCESS);
}
