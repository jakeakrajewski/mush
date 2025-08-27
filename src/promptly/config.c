#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

#include "config.h"

// Global configuration instance
Config config = {0};

// Default configuration values
static void set_default_config() {
    // Prompt format: %t = time, %s = status, %u = username, %h = hostname, %w = working directory
    strcpy(config.prompt_format, "%t %s μ> ");
    
    // Default colors (ANSI escape codes) - use actual escape characters, not literal strings
    strcpy(config.color_time, "\033[33m");      // Yellow
    strcpy(config.color_success, "\033[32m");   // Green
    strcpy(config.color_error, "\033[31m");     // Red
    strcpy(config.color_username, "\033[36m");  // Cyan
    strcpy(config.color_hostname, "\033[35m");  // Magenta
    strcpy(config.color_directory, "\033[34m"); // Blue
    strcpy(config.color_prompt, "\033[0m");     // Reset/White
    strcpy(config.color_reset, "\033[0m");      // Reset
    
    // Default symbols
    strcpy(config.success_symbol, "✓");
    strcpy(config.error_symbol, "✗");
    
    // Default settings
    config.show_time = 1;
    config.show_status = 1;
    config.show_username = 0;
    config.show_hostname = 0;
    config.show_directory = 0;
    config.use_colors = 1;
    config.multiline_prompt = 0;
}

// Create config directory if it doesn't exist
static int create_config_directory() {
    const char *home = getenv("HOME");
    if (!home) {
        return -1;
    }
    
    char config_dir[512];
    snprintf(config_dir, sizeof(config_dir), "%s/.config", home);
    
    // Create .config directory
    if (mkdir(config_dir, 0755) != 0 && errno != EEXIST) {
        return -1;
    }
    
    // Create mu directory
    snprintf(config_dir, sizeof(config_dir), "%s/.config/mu", home);
    if (mkdir(config_dir, 0755) != 0 && errno != EEXIST) {
        return -1;
    }
    
    return 0;
}

// Helper function to process escape sequences
static void process_escape_sequences(char *str) {
    char *src = str, *dst = str;
    while (*src) {
        if (*src == '\\' && *(src + 1)) {
            if (*(src + 1) == '0' && *(src + 2) == '3' && *(src + 3) == '3') {
                // Convert \033 to actual ESC character
                *dst++ = '\033';
                src += 4;
            } else {
                switch (*(src + 1)) {
                    case '\\': *dst++ = '\\'; src += 2; break;
                    case 'n': *dst++ = '\n'; src += 2; break;
                    case 't': *dst++ = '\t'; src += 2; break;
                    case 'r': *dst++ = '\r'; src += 2; break;
                    default: 
                        *dst++ = *src++;
                        break;
                }
            }
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

// Parse a configuration line
static void parse_config_line(char *line) {
    // Remove trailing newline and whitespace
    char *end = line + strlen(line) - 1;
    while (end > line && (*end == '\n' || *end == '\r' || *end == ' ' || *end == '\t')) {
        *end = '\0';
        end--;
    }
    
    // Skip empty lines and comments
    if (line[0] == '\0' || line[0] == '#') {
        return;
    }
    
    // Find the = separator
    char *equals = strchr(line, '=');
    if (!equals) {
        return;
    }
    
    // Split into key and value
    *equals = '\0';
    char *key = line;
    char *value = equals + 1;
    
    // Trim whitespace from key
    while (*key == ' ' || *key == '\t') key++;
    end = key + strlen(key) - 1;
    while (end > key && (*end == ' ' || *end == '\t')) {
        *end = '\0';
        end--;
    }
    
    // Trim whitespace from value
    while (*value == ' ' || *value == '\t') value++;
    
    // Remove inline comments (everything after # in the value)
    char *comment = strchr(value, '#');
    if (comment) {
        *comment = '\0';
    }
    
    // Trim trailing whitespace from value
    end = value + strlen(value) - 1;
    while (end > value && (*end == ' ' || *end == '\t')) {
        *end = '\0';
        end--;
    }
    
    // Remove quotes from value if present
    if (strlen(value) >= 2 && 
        ((value[0] == '"' && value[strlen(value)-1] == '"') ||
         (value[0] == '\'' && value[strlen(value)-1] == '\''))) {
        value[strlen(value)-1] = '\0';
        value++;
    }
    
    // Parse configuration options
    if (strcmp(key, "prompt_format") == 0) {
        strncpy(config.prompt_format, value, sizeof(config.prompt_format) - 1);
        process_escape_sequences(config.prompt_format);
    } else if (strcmp(key, "color_time") == 0) {
        strncpy(config.color_time, value, sizeof(config.color_time) - 1);
        process_escape_sequences(config.color_time);
    } else if (strcmp(key, "color_success") == 0) {
        strncpy(config.color_success, value, sizeof(config.color_success) - 1);
        process_escape_sequences(config.color_success);
    } else if (strcmp(key, "color_error") == 0) {
        strncpy(config.color_error, value, sizeof(config.color_error) - 1);
        process_escape_sequences(config.color_error);
    } else if (strcmp(key, "color_username") == 0) {
        strncpy(config.color_username, value, sizeof(config.color_username) - 1);
        process_escape_sequences(config.color_username);
    } else if (strcmp(key, "color_hostname") == 0) {
        strncpy(config.color_hostname, value, sizeof(config.color_hostname) - 1);
        process_escape_sequences(config.color_hostname);
    } else if (strcmp(key, "color_directory") == 0) {
        strncpy(config.color_directory, value, sizeof(config.color_directory) - 1);
        process_escape_sequences(config.color_directory);
    } else if (strcmp(key, "color_prompt") == 0) {
        strncpy(config.color_prompt, value, sizeof(config.color_prompt) - 1);
        process_escape_sequences(config.color_prompt);
    } else if (strcmp(key, "color_reset") == 0) {
        strncpy(config.color_reset, value, sizeof(config.color_reset) - 1);
        process_escape_sequences(config.color_reset);
    } else if (strcmp(key, "success_symbol") == 0) {
        strncpy(config.success_symbol, value, sizeof(config.success_symbol) - 1);
    } else if (strcmp(key, "error_symbol") == 0) {
        strncpy(config.error_symbol, value, sizeof(config.error_symbol) - 1);
    } else if (strcmp(key, "show_time") == 0) {
        config.show_time = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0 || strcmp(value, "yes") == 0);
    } else if (strcmp(key, "show_status") == 0) {
        config.show_status = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0 || strcmp(value, "yes") == 0);
    } else if (strcmp(key, "show_username") == 0) {
        config.show_username = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0 || strcmp(value, "yes") == 0);
    } else if (strcmp(key, "show_hostname") == 0) {
        config.show_hostname = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0 || strcmp(value, "yes") == 0);
    } else if (strcmp(key, "show_directory") == 0) {
        config.show_directory = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0 || strcmp(value, "yes") == 0);
    } else if (strcmp(key, "use_colors") == 0) {
        config.use_colors = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0 || strcmp(value, "yes") == 0);
    } else if (strcmp(key, "multiline_prompt") == 0) {
        config.multiline_prompt = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0 || strcmp(value, "yes") == 0);
    }
}

