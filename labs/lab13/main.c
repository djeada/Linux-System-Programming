#include "../common/io.h"
#include <poll.h>
#include <sys/signalfd.h>
#include <termios.h>

enum { LINE_MAXIMUM = 65536, TOKENS = 256, STAGES = 8, ARGS = 64, JOBS = 16 };
typedef struct {
  char *word;
  char kind;
} Token;
typedef struct {
  char *argv[ARGS + 1];
  int argc;
  char *input, *output;
  bool append;
} Command;
typedef struct {
  bool used, modes_saved;
  pid_t group, pids[STAGES];
  int states[STAGES], statuses[STAGES],
      count; /* running=0, stopped=1, done=2 */
  struct termios modes;
  char description[160];
} Job;
static Job jobs[JOBS];
static bool interactive;
static pid_t shell_group;
static struct termios shell_modes;
static sigset_t original_mask;

/* Decode quotes into separate storage, so operators cannot overwrite input. */
static int tokenize(const char *line, Token *tokens, char *storage) {
  int count = 0;
  char *out = storage;
  while (*line) {
    if (isspace((unsigned char)*line)) {
      ++line;
      continue;
    }
    if (count == TOKENS)
      return -1;
    if (strchr("|<>&", *line)) {
      char operation = *line++;
      if (operation == '>' && *line == '>') {
        operation = 'A';
        ++line;
      }
      tokens[count++] = (Token){.kind = operation};
      continue;
    }
    char *start = out;
    char quote = 0;
    while (*line && (quote || (!isspace((unsigned char)*line) &&
                               !strchr("|<>&", *line)))) {
      char c = *line++;
      if (c == '\\' && quote != '\'') {
        if (!*line)
          return -1;
        *out++ = *line++;
      } else if (c == '\'' || c == '"') {
        if (!quote)
          quote = c;
        else if (quote == c)
          quote = 0;
        else
          *out++ = c;
      } else
        *out++ = c;
    }
    if (quote)
      return -1;
    *out++ = '\0';
    tokens[count++] = (Token){.word = start};
  }
  return count;
}

static int parse(Token *tokens, int count, Command *commands,
                 bool *background) {
  int stage = 0;
  *background = false;
  for (int i = 0; i < count; ++i) {
    Command *command = &commands[stage];
    char op = tokens[i].kind;
    if (!op) {
      if (command->argc == ARGS)
        return -1;
      command->argv[command->argc++] = tokens[i].word;
    } else if (op == '|') {
      if (!command->argc || stage + 1 == STAGES)
        return -1;
      ++stage;
    } else if (op == '&') {
      if (i != count - 1 || !command->argc)
        return -1;
      *background = true;
    } else {
      if (++i == count || tokens[i].kind)
        return -1;
      if (op == '<') {
        if (command->input)
          return -1;
        command->input = tokens[i].word;
      } else {
        if (command->output)
          return -1;
        command->output = tokens[i].word;
        command->append = op == 'A';
      }
    }
  }
  return commands[stage].argc ? stage + 1 : -1;
}

