#ifndef SIGNAL_HANDLERS_H
#define SIGNAL_HANDLERS_H
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern volatile sig_atomic_t job_notification_pending;
void sigchld_handler(int sig);
void sigint_handler(int sig);
void sigtstp_handler(int sig);
void setup_signal_handlers(void);

#endif
