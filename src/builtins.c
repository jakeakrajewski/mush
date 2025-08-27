#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "builtins.h"
#include "job.h"
#include "job_control.h"

int mu_exit_command = 0;

char *builtin_str[] = {
    "cd",
    "help",
    "exit",
    "exec",
    "kill",
    "fg",
    "bg",
    "jobs",
};

int (*builtin_func[])(char **) = {
    &mu_cd,
    &mu_help,
    &mu_exit,
    &mu_exec,
    &mu_kill,
    &mu_fg,
    &mu_bg,
    &mu_jobs,
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

int mu_help() {
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

int mu_kill(char **args) {
  if (args[1] == NULL) {
    fprintf(stderr, "mu: expected argument to \"kill\"\n");
    return 1;
  }

  pid_t pid;
  int sig = SIGTERM; // default signal

  // Check if first argument is a signal
  if (args[1][0] == '-') {
    if (args[2] == NULL) {
      fprintf(stderr, "mu: expected pid after signal\n");
      return 1;
    }
    
    // Parse signal number (skip the '-')
    sig = atoi(&args[1][1]);
    if (sig <= 0) {
      fprintf(stderr, "mu: invalid signal number\n");
      return 1;
    }
    
    pid = atoi(args[2]);
  } else {
    pid = atoi(args[1]);
  }

  if (pid <= 0) {
    fprintf(stderr, "mu: invalid pid\n");
    return 1;
  }

  if (kill(pid, sig) == -1) {
    perror("mu");
    return 1;
  }

  return 0;
}

int mu_fg(char **args) {
  extern job *first_job;
  
  if (!shell_is_interactive) {
    fprintf(stderr, "mu: fg: no job control in this shell\n");
    return 1;
  }

  job *j = NULL;

  if (args[1] == NULL) {
    // No argument - find most recent job that can be foregrounded
    // Priority: stopped jobs first, then running background jobs
    for (j = first_job; j; j = j->next) {
      if (job_is_stopped(j)) {
        break;
      }
    }
    
    // If no stopped job found, look for any running job
    if (!j) {
      for (j = first_job; j; j = j->next) {
        if (!job_is_completed(j)) {
          break;
        }
      }
    }
    
    if (!j) {
      fprintf(stderr, "mu: fg: no current job\n");
      return 1;
    }
  } else {
    // Parse job specification
    int job_num;
    if (args[1][0] == '%') {
      job_num = atoi(&args[1][1]);
    } else {
      job_num = atoi(args[1]);
    }
    
    // Find job by number (position in list)
    int current_num = 1;
    for (j = first_job; j; j = j->next) {
      if (current_num == job_num) {
        break;
      }
      current_num++;
    }
    
    if (!j) {
      fprintf(stderr, "mu: fg: job not found\n");
      return 1;
    }
  }

  // Check if job is still valid before continuing
  if (job_is_completed(j)) {
    fprintf(stderr, "mu: fg: job has already completed\n");
    return 1;
  }
  
  continue_job(j, 1); // 1 = foreground
  return 0;
}

int mu_bg(char **args) {
  extern job *first_job;
  
  if (!shell_is_interactive) {
    fprintf(stderr, "mu: bg: no job control in this shell\n");
    return 1;
  }

  job *j = NULL;

  if (args[1] == NULL) {
    // No argument - find most recent stopped job
    for (j = first_job; j; j = j->next) {
      if (job_is_stopped(j)) {
        break;
      }
    }
    
    if (!j) {
      fprintf(stderr, "mu: bg: no stopped job\n");
      return 1;
    }
  } else {
    // Parse job specification
    int job_num;
    if (args[1][0] == '%') {
      job_num = atoi(&args[1][1]);
    } else {
      job_num = atoi(args[1]);
    }

    // Find job by number (position in list)
    int current_num = 1;
    for (j = first_job; j; j = j->next) {
      if (current_num == job_num) {
        break;
      }
      current_num++;
    }

    if (!j) {
      fprintf(stderr, "mu: bg: job not found\n");
      return 1;
    }
    
    if (!job_is_stopped(j)) {
      fprintf(stderr, "mu: bg: job is already running\n");
      return 1;
    }
  }

  continue_job(j, 0); // 0 = background
  return 0;
}

int mu_jobs() {
  extern job *first_job;
  
  if (!shell_is_interactive) {
    fprintf(stderr, "mu: jobs: no job control in this shell\n");
    return 1;
  }

  job *j;
  int job_num = 1;

  for (j = first_job; j; j = j->next) {
    const char *status;
    
    if (job_is_completed(j)) {
      status = "Done";
    } else if (job_is_stopped(j)) {
      status = "Stopped";
    } else {
      status = "Running";
    }

    printf("[%d]  %s\t\t%s\n", job_num, status, j->command);
    job_num++;
  }

  return 0;
}