static bool done(const Job *job) {
  for (int i = 0; i < job->count; ++i)
    if (job->states[i] != 2)
      return false;
  return true;
}
static bool stopped(const Job *job) {
  if (done(job))
    return false;
  for (int i = 0; i < job->count; ++i)
    if (job->states[i] == 0)
      return false;
  return true;
}
static void record(pid_t pid, int status) {
  for (int j = 0; j < JOBS; ++j)
    if (jobs[j].used)
      for (int i = 0; i < jobs[j].count; ++i)
        if (jobs[j].pids[i] == pid) {
          jobs[j].statuses[i] = status;
          jobs[j].states[i] = WIFSTOPPED(status)     ? 1
                              : WIFCONTINUED(status) ? 0
                                                     : 2;
          return;
        }
}
static void reap(void) {
  int status;
  pid_t pid;
  while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0)
    record(pid, status);
  for (int j = 0; j < JOBS; ++j)
    if (jobs[j].used && done(&jobs[j])) {
      printf("[%d] done %s\n", j + 1, jobs[j].description);
      jobs[j].used = false;
    }
}
static int terminal(pid_t group) {
  if (!interactive)
    return 0;
  int result;
  do {
    result = tcsetpgrp(STDIN_FILENO, group);
  } while (result < 0 && errno == EINTR);
  return result;
}
static int foreground(Job *job, bool resume) {
  if (terminal(job->group) < 0) {
    perror("tcsetpgrp job");
    return 1;
  }
  if (resume) {
    if (interactive && job->modes_saved)
      tcsetattr(STDIN_FILENO, TCSADRAIN, &job->modes);
    if (kill(-job->group, SIGCONT) < 0)
      perror("SIGCONT");
    for (int i = 0; i < job->count; ++i)
      if (job->states[i] == 1)
        job->states[i] = 0;
  }
  while (!done(job) && !stopped(job)) {
    int status;
    pid_t pid = waitpid(-job->group, &status, WUNTRACED);
    if (pid < 0 && errno == EINTR)
      continue;
    if (pid < 0) {
      perror("waitpid foreground");
      break;
    }
    record(pid, status);
  }
  if (interactive && stopped(job)) {
    job->modes_saved = tcgetattr(STDIN_FILENO, &job->modes) == 0;
  }
  if (terminal(shell_group) < 0)
    perror("tcsetpgrp shell");
  if (interactive)
    tcsetattr(STDIN_FILENO, TCSADRAIN, &shell_modes);
  int status = job->statuses[job->count - 1];
  int result = WIFEXITED(status)     ? WEXITSTATUS(status)
               : WIFSIGNALED(status) ? 128 + WTERMSIG(status)
                                     : 128 + SIGTSTP;
  if (done(job))
    job->used = false;
  else
    printf("[%ld] stopped %s\n", (long)job->group, job->description);
  return result;
}

static int redirect(const char *path, int target, int flags) {
  int fd = open(path, flags | O_CLOEXEC, 0600);
  if (fd < 0)
    return -1;
  if (fd == target)
    return fcntl(fd, F_SETFD, 0);
  int result = dup2(fd, target);
  int saved = errno;
  close(fd);
  errno = saved;
  return result;
}

static int launch(Command *commands, int count, bool background,
                  const char *line) {
  int slot = 0;
  while (slot < JOBS && jobs[slot].used)
    ++slot;
  if (slot == JOBS) {
    fprintf(stderr, "Job table is full\n");
    return 1;
  }
  int pipes[STAGES - 1][2], gate[2] = {-1, -1}, created = 0;
  Job *job = &jobs[slot];
  *job = (Job){.used = true};
  snprintf(job->description, sizeof job->description, "%s", line);
  for (; created < count - 1; ++created)
    if (pipe2(pipes[created], O_CLOEXEC) < 0)
      goto failed;
  if (pipe2(gate, O_CLOEXEC) < 0)
    goto failed;
  fflush(NULL);
  for (int i = 0; i < count; ++i) {
    pid_t pid = fork();
    if (pid < 0)
      goto failed;
    if (!pid) {
      close(gate[1]);
      if (setpgid(0, job->group) < 0)
        _exit(126);
      int defaults[] = {SIGINT,  SIGQUIT, SIGTSTP, SIGTTIN,
                        SIGTTOU, SIGCHLD, SIGPIPE};
      struct sigaction action = {.sa_handler = SIG_DFL};
      sigemptyset(&action.sa_mask);
      for (size_t s = 0; s < sizeof defaults / sizeof *defaults; ++s)
        sigaction(defaults[s], &action, NULL);
      sigprocmask(SIG_SETMASK, &original_mask, NULL);
      char byte;
      if (lab_read(gate[0], &byte, 1) < 0)
        _exit(126);
      close(gate[0]);
      int error = 0;
      if (i && dup2(pipes[i - 1][0], STDIN_FILENO) < 0)
        error = 1;
      if (i < count - 1 && dup2(pipes[i][1], STDOUT_FILENO) < 0)
        error = 1;
      for (int p = 0; p < created; ++p) {
        close(pipes[p][0]);
        close(pipes[p][1]);
      }
      Command *command = &commands[i];
      if (command->input &&
          redirect(command->input, STDIN_FILENO, O_RDONLY) < 0)
        error = 1;
      else if (!i && background && !interactive && !command->input &&
               redirect("/dev/null", STDIN_FILENO, O_RDONLY) < 0)
        error = 1;
      if (command->output &&
          redirect(command->output, STDOUT_FILENO,
                   O_WRONLY | O_CREAT |
                       (command->append ? O_APPEND : O_TRUNC)) < 0)
        error = 1;
      if (error) {
        perror("pipeline redirection");
        _exit(126);
      }
      execvp(command->argv[0], command->argv);
      int saved = errno;
      perror(command->argv[0]);
      _exit(saved == ENOENT ? 127 : 126);
    }
    if (!job->group)
      job->group = pid;
    job->pids[job->count++] = pid;
    if (setpgid(pid, job->group) < 0 && errno != EACCES && errno != ESRCH)
      goto failed;
  }
  for (int p = 0; p < created; ++p) {
    close(pipes[p][0]);
    close(pipes[p][1]);
  }
  /* Transfer the terminal before releasing children to read it. */
  if (!background && terminal(job->group) < 0) {
    perror("tcsetpgrp");
    kill(-job->group, SIGKILL);
  }
  close(gate[0]);
  close(gate[1]);
  if (background) {
    printf("[%d] pgid=%ld\n", slot + 1, (long)job->group);
    return 0;
  }
  return foreground(job, false);
failed: {
  int saved = errno;
  /* Kill known PIDs as well: setpgid itself might have failed. */
  for (int i = 0; i < job->count; ++i)
    kill(job->pids[i], SIGKILL);
  if (gate[0] >= 0)
    close(gate[0]);
  if (gate[1] >= 0)
    close(gate[1]);
  for (int p = 0; p < created; ++p) {
    close(pipes[p][0]);
    close(pipes[p][1]);
  }
  for (int i = 0; i < job->count; ++i)
    while (waitpid(job->pids[i], NULL, 0) < 0 && errno == EINTR) {
    }
  job->used = false;
  errno = saved;
  perror("pipeline startup");
  return 1;
}
}

