
#include "builtins.h"
#include "job_control.h"
#include <sys/wait.h>
#include <signal.h>

#include <unistd.h>
extern int shell_is_interactive;
extern int shell_terminal;
extern int shell_pgid;

volatile sig_atomic_t job_notification_pending = 0;

void sigchld_handler(int sig) {
    (void)sig;
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        mark_process_status(pid, status);
        job_notification_pending = 1;
    }
}

void sigint_handler(int sig) {
    (void)sig;
    // Forward SIGINT to foreground job's process group (if any)
    if (shell_is_interactive) {
        // Send SIGINT to the foreground process group
        pid_t fg_pgid = tcgetpgrp(shell_terminal);
        if (fg_pgid != shell_pgid) { // don't send to shell itself
            kill(-fg_pgid, SIGINT);
        }
    }
}

void sigtstp_handler(int sig) {
    (void)sig;
    // Forward SIGTSTP (Ctrl-Z) to foreground job's process group
    if (shell_is_interactive) {
        pid_t fg_pgid = tcgetpgrp(shell_terminal);
        if (fg_pgid != shell_pgid) {
            kill(-fg_pgid, SIGTSTP);
        }
    }
}

// Setup all signal handlers
void setup_signal_handlers(void) {
    struct sigaction sa_int, sa_tstp, sa_chld;

    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa_int, NULL);

    sa_tstp.sa_handler = sigtstp_handler;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa_tstp, NULL);

    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa_chld, NULL);
}
