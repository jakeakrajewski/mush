#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "execute.h"
#include "tokenizer.h"

extern int debug_substitution;

char *execute_substitution(ASTNode *node) {
  if (!node)
    return strdup("");

  if (debug_substitution) {
    fprintf(stderr, "DEBUG: About to execute substitution\n");
    if (node->type == NODE_COMMAND) {
      fprintf(stderr, "DEBUG: Command has %d args\n", node->argc);
      for (int i = 0; i < node->argc; i++) {
        if (node->args[i].is_substitution) {
          fprintf(stderr, "DEBUG: Arg %d is substitution\n", i);
        } else {
          fprintf(stderr, "DEBUG: Arg %d: '%s'\n", i, node->args[i].text);
        }
      }
    }
  }

  // Create a pipe for inter-process communication
  int pipefd[2];
  if (pipe(pipefd) == -1) {
    perror("pipe");
    return strdup("");
  }

  pid_t pid = fork();
  if (pid == 0) {
    // Child process
    close(pipefd[0]); // Close read end

    // Redirect stdout to the write end of the pipe
    if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
      perror("dup2");
      exit(EXIT_FAILURE);
    }
    close(pipefd[1]); // Close the original write end

    // Don't redirect stderr to /dev/null during debugging
    if (!debug_substitution) {
      int devnull = open("/dev/null", O_WRONLY);
      if (devnull != -1) {
        dup2(devnull, STDERR_FILENO);
        close(devnull);
      }
    }

    // Execute the command
    if (debug_substitution) {
      fprintf(stderr,
              "DEBUG: Child about to call execute() for substitution\n");
    }
    int status = execute(node, 0);
    if (debug_substitution) {
      fprintf(stderr, "DEBUG: Child execute() returned status %d\n", status);
    }
    exit(status);
  } else if (pid > 0) {
    // Parent process
    close(pipefd[1]); // Close write end

    // Read all data from the pipe
    char *output = malloc(4096);
    if (!output) {
      perror("malloc");
      close(pipefd[0]);
      waitpid(pid, NULL, 0);
      return strdup("");
    }

    size_t total = 0;
    size_t capacity = 4096;
    ssize_t n;

    while ((n = read(pipefd[0], output + total, capacity - total - 1)) > 0) {
      total += n;
      if (total >= capacity - 1) {
        capacity *= 2;
        char *new_output = realloc(output, capacity);
        if (!new_output) {
          perror("realloc");
          free(output);
          close(pipefd[0]);
          waitpid(pid, NULL, 0);
          return strdup("");
        }
        output = new_output;
      }
    }

    output[total] = '\0';
    close(pipefd[0]);

    int wstatus;
    waitpid(pid, &wstatus, 0);

    if (debug_substitution) {
      fprintf(stderr, "DEBUG: Child exit status: %d\n", WEXITSTATUS(wstatus));
      fprintf(stderr, "DEBUG: Captured output: '%s' (length: %zu)\n", output,
              total);
    }

    // Strip trailing newlines
    while (total > 0 &&
           (output[total - 1] == '\n' || output[total - 1] == '\r')) {
      output[--total] = '\0';
    }

    return output;

  } else {
    // Fork failed
    perror("fork");
    close(pipefd[0]);
    close(pipefd[1]);
    return strdup("");
  }
}
