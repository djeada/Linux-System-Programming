#include "../common/system.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
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

static bool load_maze(const char *filename);
static void print_maze(void);
static void clear_screen(void);
static void reveal_cell(int r, int c);
static void reveal_surroundings(int r, int c);
static bool reveal_adjacent_hint(void);
static void reveal_random_safe_cell(void);
static bool is_maze_fully_revealed(void);
static bool in_bounds(int r, int c);
static int read_move(char *move_out);

static int signal_pipe[2] = {-1, -1};
static volatile sig_atomic_t quit_requested = 0;
static volatile sig_atomic_t cheat_requested = 0;

/* Only async-signal-safe notification belongs in a handler. */
static void notify_signal(int signo) {
  int saved_errno = errno;
  if (signo == SIGINT)
    quit_requested = 1;
  else
    cheat_requested = 1;
  const char wake = 'x';
  ssize_t ignored = write(signal_pipe[1], &wake, 1);
  (void)ignored;
  errno = saved_errno;
}

static bool setup_signals(void) {
  if (pipe(signal_pipe) < 0) {
    perror("pipe");
    return false;
  }
  for (int i = 0; i < 2; ++i) {
    if (fcntl(signal_pipe[i], F_SETFL, O_NONBLOCK) < 0 ||
        fcntl(signal_pipe[i], F_SETFD, FD_CLOEXEC) < 0) {
      perror("fcntl");
      close(signal_pipe[0]);
      close(signal_pipe[1]);
      return false;
    }
  }
  struct sigaction action = {.sa_handler = notify_signal};
  sigemptyset(&action.sa_mask);
  if (sigaction(SIGINT, &action, NULL) < 0 ||
      sigaction(SIGTSTP, &action, NULL) < 0) {
    perror("sigaction");
    close(signal_pipe[0]);
    close(signal_pipe[1]);
    return false;
  }
  return true;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <maze_file>\n", argv[0]);
    return EXIT_FAILURE;
  }

  if (!load_maze(argv[1]))
    return EXIT_FAILURE;
  srand((unsigned int)time(NULL));

  if (!setup_signals())
    return EXIT_FAILURE;

  while (!game_completed) {
    print_maze();
    printf("\nUse W (up), A (left), S (down), D (right) to move: ");
    fflush(stdout);

    char move = '\0';
    int input_status = read_move(&move);
    if (input_status <= 0 || game_completed) {
      sigset_t blocked;
      sigemptyset(&blocked);
      sigaddset(&blocked, SIGINT);
      sigaddset(&blocked, SIGTSTP);
      sigprocmask(SIG_BLOCK, &blocked, NULL);
      close(signal_pipe[0]);
      close(signal_pipe[1]);
      return input_status < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
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

  sigset_t blocked;
  sigemptyset(&blocked);
  sigaddset(&blocked, SIGINT);
  sigaddset(&blocked, SIGTSTP);
  sigprocmask(SIG_BLOCK, &blocked, NULL);
  close(signal_pipe[0]);
  close(signal_pipe[1]);
  return EXIT_SUCCESS;
}

static bool load_maze(const char *filename) {
  FILE *fp = fopen(filename, "r");
  if (!fp) {
    perror("open maze");
    return false;
  }
  char *line = NULL;
  size_t capacity = 0;
  ssize_t length;
  int starts = 0, exits = 0;
  bool valid = false;
  rows = 0;
  cols = 0;
  while ((length = getline(&line, &capacity, fp)) >= 0) {
    if (length && line[length - 1] == '\n')
      --length;
    if (length && line[length - 1] == '\r')
      --length;
    if (length == 0 || length > MAX_COLS || rows == MAX_ROWS ||
        memchr(line, '\0', (size_t)length) || (rows && length != cols)) {
      fprintf(stderr, "Invalid maze dimensions or inconsistent row lengths.\n");
      goto done;
    }
    cols = (int)length;
    for (int column = 0; column < cols; ++column) {
      maze[rows][column] = line[column];
      revealed[rows][column] = false;
      if (line[column] == START_CELL) {
        ++starts;
        player_row = rows;
        player_col = column;
      } else if (line[column] == EXIT_CELL)
        ++exits;
    }
    ++rows;
  }
  if (ferror(fp)) {
    perror("read maze");
    goto done;
  }
  if (starts != 1 || exits == 0) {
    fprintf(stderr, "Maze must contain exactly one S and at least one E.\n");
    goto done;
  }
  reveal_surroundings(player_row, player_col);
  valid = true;
done:
  free(line);
  fclose(fp);
  return valid;
}

static void clear_screen(void) {
  if (isatty(STDOUT_FILENO))
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

static int read_move(char *move_out) {
  static char input[4096];
  static size_t position = 0, available = 0;
  for (;;) {
    if (quit_requested) {
      printf("\nExiting the Maze Navigator. Goodbye!\n");
      game_completed = true;
      return 0;
    }
    if (cheat_requested) {
      cheat_requested = 0;
      printf("\nCheat mode activated!\n");
      reveal_random_safe_cell();
      print_maze();
      if (is_maze_fully_revealed()) {
        game_completed = true;
        printf("The maze is fully revealed. Game over!\n");
        return 0;
      }
    }
    while (position < available) {
      unsigned char value = (unsigned char)input[position++];
      if (!isspace(value)) {
        *move_out = (char)toupper(value);
        return 1;
      }
    }
    struct pollfd ready[] = {{STDIN_FILENO, POLLIN, 0},
                             {signal_pipe[0], POLLIN, 0}};
    int result = poll(ready, 2, TIMEOUT_SECONDS * 1000);
    if (result < 0) {
      if (errno == EINTR)
        continue;
      perror("poll");
      return -1;
    }
    if (result == 0) {
      printf("\nTime's up! Please respond faster.\n");
      fflush(stdout);
      continue;
    }
    if (ready[1].revents & POLLIN) {
      char wake[64];
      while (read(signal_pipe[0], wake, sizeof(wake)) > 0) {
      }
      continue;
    }
    if (ready[0].revents & (POLLIN | POLLHUP)) {
      ssize_t bytes = read(STDIN_FILENO, input, sizeof(input));
      if (bytes < 0 && errno == EINTR)
        continue;
      if (bytes < 0) {
        perror("read input");
        return -1;
      }
      if (bytes == 0)
        return 0;
      position = 0;
      available = (size_t)bytes;
    } else if (ready[0].revents & (POLLERR | POLLNVAL)) {
      fprintf(stderr, "Input stream unavailable\n");
      return -1;
    }
  }
}
