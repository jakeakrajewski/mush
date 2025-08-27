#ifndef BUILTINS_H
#define BUILTINS_H

#include <sys/types.h>

// External declarations for variables defined in main.c/job_control.c
extern int shell_is_interactive;

// Forward declarations for structs (so you don't need to include other headers)
typedef struct job job;

// Builtin function declarations
int mu_cd(char **args);
int mu_help();
int mu_exit(char **args);
int mu_exec(char **args);
int mu_kill(char **args);
int mu_fg(char **args);
int mu_bg(char **args);
int mu_jobs();

// Builtin management
int mu_num_builtins(void);

// External builtin arrays
extern char *builtin_str[];
extern int (*builtin_func[])(char **);

// External variables
extern int mu_exit_command;

#endif // BUILTINS_H
