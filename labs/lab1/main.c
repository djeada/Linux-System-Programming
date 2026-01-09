#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAX_ROWS 100
#define MAX_COLS 100
#define TIMEOUT_SECONDS 5

#define WALL_CELL '#'
#define START_CELL 'S'
#define EXIT_CELL 'E'
#define PLAYER_CELL 'P'
#define HIDDEN_CELL '?'

static char maze[MAX_ROWS][MAX_COLS];
static bool revealed[MAX_ROWS][MAX_COLS];
static int rows = 0;
static int cols = 0;
static int player_row = -1;
static int player_col = -1;
static bool game_completed = false;

static void load_maze(const char *filename);
static void print_maze(void);
static void clear_screen(void);
static void reveal_cell(int r, int c);
static void reveal_surroundings(int r, int c);
static bool reveal_adjacent_hint(void);
static void reveal_random_safe_cell(void);
static bool is_maze_fully_revealed(void);
static bool in_bounds(int r, int c);
static bool read_move(char *move_out);

static void cheat_handler(int signo);
static void alarm_handler(int signo);
static void quit_handler(int signo);

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <maze_file>\n", argv[0]);
    return EXIT_FAILURE;
  }

  load_maze(argv[1]);
  srand((unsigned int)time(NULL));

  signal(SIGALRM, alarm_handler);
  signal(SIGINT, quit_handler);
  signal(SIGTSTP, cheat_handler);

  while (!game_completed) {
    print_maze();
    printf("\nUse W (up), A (left), S (down), D (right) to move: ");
    fflush(stdout);

    char move = '\0';
    if (!read_move(&move)) {
      printf("\nInput error. Exiting...\n");
      return EXIT_FAILURE;
    }

    int new_row = player_row;
    int new_col = player_col;
    if (move == 'W') {
      new_row--;
    } else if (move == 'A') {
      new_col--;
    } else if (move == 'S') {
      new_row++;
    } else if (move == 'D') {
      new_col++;
    } else {
      printf("\nInvalid command. Please use W, A, S, or D.\n");
      continue;
    }

    if (!in_bounds(new_row, new_col)) {
      printf("\nYou can't move outside the maze! Here's a hint.\n");
      reveal_adjacent_hint();
      continue;
    }

    if (maze[new_row][new_col] == WALL_CELL) {
      printf("\nYou hit a wall! Here's a hint.\n");
      reveal_adjacent_hint();
      continue;
    }

    player_row = new_row;
    player_col = new_col;
    reveal_surroundings(player_row, player_col);

    if (maze[player_row][player_col] == EXIT_CELL) {
      print_maze();
      printf("\nCongratulations! You have reached the exit!\n");
      game_completed = true;
      continue;
    }

    if (is_maze_fully_revealed()) {
      print_maze();
      printf("\nThe maze is fully revealed. Game over!\n");
      game_completed = true;
    }
  }

  return EXIT_SUCCESS;
}

static void load_maze(const char *filename) {
  FILE *fp = fopen(filename, "r");
  if (!fp) {
    perror("Error opening maze file");
    exit(EXIT_FAILURE);
  }

  char line[MAX_COLS + 2];
  bool start_found = false;
  bool exit_found = false;

  rows = 0;
  cols = 0;

  while (fgets(line, sizeof(line), fp)) {
    line[strcspn(line, "\r\n")] = '\0';

    int line_len = (int)strlen(line);
    if (line_len == 0) {
      continue;
    }
    if (line_len > MAX_COLS) {
      fprintf(stderr, "Maze row exceeds max columns (%d).\n", MAX_COLS);
      exit(EXIT_FAILURE);
    }

    if (rows == 0) {
      cols = line_len;
    } else if (line_len != cols) {
      fprintf(stderr, "Inconsistent row lengths in maze file.\n");
      exit(EXIT_FAILURE);
    }

    for (int i = 0; i < cols; i++) {
      maze[rows][i] = line[i];
      revealed[rows][i] = false;
      if (line[i] == START_CELL) {
        player_row = rows;
        player_col = i;
        start_found = true;
      } else if (line[i] == EXIT_CELL) {
        exit_found = true;
      }
    }

    rows++;
    if (rows >= MAX_ROWS) {
      fprintf(stderr, "Maze exceeds max rows (%d).\n", MAX_ROWS);
      exit(EXIT_FAILURE);
    }
  }

  fclose(fp);

  if (!start_found) {
    fprintf(stderr,
            "Maze file must contain a starting position marked with '%c'.\n",
            START_CELL);
    exit(EXIT_FAILURE);
  }
  if (!exit_found) {
    fprintf(stderr, "Maze file must contain an exit marked with '%c'.\n",
            EXIT_CELL);
    exit(EXIT_FAILURE);
  }

  reveal_surroundings(player_row, player_col);
}

