#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "builtins.h"
#include "job.h"

int mu_exit_command = 0;

char *builtin_str[] = {
    "cd",
    "help",
    "exit",
    "exec",
};

int (*builtin_func[])(char **) = {
    &mu_cd,
    &mu_help,
    &mu_exit,
    &mu_exec,
};

int mu_num_builtins() { return sizeof(builtin_str) / sizeof(char *); }

int mu_cd(char **args) {
  if (args[1] == NULL) {
    fprintf(stderr, "mu: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("mu");
    } else {
      return 0;
    }
  }
  return 1;
}

int mu_help(char **args) {
  printf("mu shell — type program names and arguments, then hit enter.\n");
  printf("The following are built in:\n");
  for (int i = 0; i < mu_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }
  printf("Use the man command for external programs.\n");
  return 1;
}

int mu_exit(char **args) {
  mu_exit_command = 1;

  if (args[1] != NULL) {
    return atoi(args[1]); // exit status from argument
  }

  return 0; // default to 0 if no argument given
}

int mu_exec(char **args) {
  if (args[1] == NULL) {
    fprintf(stderr, "mu: expected argument to \"exec\"\n");
    return 1;
  }

  // Skip the "exec" command itself
  if (execvp(args[1], &args[1]) == -1) {
    perror("mu");
  }

  // If execvp succeeds, this never runs
  return 1;
}
