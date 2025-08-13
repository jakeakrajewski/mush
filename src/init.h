#ifndef MU_INIT_H
#define MU_INIT_H

#include <sys/types.h>
#include <termios.h>

pid_t shell_pgid;
struct termios shell_tmodes;
int shell_terminal;
int shell_is_interactive;

void mu_init();
void restore_terminal_control();

#endif /* ifndef  */
