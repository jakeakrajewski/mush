#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>

#include "prompt.h"
#include "history.h"
#include "cursor.h"

char *current_line = NULL;
extern int cursor_pos;
extern int min_cursor_pos;

int line_length = 0;

extern History history;
extern char *temp_line;  // For storing current line when navigating history
extern char *current_line;

// Character reading function
int read_char(void) {
    struct termios oldt, newt;
    int ch;
    
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    ch = getchar();
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

// Insert character at cursor position
void insert_char(char c) {
    if (line_length >= MAX_LINE_LENGTH - 1) return;
    
    // Shift characters to the right
    memmove(current_line + cursor_pos + 1, current_line + cursor_pos, line_length - cursor_pos);
    current_line[cursor_pos] = c;
    line_length++;
    cursor_pos++;
    
    // If we're at the end of the line, just print the character
    if (cursor_pos == line_length) {
        printf("%c", c);
    } else {
        // We're in the middle - need to reprint from cursor position
        for (int i = cursor_pos - 1; i < line_length; i++) {
            printf("%c", current_line[i]);
        }
        // Move cursor back to correct position
        if (line_length > cursor_pos) {
            printf("\033[%dD", line_length - cursor_pos);
        }
    }
    fflush(stdout);
}

// Delete character at cursor position
void delete_char() {
    if (cursor_pos >= line_length) return;
    
    memmove(current_line + cursor_pos, current_line + cursor_pos + 1, line_length - cursor_pos - 1);
    line_length--;
    
    // Clear from cursor to end of line and reprint
    printf("\033[K");
    for (int i = cursor_pos; i < line_length; i++) {
        printf("%c", current_line[i]);
    }
    // Move cursor back to original position
    if (line_length > cursor_pos) {
        printf("\033[%dD", line_length - cursor_pos);
    }
    fflush(stdout);
}

// Delete character before cursor (backspace)
void backspace_char() {
    if (cursor_pos <= min_cursor_pos) return;  // Don't delete past prompt
    
    cursor_pos--;
    memmove(current_line + cursor_pos, current_line + cursor_pos + 1, line_length - cursor_pos - 1);
    line_length--;
    
    // Move cursor back, clear from cursor to end, reprint remainder
    printf("\b\033[K");
    for (int i = cursor_pos; i < line_length; i++) {
        printf("%c", current_line[i]);
    }
    // Move cursor back to correct position
    if (line_length > cursor_pos) {
        printf("\033[%dD", line_length - cursor_pos);
    }
    fflush(stdout);
}



// Replace current line with new content
void replace_line(const char *new_content) {
    // Move cursor to beginning of input area
    if (cursor_pos > min_cursor_pos) {
        printf("\033[%dD", cursor_pos - min_cursor_pos);
    }
    
    // Clear from cursor to end of line
    printf("\033[K");
    
    // Copy new content and print it
    strcpy(current_line + min_cursor_pos, new_content);
    line_length = min_cursor_pos + strlen(new_content);
    printf("%s", new_content);
    cursor_pos = line_length;
    
    fflush(stdout);
}



// Add these helper functions for directory completion

// Check if a path is a directory
int is_directory(const char *path) {
    struct stat statbuf;
    return (stat(path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode));
}

// Get directory entries matching a prefix
int get_directory_matches(const char *dir_path, const char *prefix, char matches[][256], int max_matches) {
    DIR *dir;
    struct dirent *entry;
    int count = 0;
    int prefix_len = strlen(prefix);
    
    dir = opendir(dir_path);
    if (!dir) return 0;
    
    while ((entry = readdir(dir)) != NULL && count < max_matches) {
        // Skip . and .. unless explicitly typed
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            if (prefix_len == 0 || (prefix[0] != '.' || 
                (prefix_len == 1 && prefix[0] == '.') ||
                (prefix_len == 2 && strncmp(prefix, "..", 2) == 0))) {
                continue;
            }
        }
        
        // Skip hidden files unless prefix starts with .
        if (entry->d_name[0] == '.' && prefix_len > 0 && prefix[0] != '.') {
            continue;
        }
        
        // Check if name matches prefix
        if (strncmp(entry->d_name, prefix, prefix_len) == 0) {
            strcpy(matches[count], entry->d_name);
            count++;
        }
    }
    
    closedir(dir);
    return count;
}

