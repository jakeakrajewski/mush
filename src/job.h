#ifndef JOB_H
#define JOB_H

#include <sys/types.h>
#include <termios.h>

#include "process.h"

typedef struct job {
  struct job *next;
  char *command;
  process *first_process;
  pid_t pgid;
  char notified;
  struct termios tmodes;
  int stdin, stdout, stderr;
} job;

#endif
