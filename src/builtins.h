// Add this to your builtins.h file

#ifndef BUILTINS_H
#define BUILTINS_H

// External declarations for variables defined in main.c/job.c
extern int shell_is_interactive;
extern struct job *first_job; // You'll also need this for fg/bg commands

// Forward declaration for job struct (so you don't need to include job.h)
struct job;

// Builtin function declarations
int mu_cd(char **args);
int mu_help(char **args);
int mu_exit(char **args);
int mu_exec(char **args);
int mu_kill(char **args);
int mu_fg(char **args);
int mu_bg(char **args);

// Builtin management
int mu_num_builtins(void);

// External builtin arrays
extern char *builtin_str[];
extern int (*builtin_func[])(char **);

#endif // BUILTINS_H
