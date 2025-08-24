#ifndef CURSOR_H
#define CURSOR_H

extern int cursor_pos;
extern int min_cursor_pos;

void cursor_left(void);
void cursor_right(void);
void cursor_home(void);
void cursor_end(void);
void move_cursor_left(int n);
void move_cursor_right(int n);
void clear_line_from_cursor(void);
void save_cursor_pos(void);
void restore_cursor_pos(void);

#endif
