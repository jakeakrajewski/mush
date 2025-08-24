#include <stdio.h>

#include "promptly.h"

extern int line_length;
int cursor_pos = 0;
int min_cursor_pos = 0;

// Terminal control functions
void move_cursor_left(int n) {
    if (n > 0) printf("\033[%dD", n);
}

void move_cursor_right(int n) {
    if (n > 0) printf("\033[%dC", n);
}

void clear_line_from_cursor() {
    printf("\033[K");
}

void save_cursor_pos() {
    printf("\033[s");
}

void restore_cursor_pos() {
    printf("\033[u");
}

// Move cursor left
void cursor_left() {
    if (cursor_pos > min_cursor_pos) {
        cursor_pos--;
        move_cursor_left(1);
    }
}

// Move cursor right
void cursor_right() {
    if (cursor_pos < line_length) {
        cursor_pos++;
        move_cursor_right(1);
    }
}

// Move cursor to beginning of line (after prompt)
void cursor_home() {
    move_cursor_left(cursor_pos - min_cursor_pos);
    cursor_pos = min_cursor_pos;
}

// Move cursor to end of line
void cursor_end() {
    move_cursor_right(line_length - cursor_pos);
    cursor_pos = line_length;
}
