#include "prompt.h"
#include <stdio.h>
#include <time.h>

extern int mu_last_status;
extern size_t lines_count;

void print_prompt() {
  if (lines_count > 0) {
    printf("> ");
    fflush(stdout);
    return;
  }

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  char timestr[6];
  strftime(timestr, sizeof(timestr), "%H:%M", t);

  const char *status_symbol =
      (mu_last_status == 0) ? "\033[32m✓\033[0m" : "\033[31m✗\033[0m";
  printf("\033[33m%s\033[0m %s μ> ", timestr, status_symbol);
  fflush(stdout);
}
