#ifndef PROMPTLY_H
#define PROMPTLY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>

// External variables (to be defined in your shell)
extern int mu_last_status;
extern size_t lines_count;
extern int line_length;

// Core line editor functions
char *promptly_loop(void);

// Line editing functions
void print_line_with_highlighting(void);
void insert_char(char c);
void delete_char(void);
void backspace_char(void);
void replace_line(const char *new_content);

// Character handling functions
int read_char(void);
void handle_printable(int c);
void handle_special(int c);
void handle_char(int c);
void handle_tab_completion(void);

#endif /* PROMPTLY_H */
