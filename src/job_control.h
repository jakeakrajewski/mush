#ifndef JOB_CONTROL_H
#define JOB_CONTROL_H

#include <sys/types.h>
#include <termios.h>

#include "job.h"

extern job *first_job;

void launch_job(job *j, int foreground);
void continue_job(job *j, int foreground);
void do_job_notification(void);
job *find_job(pid_t pgid);
void free_job(job *j);
void wait_for_job(job *j);
int job_is_completed(job *j);
int job_is_stopped(job *j);
void sigchld_handler(int sig);
void setup_signal_handlers();
int find_job_by_number(int job_num, job **found_job);
void put_job_in_foreground(job *j, int cont);
void put_job_in_background(job *j, int cont);
void format_job_info(job *j, const char *status);
int mark_process_status(pid_t pid, int status);

#endif
