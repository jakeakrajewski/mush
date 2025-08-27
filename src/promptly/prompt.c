#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <limits.h>
#include <stdlib.h>

#include "config.h"

extern int mu_last_status;
extern size_t lines_count;

// Helper function to get current working directory, abbreviated if needed
static void get_abbreviated_cwd(char *buffer, size_t buffer_size) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        strncpy(buffer, "?", buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
        return;
    }
    
    const char *home = getenv("HOME");
    if (home && strncmp(cwd, home, strlen(home)) == 0) {
        // Replace home directory with ~
        snprintf(buffer, buffer_size, "~%s", cwd + strlen(home));
    } else {
        strncpy(buffer, cwd, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
    }
    
    // If path is too long, show only the last few directories
    if (strlen(buffer) > 30) {
        char *last_slash = strrchr(buffer, '/');
        if (last_slash && last_slash != buffer) {
            char *second_last_slash = last_slash - 1;
            while (second_last_slash > buffer && *second_last_slash != '/') {
                second_last_slash--;
            }
            if (*second_last_slash == '/' && second_last_slash != buffer) {
                snprintf(buffer, buffer_size, "...%s", second_last_slash);
            }
        }
    }
}

// Helper function to get username
static void get_username(char *buffer, size_t buffer_size) {
    struct passwd *pw = getpwuid(getuid());
    if (pw) {
        strncpy(buffer, pw->pw_name, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
    } else {
        strncpy(buffer, "user", buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
    }
}

// Helper function to get hostname
static void get_hostname(char *buffer, size_t buffer_size) {
    if (gethostname(buffer, buffer_size - 1) != 0) {
        strncpy(buffer, "localhost", buffer_size - 1);
    }
    buffer[buffer_size - 1] = '\0';
    
    // Remove domain part if present
    char *dot = strchr(buffer, '.');
    if (dot) {
        *dot = '\0';
    }
}

// Format and print the prompt based on configuration
void print_prompt() {
    if (lines_count > 0) {
        printf("> ");
        fflush(stdout);
        return;
    }
    
    // Prepare all the components
    char timestr[32] = "";
    char username[64] = "";
    char hostname[64] = "";
    char directory[256] = "";
    char status_symbol[64] = "";
    
    // Get time if needed
    if (config.show_time || strchr(config.prompt_format, 't')) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        strftime(timestr, sizeof(timestr), "%H:%M", t);
    }
    
    // Get status symbol if needed
    if (config.show_status || strchr(config.prompt_format, 's')) {
        const char *symbol = (mu_last_status == 0) ? config.success_symbol : config.error_symbol;
        const char *color = (mu_last_status == 0) ? 
            get_color(config.color_success) : get_color(config.color_error);
        snprintf(status_symbol, sizeof(status_symbol), "%s%s%s", 
                 color, symbol, get_color(config.color_reset));
    }
    
    // Get username if needed
    if (config.show_username || strchr(config.prompt_format, 'u')) {
        get_username(username, sizeof(username));
    }
    
    // Get hostname if needed
    if (config.show_hostname || strchr(config.prompt_format, 'h')) {
        get_hostname(hostname, sizeof(hostname));
    }
    
    // Get directory if needed
    if (config.show_directory || strchr(config.prompt_format, 'w')) {
        get_abbreviated_cwd(directory, sizeof(directory));
    }
    
    // Build the prompt string
    char prompt[1024] = "";
    const char *format = config.prompt_format;
    
    for (int i = 0; format[i]; i++) {
        if (format[i] == '%' && format[i + 1]) {
            switch (format[i + 1]) {
                case 't': // Time
                    if (config.show_time) {
                        strcat(prompt, get_color(config.color_time));
                        strcat(prompt, timestr);
                        strcat(prompt, get_color(config.color_reset));
                    }
                    i++; // Skip the format character
                    break;
                case 's': // Status symbol
                    if (config.show_status) {
                        strcat(prompt, status_symbol);
                    }
                    i++; // Skip the format character
                    break;
                case 'u': // Username
                    if (config.show_username) {
                        strcat(prompt, get_color(config.color_username));
                        strcat(prompt, username);
                        strcat(prompt, get_color(config.color_reset));
                    }
                    i++; // Skip the format character
                    break;
                case 'h': // Hostname
                    if (config.show_hostname) {
                        strcat(prompt, get_color(config.color_hostname));
                        strcat(prompt, hostname);
                        strcat(prompt, get_color(config.color_reset));
                    }
                    i++; // Skip the format character
                    break;
                case 'w': // Working directory
                    if (config.show_directory) {
                        strcat(prompt, get_color(config.color_directory));
                        strcat(prompt, directory);
                        strcat(prompt, get_color(config.color_reset));
                    }
                    i++; // Skip the format character
                    break;
                case '%': // Literal %
                    strncat(prompt, "%", 1);
                    i++; // Skip the format character
                    break;
                default:
                    // Unknown format specifier, treat as literal
                    strncat(prompt, &format[i], 1);
                    break;
            }
        } else {
            // Regular character
            strncat(prompt, &format[i], 1);
        }
    }
    
    // Apply prompt color and print
    printf("%s%s%s", get_color(config.color_prompt), prompt, get_color(config.color_reset));
    fflush(stdout);
}
