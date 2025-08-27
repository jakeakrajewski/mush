#include <stdio.h>
#include <sys/types.h>
#include <termios.h>

#include "process.h"

typedef struct job {
  struct job *next;          /* next active job */
  char *command;             /* command line, used for messages */
  process *first_process;    /* list of processes in this job */
  pid_t pgid;                /* process group ID */
  char notified;             /* true if user told about stopped job */
  char is_background;        /* true if this is a background job */
  struct termios tmodes;     /* saved terminal modes */
  int stdin, stdout, stderr; /* standard i/o channels */
} job;