// Extract directory and filename from a path
void split_path(const char *path, char *dir, char *filename) {
    const char *last_slash = strrchr(path, '/');
    
    if (last_slash) {
        // Copy directory part
        int dir_len = last_slash - path;
        if (dir_len == 0) {
            strcpy(dir, "/");  // Root directory
        } else {
            strncpy(dir, path, dir_len);
            dir[dir_len] = '\0';
        }
        
        // Copy filename part
        strcpy(filename, last_slash + 1);
    } else {
        // No slash, current directory
        strcpy(dir, ".");
        strcpy(filename, path);
    }
}

// Check if we should do file/directory completion vs command completion
int should_complete_path(const char *line, int cursor_pos, int min_cursor_pos) {
    // If we're not at the beginning of the line, it's likely a path
    if (cursor_pos > min_cursor_pos) {
        // Find the start of the current word
        int word_start = cursor_pos - 1;
        while (word_start > min_cursor_pos && line[word_start - 1] != ' ' && line[word_start - 1] != '\t') {
            word_start--;
        }
        
        // If this is not the first word, it's likely a path
        if (word_start > min_cursor_pos) {
            return 1;
        }
        
        // Check if the current word contains a path separator
        for (int i = word_start; i < cursor_pos; i++) {
            if (line[i] == '/' || line[i] == '~') {
                return 1;
            }
        }
    }
    
    return 0;
}

// Expand ~ to home directory
void expand_home(const char *path, char *expanded) {
    if (path[0] == '~') {
        const char *home = getenv("HOME");
        if (home) {
            strcpy(expanded, home);
            strcat(expanded, path + 1);
        } else {
            strcpy(expanded, path);
        }
    } else {
        strcpy(expanded, path);
    }
}

