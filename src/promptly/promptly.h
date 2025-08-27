#ifndef PROMPTLY_H
#define PROMPTLY_H

#define MAX_LINE_LENGTH 1024

#include <stddef.h>

// Function declarations from promptly.c
char *promptly_loop();
int read_char(void);
void insert_char(char c);
void delete_char();
void backspace_char();
void replace_line(const char *new_content);
void handle_tab_completion();
void handle_printable(int c);
void handle_special(int c);
void handle_char(int c);

// Function declarations from prompt.c
void print_prompt();

// Function declarations from history.c
void init_history();
void add_to_history(const char *line);
char *get_history_entry(int direction);
void cleanup_history();

// Function declarations from cursor.c
void move_cursor_left(int n);
void move_cursor_right(int n);
void clear_line_from_cursor();
void save_cursor_pos();
void restore_cursor_pos();
void cursor_left();
void cursor_right();
void cursor_home();
void cursor_end();

// Function declarations from config.c
void init_config();
int load_config();
int save_config();
const char* get_color(const char* color_code);

// Global variables
extern char *current_line;
extern int line_length;
extern int cursor_pos;
extern int min_cursor_pos;
extern int mu_last_status;
extern size_t lines_count;

#endif // PROMPTLY_H
