#ifndef PROCESS_H
#define PROCESS_H

#include <sys/types.h>
#include <termios.h>

typedef struct process {
  struct process *next; /* next process in pipeline */
  char **argv;          /* for exec */
  pid_t pid;            /* process ID */
  char completed;       /* true if process has completed */
  char stopped;         /* true if process has stopped */
  int status;           /* reported status value */
} process;

#endif
