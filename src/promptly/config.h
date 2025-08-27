#ifndef CONFIG_H
#define CONFIG_H

// Configuration structure
typedef struct {
    // Prompt format string
    char prompt_format[256];
    
    // Colors
    char color_time[32];
    char color_success[32];
    char color_error[32];
    char color_username[32];
    char color_hostname[32];
    char color_directory[32];
    char color_prompt[32];
    char color_reset[32];
    
    // Status symbols
    char success_symbol[16];
    char error_symbol[16];
    
    // Display options
    int show_time;
    int show_status;
    int show_username;
    int show_hostname;
    int show_directory;
    int use_colors;
    int multiline_prompt;
} Config;

// Global configuration instance
extern Config config;

// Function declarations
int load_config();
int save_config();
const char* get_color(const char* color_code);
void init_config();

#endif // CONFIG_H