// Load configuration from file
int load_config() {
    const char *home = getenv("HOME");
    if (!home) {
        set_default_config();
        return -1;
    }
    
    // Set defaults first
    set_default_config();
    
    // Try to load from config file
    char config_path[512];
    snprintf(config_path, sizeof(config_path), "%s/.config/mu/config", home);
    
    FILE *file = fopen(config_path, "r");
    if (!file) {
        // Config file doesn't exist, create directory for future use
        create_config_directory();
        return 0;  // Not an error, just use defaults
    }
    
    char line[512];
    while (fgets(line, sizeof(line), file)) {
        parse_config_line(line);
    }
    
    fclose(file);
    return 0;
}

// Save current configuration to file
int save_config() {
    const char *home = getenv("HOME");
    if (!home) {
        return -1;
    }
    
    // Create config directory if needed
    if (create_config_directory() != 0) {
        return -1;
    }
    
    char config_path[512];
    snprintf(config_path, sizeof(config_path), "%s/.config/mu/config", home);
    
    FILE *file = fopen(config_path, "w");
    if (!file) {
        return -1;
    }
    
    fprintf(file, "# mu shell configuration file\n");
    fprintf(file, "# Format specifiers: %%t=time, %%s=status, %%u=username, %%h=hostname, %%w=directory\n");
    fprintf(file, "\n");
    
    fprintf(file, "# Prompt format\n");
    fprintf(file, "prompt_format=%s\n", config.prompt_format);
    fprintf(file, "\n");
    
    fprintf(file, "# Colors (ANSI escape sequences)\n");
    fprintf(file, "color_time=%s\n", config.color_time);
    fprintf(file, "color_success=%s\n", config.color_success);
    fprintf(file, "color_error=%s\n", config.color_error);
    fprintf(file, "color_username=%s\n", config.color_username);
    fprintf(file, "color_hostname=%s\n", config.color_hostname);
    fprintf(file, "color_directory=%s\n", config.color_directory);
    fprintf(file, "color_prompt=%s\n", config.color_prompt);
    fprintf(file, "color_reset=%s\n", config.color_reset);
    fprintf(file, "\n");
    
    fprintf(file, "# Status symbols\n");
    fprintf(file, "success_symbol=%s\n", config.success_symbol);
    fprintf(file, "error_symbol=%s\n", config.error_symbol);
    fprintf(file, "\n");
    
    fprintf(file, "# Display options\n");
    fprintf(file, "show_time=%s\n", config.show_time ? "true" : "false");
    fprintf(file, "show_status=%s\n", config.show_status ? "true" : "false");
    fprintf(file, "show_username=%s\n", config.show_username ? "true" : "false");
    fprintf(file, "show_hostname=%s\n", config.show_hostname ? "true" : "false");
    fprintf(file, "show_directory=%s\n", config.show_directory ? "true" : "false");
    fprintf(file, "use_colors=%s\n", config.use_colors ? "true" : "false");
    fprintf(file, "multiline_prompt=%s\n", config.multiline_prompt ? "true" : "false");
    
    fclose(file);
    return 0;
}

// Get color string based on use_colors setting
const char* get_color(const char* color_code) {
    return config.use_colors ? color_code : "";
}

// Initialize configuration system
void init_config() {
    load_config();
}