static void shutdown_jobs(void) {
  for (int j = 0; j < JOBS; ++j)
    if (jobs[j].used && !done(&jobs[j])) {
      kill(-jobs[j].group, SIGHUP);
      kill(-jobs[j].group, SIGCONT);
    }
  uint64_t deadline = lab_milliseconds() + 200;
  while (lab_milliseconds() < deadline) {
    reap();
    bool live = false;
    for (int j = 0; j < JOBS; ++j)
      live |= jobs[j].used;
    if (!live)
      return;
    lab_sleep_ms(10);
  }
  for (int j = 0; j < JOBS; ++j)
    if (jobs[j].used)
      kill(-jobs[j].group, SIGKILL);
  while (waitpid(-1, NULL, 0) > 0 || errno == EINTR) {
  }
}

int main(void) {
  interactive = isatty(STDIN_FILENO);
  shell_group = getpgrp();
  if (interactive) {
    /* A shell launched in the background must wait for terminal ownership. */
    struct sigaction action = {.sa_handler = SIG_DFL};
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGTTIN, &action, NULL) < 0) {
      perror("SIGTTIN");
      return 1;
    }
    while (tcgetpgrp(STDIN_FILENO) != shell_group)
      kill(-shell_group, SIGTTIN);
    if (setpgid(0, 0) < 0 && getpgrp() != getpid()) {
      perror("setpgid shell");
      return 1;
    }
    shell_group = getpgrp();
  }
  struct sigaction ignore = {.sa_handler = SIG_IGN};
  sigemptyset(&ignore.sa_mask);
  int ignored[] = {SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU};
  for (size_t i = 0; i < sizeof ignored / sizeof *ignored; ++i)
    sigaction(ignored[i], &ignore, NULL);
  if (interactive && (terminal(shell_group) < 0 ||
                      tcgetattr(STDIN_FILENO, &shell_modes) < 0)) {
    perror("terminal setup");
    return 1;
  }
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGCHLD);
  if (sigprocmask(SIG_BLOCK, &mask, &original_mask) < 0) {
    perror("sigprocmask");
    return 1;
  }
  int signals = signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
  if (signals < 0) {
    perror("signalfd");
    return 1;
  }
  char *line = malloc(LINE_MAXIMUM + 1), *storage = malloc(LINE_MAXIMUM + 1);
  if (!line || !storage) {
    free(line);
    free(storage);
    close(signals);
    return 1;
  }
  size_t length = 0;
  bool discard = false, exiting = false;
  int status = 0;
  if (interactive) {
    printf("lab13$ ");
    fflush(stdout);
  }
  while (!exiting) {
    struct pollfd ready[] = {{.fd = STDIN_FILENO, .events = POLLIN},
                             {.fd = signals, .events = POLLIN}};
    int available = poll(ready, 2, -1);
    if (available < 0 && errno == EINTR)
      continue;
    if (available < 0) {
      perror("poll");
      status = 1;
      break;
    }
    if (ready[1].revents & POLLIN) {
      struct signalfd_siginfo info;
      while (read(signals, &info, sizeof info) == sizeof info) {
      }
      reap();
      fflush(stdout);
    }
    if (!(ready[0].revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL)))
      continue;
    char c;
    ssize_t got = lab_read(STDIN_FILENO, &c, 1);
    if (got < 0) {
      perror("stdin");
      status = 1;
      break;
    }
    if (got && c != '\n') {
      if (!c || length == LINE_MAXIMUM)
        discard = true;
      if (!discard)
        line[length++] = c;
      continue;
    }
    line[length] = '\0';
    if (!got)
      exiting = true;
    if (discard) {
      fprintf(stderr, "Invalid or oversized command\n");
      status = 2;
    } else if (length) {
      Token tokens[TOKENS];
      Command commands[STAGES] = {0};
      bool background = false;
      int count = tokenize(line, tokens, storage);
      int stages = count > 0 ? parse(tokens, count, commands, &background) : 0;
      if (count < 0 || stages < 0) {
        fprintf(stderr, "Invalid command syntax\n");
        status = 2;
      } else if (count > 0) {
        char **args = commands[0].argv;
        bool builtin = !strcmp(args[0], "exit") || !strcmp(args[0], "cd") ||
                       !strcmp(args[0], "jobs") || !strcmp(args[0], "fg") ||
                       !strcmp(args[0], "bg");
        if (builtin && (stages != 1 || background || commands[0].input ||
                        commands[0].output)) {
          fprintf(
              stderr,
              "Builtins require a standalone command without redirection\n");
          status = 2;
        } else if (!builtin)
          status = launch(commands, stages, background, line);
        else if (!strcmp(args[0], "exit")) {
          int value = status;
          if (commands[0].argc > 2 ||
              (args[1] &&
               (!lab_parse_int(args[1], 0, &value) || value > 255))) {
            fprintf(stderr, "exit [status 0..255]\n");
            status = 2;
          } else {
            status = value;
            exiting = true;
          }
        } else if (!strcmp(args[0], "cd")) {
          const char *path = args[1] ? args[1] : getenv("HOME");
          status = commands[0].argc > 2 || !path || chdir(path) < 0;
          if (status)
            fprintf(stderr, "cd requires an accessible directory\n");
        } else if (!strcmp(args[0], "jobs")) {
          reap();
          for (int j = 0; j < JOBS; ++j)
            if (jobs[j].used)
              printf("[%d] %s pgid=%ld %s\n", j + 1,
                     stopped(&jobs[j]) ? "stopped" : "running",
                     (long)jobs[j].group, jobs[j].description);
          status = 0;
        } else {
          int id = 0;
          if (commands[0].argc != 2 || !lab_parse_int(args[1], 1, &id) ||
              id > JOBS || !jobs[id - 1].used) {
            fprintf(stderr, "fg/bg requires an active job number from jobs\n");
            status = 2;
          } else if (!strcmp(args[0], "fg"))
            status = foreground(&jobs[id - 1], true);
          else {
            status = kill(-jobs[id - 1].group, SIGCONT) < 0;
            if (status)
              perror("SIGCONT");
            else
              for (int i = 0; i < jobs[id - 1].count; ++i)
                if (jobs[id - 1].states[i] == 1)
                  jobs[id - 1].states[i] = 0;
          }
        }
      }
    }
    length = 0;
    discard = false;
    if (interactive && !exiting) {
      printf("lab13$ ");
      fflush(stdout);
    }
  }
  shutdown_jobs();
  free(storage);
  free(line);
  close(signals);
  sigprocmask(SIG_SETMASK, &original_mask, NULL);
  return status;
}
