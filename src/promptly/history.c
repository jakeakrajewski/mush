#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_HISTORY 1000
#define MAX_LINE_LENGTH 1024

typedef struct {
    char **entries;
    int count;
    int capacity;
    int current_index;
} History;

History history = {NULL, 0, 0, -1};
char *temp_line = NULL;  // For storing current line when navigating history
char *current_line;

// History functions
void init_history() {
    if (history.entries == NULL) {
        history.capacity = MAX_HISTORY;
        history.entries = malloc(history.capacity * sizeof(char*));
        history.count = 0;
        history.current_index = -1;
    }
}

void add_to_history(const char *line) {
    if (!line || strlen(line) == 0) return;
    
    init_history();
    
    // Don't add duplicate consecutive entries
    if (history.count > 0 && strcmp(history.entries[history.count - 1], line) == 0) {
        return;
    }
    
    // If at capacity, shift all entries left and overwrite oldest
    if (history.count >= history.capacity) {
        free(history.entries[0]);
        memmove(history.entries, history.entries + 1, (history.capacity - 1) * sizeof(char*));
        history.count--;
    }
    
    // Add new entry
    history.entries[history.count] = strdup(line);
    history.count++;
    history.current_index = -1;  // Reset to indicate we're not browsing history
}

char *get_history_entry(int direction) {
    init_history();
    
    if (history.count == 0) return NULL;
    
    if (direction == 1) {  // Up arrow - go back in history
        if (history.current_index == -1) {
            // First time browsing history, save current line
            if (temp_line) free(temp_line);
            temp_line = strdup(current_line);
            history.current_index = history.count - 1;
        } else if (history.current_index > 0) {
            history.current_index--;
        } else {
            return NULL;  // Already at oldest entry
        }
        return strdup(history.entries[history.current_index]);
    } else if (direction == -1) {  // Down arrow - go forward in history
        if (history.current_index == -1) {
            return NULL;  // Not browsing history
        } else if (history.current_index < history.count - 1) {
            history.current_index++;
            return strdup(history.entries[history.current_index]);
        } else {
            // Return to current line
            history.current_index = -1;
            char *result = temp_line;
            temp_line = NULL;
            return result;
        }
    }
    
    return NULL;
}

// Cleanup function (call this before program exit)
void cleanup_history() {
    if (history.entries) {
        for (int i = 0; i < history.count; i++) {
            free(history.entries[i]);
        }
        free(history.entries);
        history.entries = NULL;
        history.count = 0;
        history.capacity = 0;
    }
    if (temp_line) {
        free(temp_line);
        temp_line = NULL;
    }
}
