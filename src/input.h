
#ifndef INPUT_H
#define INPUT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global variables — declared, not defined
extern char **lines;
extern size_t lines_count;
extern size_t lines_capacity;

int is_partial_input(const char *str);
void remove_trailing_backslash(char *str);
void add_line(const char *input);
void free_lines(void);
char *join_lines(void);
char *mu_read_line(void);
char *trim_whitespace(char *str);

#endif
