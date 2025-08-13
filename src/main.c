#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins.h"
#include "execute.h"
#include "init.h"
#include "input.h"
#include "prompt.h"

#include "job_control.h"

#define MU_RL_BUFSIZE 1024
#define MU_TOK_BUFSIZE 64
#define MU_TOK_DELIM " \t\r\n\a"

int mu_last_status = 0;
extern int mu_exit_command;
extern volatile sig_atomic_t job_notification_pending;
 

int debug_substitution = 0;

int main() {
    mu_init();

    setup_signal_handlers();

    char *line;

    while (1) {
        if (mu_exit_command)
            break;

        restore_terminal_control();

        // Notify immediately if any job status changed (set by SIGCHLD handler)
        if (job_notification_pending) {
            do_job_notification();
            job_notification_pending = 0;
        }

        print_prompt();
        line = mu_read_line();

        if (line && strlen(line) > 1) {
            // Debug toggle
            if (strncmp(line, "debug", 5) == 0) {
                debug_substitution = !debug_substitution;
                printf("Debug substitution: %s\n", debug_substitution ? "ON" : "OFF");
                free(line);
                continue;
            }

            if (is_partial_input(line)) {
                remove_trailing_backslash(line);
                add_line(line);
                free(line);
            } else {
                add_line(line);
                char *input = join_lines();
                mu_last_status = mu_execute_logical_commands(input);
                free_lines();
                free(line);
                free(input);
            }
        } else {
            free(line);
        }
    }

    return EXIT_SUCCESS;
}
