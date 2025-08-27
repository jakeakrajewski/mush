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
#include <unistd.h>

#include "builtins.h"

extern int debug_substitution;

int mu_launch(char **args) {
  pid_t pid;
  int status;

  if (debug_substitution) {
    fprintf(stderr, "DEBUG mu_launch: About to fork for command: %s\n",
            args[0] ? args[0] : "(null)");
    for (int i = 0; args[i]; i++) {
      fprintf(stderr, "DEBUG mu_launch: args[%d] = '%s'\n", i, args[i]);
    }
  }

  pid = fork();
  if (pid == 0) {
    // Child process
    if (debug_substitution) {
      fprintf(stderr, "DEBUG mu_launch: Child process calling execvp with %s\n",
              args[0]);
    }

    if (execvp(args[0], args) == -1) {
      fprintf(stderr, "DEBUG mu_launch: execvp failed: %s\n", strerror(errno));
      perror("mu");
      exit(EXIT_FAILURE);
    }
    // This should never be reached if execvp succeeds
    exit(EXIT_SUCCESS);
  } else if (pid < 0) {
    perror("mu");
    return 1;
  } else {
    // Parent process
    do {
      // wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));

    int exit_status = WEXITSTATUS(status);
    if (debug_substitution) {
      fprintf(stderr, "DEBUG mu_launch: Child exited with status %d\n",
              exit_status);
    }
    return exit_status;
  }
}

int mu_execute(char **args) {
  if (args[0] == NULL)
    return 0; // Empty command

  if (debug_substitution) {
    fprintf(stderr, "DEBUG mu_execute: Executing command: %s\n", args[0]);
    for (int i = 0; args[i]; i++) {
      fprintf(stderr, "DEBUG mu_execute: args[%d] = '%s'\n", i, args[i]);
    }
  }

  int negate = 0;
  if (strcmp(args[0], "!") == 0) {
    negate = 1;
    args++;
    if (args[0] == NULL) {
      fprintf(stderr, "mu: syntax error: ! must be followed by a command\n");
      return 1;
    }
  }

  // Check for built-ins
  for (int i = 0; i < mu_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      int status = (*builtin_func[i])(args);
      return negate ? !status : status;
    }
  }

  int status = mu_launch(args);
  return negate ? !status : status;
}