static void clear_screen(void) {
  printf("\033[H\033[J");
  fflush(stdout);
}

static void print_maze(void) {
  clear_screen();
  for (int r = 0; r < rows; r++) {
    for (int c = 0; c < cols; c++) {
      if (r == player_row && c == player_col) {
        putchar(PLAYER_CELL);
      } else if (revealed[r][c]) {
        putchar(maze[r][c]);
      } else {
        putchar(HIDDEN_CELL);
      }
    }
    putchar('\n');
  }
}

static void reveal_cell(int r, int c) {
  if (in_bounds(r, c)) {
    revealed[r][c] = true;
  }
}

static void reveal_surroundings(int r, int c) {
  reveal_cell(r, c);
  reveal_cell(r - 1, c);
  reveal_cell(r + 1, c);
  reveal_cell(r, c - 1);
  reveal_cell(r, c + 1);
}

static bool reveal_adjacent_hint(void) {
  static const int dr[4] = {-1, 1, 0, 0};
  static const int dc[4] = {0, 0, -1, 1};

  for (int i = 0; i < 4; i++) {
    int nr = player_row + dr[i];
    int nc = player_col + dc[i];
    if (in_bounds(nr, nc) && !revealed[nr][nc] && maze[nr][nc] != WALL_CELL) {
      reveal_cell(nr, nc);
      printf("Hint: Revealed cell at (%d, %d).\n", nr + 1, nc + 1);
      return true;
    }
  }

  printf("No adjacent hint available.\n");
  return false;
}

static void reveal_random_safe_cell(void) {
  int safe_hidden = 0;
  for (int r = 0; r < rows; r++) {
    for (int c = 0; c < cols; c++) {
      if (!revealed[r][c] && maze[r][c] != WALL_CELL) {
        safe_hidden++;
      }
    }
  }

  if (safe_hidden == 0) {
    printf("Cheat mode: No unrevealed safe cell found.\n");
    return;
  }

  int target = rand() % safe_hidden;
  for (int r = 0; r < rows; r++) {
    for (int c = 0; c < cols; c++) {
      if (!revealed[r][c] && maze[r][c] != WALL_CELL) {
        if (target == 0) {
          reveal_cell(r, c);
          printf("Cheat: Revealed cell at (%d, %d) containing '%c'.\n", r + 1,
                 c + 1, maze[r][c]);
          return;
        }
        target--;
      }
    }
  }
}

static bool is_maze_fully_revealed(void) {
  for (int r = 0; r < rows; r++) {
    for (int c = 0; c < cols; c++) {
      if (!revealed[r][c]) {
        return false;
      }
    }
  }
  return true;
}

static bool in_bounds(int r, int c) {
  return r >= 0 && r < rows && c >= 0 && c < cols;
}

static bool read_move(char *move_out) {
  char input_buffer[16];

  while (true) {
    errno = 0;
    alarm(TIMEOUT_SECONDS);

    if (!fgets(input_buffer, sizeof(input_buffer), stdin)) {
      alarm(0);
      if (errno == EINTR) {
        clearerr(stdin);
        continue;
      }
      return false;
    }

    alarm(0);

    for (int i = 0; input_buffer[i] != '\0'; i++) {
      if (!isspace((unsigned char)input_buffer[i])) {
        *move_out = (char)toupper((unsigned char)input_buffer[i]);
        return true;
      }
    }

    *move_out = '\0';
    return true;
  }
}

static void cheat_handler(int signo) {
  (void)signo;
  printf("\nCheat mode activated!\n");
  reveal_random_safe_cell();
  print_maze();
}

static void alarm_handler(int signo) {
  (void)signo;
  printf("\nTime's up! Please respond faster.\n");
}

static void quit_handler(int signo) {
  (void)signo;
  printf("\nExiting the Maze Navigator. Goodbye!\n");
  exit(EXIT_SUCCESS);
}