// Updated tab completion function
void handle_tab_completion() {
    if (cursor_pos == min_cursor_pos) return;  // No input to complete
    
    // Extract the current word being typed
    int word_start = cursor_pos - 1;
    while (word_start > min_cursor_pos && current_line[word_start - 1] != ' ' && current_line[word_start - 1] != '\t') {
        word_start--;
    }
    
    int word_len = cursor_pos - word_start;
    if (word_len == 0) return;
    
    char word[512];
    strncpy(word, current_line + word_start, word_len);
    word[word_len] = '\0';
    
    // Determine if we should complete commands or paths
    if (should_complete_path(current_line, cursor_pos, min_cursor_pos)) {
        // Path completion
        char expanded_word[512];
        expand_home(word, expanded_word);
        
        char dir[512], filename[256];
        split_path(expanded_word, dir, filename);
        
        char matches[100][256];
        int match_count = get_directory_matches(dir, filename, matches, 100);
        
        if (match_count == 0) {
            printf("\a");  // Beep for no matches
            fflush(stdout);
            return;
        }
        
        if (match_count == 1) {
            // Single match - complete it
            const char *completion = matches[0];
            int completion_len = strlen(completion);
            int filename_len = strlen(filename);
            int chars_to_add = completion_len - filename_len;
            
            if (chars_to_add > 0 && line_length + chars_to_add < MAX_LINE_LENGTH - 1) {
                // Calculate the full path for the match
                char full_completion[512];
                if (strcmp(dir, ".") == 0) {
                    strcpy(full_completion, completion);
                } else if (strcmp(dir, "/") == 0) {
                    snprintf(full_completion, sizeof(full_completion), "/%s", completion);
                } else {
                    snprintf(full_completion, sizeof(full_completion), "%s/%s", dir, completion);
                }
                
                // Check if it's a directory and add trailing slash
                int is_dir = is_directory(full_completion);
                if (is_dir) {
                    if (chars_to_add < MAX_LINE_LENGTH - line_length - 2) {
                        chars_to_add++;
                    }
                }
                
                // Make space for the completion
                memmove(current_line + cursor_pos + chars_to_add, current_line + cursor_pos, line_length - cursor_pos);
                
                // Insert the completion
                memcpy(current_line + word_start + (word_len - filename_len), completion, completion_len);
                
                // Add trailing slash for directories
                if (is_dir && chars_to_add > completion_len - filename_len) {
                    current_line[word_start + (word_len - filename_len) + completion_len] = '/';
                }
                
                line_length += chars_to_add;
                cursor_pos += chars_to_add;
                
                // Reprint from the word start
                printf("\033[%dD", cursor_pos - word_start - chars_to_add);  // Move to word start
                for (int i = word_start; i < line_length; i++) {
                    printf("%c", current_line[i]);
                }
                // Move cursor back to correct position
                if (line_length > cursor_pos) {
                    printf("\033[%dD", line_length - cursor_pos);
                }
            }
        } else {
            // Multiple matches - show them
            printf("\n");
            for (int i = 0; i < match_count; i++) {
                // Add visual indicator for directories
                char display_name[300];
                char full_path[512];
                
                if (strcmp(dir, ".") == 0) {
                    strcpy(full_path, matches[i]);
                } else if (strcmp(dir, "/") == 0) {
                    snprintf(full_path, sizeof(full_path), "/%s", matches[i]);
                } else {
                    snprintf(full_path, sizeof(full_path), "%s/%s", dir, matches[i]);
                }
                
                if (is_directory(full_path)) {
                    snprintf(display_name, sizeof(display_name), "%s/", matches[i]);
                } else {
                    strcpy(display_name, matches[i]);
                }
                
                printf("%-20s", display_name);
                if ((i + 1) % 4 == 0) printf("\n");  // 4 per line for paths
            }
            if (match_count % 4 != 0) printf("\n");
            
            // Reprint prompt and current line
            print_prompt();
            for (int i = min_cursor_pos; i < line_length; i++) {
                printf("%c", current_line[i]);
            }
            // Move cursor to correct position
            if (cursor_pos < line_length) {
                printf("\033[%dD", line_length - cursor_pos);
            }
        }
    } else {
        const char *commands[] = {
            "ls", "cd", "pwd", "echo", "cat", "grep", "mkdir", "rmdir", "rm", "cp", "mv", "chmod", "chown",
            "ps", "kill", "jobs", "fg", "bg", "history", "exit", "help", "clear", "date", "whoami", "which",
            "find", "wc", "sort", "uniq", "head", "tail", "less", "more", "man", "export", "env", "alias",
            NULL
        };
        
        // Find matches
        const char *matches[50];
        int match_count = 0;
        
        for (int i = 0; commands[i] != NULL && match_count < 49; i++) {
            if (strncmp(commands[i], word, word_len) == 0) {
                matches[match_count++] = commands[i];
            }
        }
        
        if (match_count == 0) {
            printf("\a");
            fflush(stdout);
            return;
        }
        
        if (match_count == 1) {
            // Single match - complete it
            const char *completion = matches[0];
            int completion_len = strlen(completion);
            int chars_to_add = completion_len - word_len;
            
            if (chars_to_add > 0 && line_length + chars_to_add < MAX_LINE_LENGTH - 1) {
                // Make space for the completion
                memmove(current_line + cursor_pos + chars_to_add, current_line + cursor_pos, line_length - cursor_pos);
                
                // Insert the completion
                memcpy(current_line + word_start, completion, completion_len);
                
                // Add a space after the command
                if (word_start == min_cursor_pos) {  // This is the first word (command)
                    current_line[word_start + completion_len] = ' ';
                    chars_to_add++;
                    completion_len++;
                }
                
                line_length += chars_to_add;
                cursor_pos += chars_to_add;
                
                // Reprint from the word start
                printf("\033[%dD", cursor_pos - word_start - chars_to_add);  // Move to word start
                for (int i = word_start; i < line_length; i++) {
                    printf("%c", current_line[i]);
                }
                // Move cursor back to correct position
                if (line_length > cursor_pos) {
                    printf("\033[%dD", line_length - cursor_pos);
                }
            }
        } else {
            // Multiple matches - show them
            printf("\n");
            for (int i = 0; i < match_count; i++) {
                printf("%s  ", matches[i]);
                if ((i + 1) % 8 == 0) printf("\n");  // 8 per line
            }
            if (match_count % 8 != 0) printf("\n");
            
            // Reprint prompt and current line
            print_prompt();
            for (int i = min_cursor_pos; i < line_length; i++) {
                printf("%c", current_line[i]);
            }
            // Move cursor to correct position
            if (cursor_pos < line_length) {
                printf("\033[%dD", line_length - cursor_pos);
            }
        }
    }
    
    fflush(stdout);
}

