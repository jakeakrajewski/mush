#ifndef HISTORY_H 
#define HISTORY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>

// Constants
#define MAX_HISTORY 1000
#define MAX_LINE_LENGTH 1024

// History structure
typedef struct {
    char **entries;
    int count;
    int capacity;
    int current_index;
} History;

extern History history;
extern char *temp_line;  // For storing current line when navigating history
extern char *current_line;

// History management functions
void init_history(void);
void add_to_history(const char *line);
char *get_history_entry(int direction);
void cleanup_history(void);

#endif /* HISTORY_H */