void handle_printable(int c) {
    insert_char((char)c);
}

void handle_special(int c) {
    if (c == 27) {  // ESC sequence
        int c2 = read_char();
        if (c2 == '[') {  // CSI sequence
            int c3 = read_char();
            switch (c3) {
                case 'A':  // Up arrow
                    {
                        char *hist_line = get_history_entry(1);
                        if (hist_line) {
                            replace_line(hist_line);
                            free(hist_line);
                        }
                    }
                    break;
                case 'B':  // Down arrow
                    {
                        char *hist_line = get_history_entry(-1);
                        if (hist_line) {
                            replace_line(hist_line);
                            free(hist_line);
                        }
                    }
                    break;
                case 'C':  // Right arrow
                    cursor_right();
                    break;
                case 'D':  // Left arrow
                    cursor_left();
                    break;
                case 'H':  // Home
                    cursor_home();
                    break;
                case 'F':  // End
                    cursor_end();
                    break;
                case '3':  // Delete key
                    if (read_char() == '~') {
                        delete_char();
                    }
                    break;
            }
        } else if (c2 == 'O') {  // Alternative sequences
            int c3 = read_char();
            switch (c3) {
                case 'H':  // Home
                    cursor_home();
                    break;
                case 'F':  // End
                    cursor_end();
                    break;
            }
        }
    } else if (c == 127 || c == 8) {  // Backspace
        backspace_char();
    } else if (c == 4) {  // Ctrl+D (EOF)
        if (line_length == min_cursor_pos) {
            // Empty line, exit
            printf("\n");
            exit(0);
        } else {
            delete_char();
        }
    } else if (c == 3) {  // Ctrl+C
        printf("^C\n");
        // Clear line and start fresh
        line_length = min_cursor_pos;
        cursor_pos = min_cursor_pos;
        current_line[line_length] = '\0';
        print_prompt();
        fflush(stdout);
    } else if (c == 12) {  // Ctrl+L (clear screen)
        printf("\033[H\033[2J");  // Clear screen and move to top
        print_prompt();
        // Reprint current line
        for (int i = min_cursor_pos; i < line_length; i++) {
            printf("%c", current_line[i]);
        }
        // Move cursor to correct position
        if (cursor_pos < line_length) {
            printf("\033[%dD", line_length - cursor_pos);
        }
        fflush(stdout);
    }
}

void handle_char(int c) {
    if (c == '\t') {  // Tab key
        handle_tab_completion();
    } else if (isprint((unsigned char)c)) {
        handle_printable(c);
    } else {
        handle_special(c);
    }
}

char *promptly_loop() {
    size_t max_chars = MAX_LINE_LENGTH;
    current_line = malloc(max_chars);
    memset(current_line, 0, max_chars);
    
    // Don't call print_prompt here - it should be called by the shell before this function
    
    // Set initial positions
    min_cursor_pos = 0;
    cursor_pos = min_cursor_pos;
    line_length = min_cursor_pos;
    
    for (;;) {
        int c = read_char();
        
        if (c == '\n' || c == '\r') {  // Enter pressed
            printf("\n");
            
            // Extract the actual command (after prompt)
            char *command = current_line + min_cursor_pos;
            int cmd_len = line_length - min_cursor_pos;
            
            // Trim whitespace
            while (cmd_len > 0 && isspace(command[cmd_len - 1])) {
                cmd_len--;
            }
            command[cmd_len] = '\0';
            
            while (*command && isspace(*command)) {
                command++;
                cmd_len--;
            }
            
            // Add to history if non-empty
            if (strlen(command) > 0) {
                add_to_history(command);
            }
            
            // Return the command
            char *result = malloc(cmd_len + 1);
            strcpy(result, command);
            
            free(current_line);
            if (temp_line) {
                free(temp_line);
                temp_line = NULL;
            }
            
            return result;
        }
        
        handle_char(c);
    }
}

