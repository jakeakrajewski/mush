# μ Shell (mush)

A lightweight, feature-rich Unix shell written in C, designed to provide a modern terminal experience with extensive customization options.

## Features

### Core Shell Functionality
- **Command Execution** - Execute external programs and built-in commands
- **Pipes** - Chain commands together with `|` operator
- **Redirections** - Redirect input/output with `<`, `>`, `>>`, `2>`, `&>`
- **Background Jobs** - Run commands in background with `&`
- **Job Control** - Manage background processes with job notifications
- **Subshells** - Execute commands in subshells with `(command)`
- **Quote Handling** - Support for single quotes, double quotes, and escape sequences
- **Tilde Expansion** - Automatic expansion of `~` to home directory

### Interactive Features
- **Line Editing** - Full cursor movement and text editing capabilities
  - Arrow keys for cursor navigation
  - Home/End keys for line beginning/end
  - Backspace/Delete for character removal
  - Insert mode for text editing
- **Command History** - Navigate through previous commands with up/down arrows
- **Tab Completion** - Smart completion for commands and file paths
  - Command completion for built-in and system commands
  - File and directory completion with visual indicators
  - Path completion with tilde expansion support
- **Multi-line Commands** - Support for line continuation with backslashes

### Customization
- **Configurable Prompts** - Highly customizable prompt with format specifiers
- **Color Support** - Full ANSI color customization for all prompt elements
- **User Configuration** - Personal settings stored in `~/.config/mu/config`

## Installation

### Prerequisites
- GCC or compatible C compiler
- Make
- POSIX-compliant Unix system (Linux, macOS, BSD)

### Building from Source
```bash
git clone git@github.com:jakeakrajewski/mush.git
cd mush
make
```

### Installation
```bash
make install
```
This installs the `mu` binary to `/usr/local/bin` by default.

## Usage

### Starting the Shell
```bash
mu
```

### Basic Commands
```bash
# Run a command
ls -la

# Use pipes
ps aux | grep mu

# Redirect output
echo "Hello World" > output.txt
cat < input.txt

# Background jobs
sleep 10 &
jobs

# Subshells
(cd /tmp && ls)
```

### Interactive Features
- **↑/↓** - Navigate command history
- **←/→** - Move cursor within current line  
- **Home/End** - Jump to beginning/end of line
- **Tab** - Complete commands and file paths
- **Ctrl+C** - Cancel current input
- **Ctrl+D** - Exit shell (when line is empty)
- **Ctrl+L** - Clear screen

## Configuration

Mu shell can be customized through a configuration file located at `~/.config/mu/config`.

### Configuration File Format
```ini
# Format specifiers: %t=time, %s=status, %u=username, %h=hostname, %w=directory

# Prompt format
prompt_format=%t %s μ> 

# Colors (ANSI escape sequences)
color_time=\033[33m        # Yellow
color_success=\033[32m     # Green
color_error=\033[31m       # Red
color_username=\033[36m    # Cyan
color_hostname=\033[35m    # Magenta
color_directory=\033[34m   # Blue
color_prompt=\033[0m       # Default
color_reset=\033[0m        # Reset

# Status symbols
success_symbol=✓
error_symbol=✗

# Display options
show_time=true
show_status=true
show_username=false
show_hostname=false
show_directory=false
use_colors=true
```

### Prompt Format Specifiers
- `%t` - Current time (HH:MM)
- `%s` - Status symbol (✓ for success, ✗ for error)
- `%u` - Username
- `%h` - Hostname (short form)
- `%w` - Current working directory (abbreviated)
- `%%` - Literal % character

### Example Configurations

**Minimalist prompt:**
```ini
prompt_format=%s> 
show_time=false
```

**Full information prompt:**
```ini
prompt_format=%u@%h:%w %t %s$ 
show_username=true
show_hostname=true
show_directory=true
```

**Git-style prompt:**
```ini
prompt_format=(%u) %w %s$ 
show_username=true
show_directory=true
show_time=false
```

## Built-in Commands

- `exit` - Exit the shell
- `cd [directory]` - Change directory
- `jobs` - List active background jobs
- `fg [job]` - Bring job to foreground
- `bg [job]` - Send job to background
- `help` - Display help information

## File Structure

```
mu-shell/
├── src/
│   ├── main.c              # Main shell loop and entry point
│   ├── execute.c           # Command execution and pipeline handling
│   ├── execute.h           # Command execution interface
│   ├── builtins.c          # Built-in commands implementation
│   ├── builtins.h          # Built-in commands interface
│   ├── job_control.c       # Background job management
│   ├── job_control.h       # Job control interface
│   ├── job.c               # Job structure and utilities
│   ├── job.h               # Job data structures
│   ├── input.c             # Input parsing and processing
│   ├── input.h             # Input parsing interface
│   ├── init.c              # Shell initialization
│   ├── init.h              # Initialization interface
│   ├── launch.c            # Process launching utilities
│   ├── launch.h            # Process launching interface
│   ├── process.h           # Process data structures
│   ├── signal_handlers.c   # Signal handling implementation
│   ├── signal_handlers.h   # Signal handling interface
│   ├── substitution.c      # Variable and command substitution
│   ├── substitution.h      # Substitution interface
│   ├── tokenizer.c         # Input tokenization
│   ├── tokenizer.h         # Tokenization interface
│   └── promptly/           # Interactive input system
│       ├── promptly.c      # Main input loop and editing
│       ├── promptly.h      # Promptly system interface
│       ├── prompt.c        # Prompt formatting and display
│       ├── prompt.h        # Prompt interface
│       ├── history.c       # Command history management
│       ├── history.h       # History interface
│       ├── cursor.c        # Cursor movement and control
│       ├── cursor.h        # Cursor control interface
│       ├── config.c        # Configuration management
│       └── config.h        # Configuration interface
├── include/                # Additional header files
├── tests/                  # Test suite
├── docs/                   # Documentation
├── build/                  # Build artifacts
├── Makefile               # Build configuration
└── README.md              # This file
```

## Advanced Features

### Tab Completion
Mu shell provides intelligent tab completion:

- **Command completion** - Complete built-in and system commands
- **Path completion** - Complete file and directory paths
- **Smart context** - Determines whether to complete commands or paths based on position
- **Visual indicators** - Shows directories with trailing `/`
- **Multiple matches** - Displays all possible completions when ambiguous

### Job Control
Background job management with real-time notifications:

```bash
# Start background job
sleep 30 &
[1] 12345

# List jobs
jobs
[1]+  Running    sleep 30 &

# Bring to foreground
fg 1

# Job completion notification
[1]+  Done       sleep 30
```

### Quote and Escape Handling
Proper handling of various quoting mechanisms:

```bash
# Single quotes preserve literal values
echo 'Hello $USER'  # Outputs: Hello $USER

# Double quotes allow variable expansion
echo "Hello $USER"  # Outputs: Hello username

# Escape sequences
echo "Line 1\nLine 2"  # Outputs on separate lines
```

### Debugging
Build with debug symbols:
```bash
make debug
```


## License

This project is licensed under the [GNU General Public License v3.0](LICENSE).

You are free to use, modify, and distribute this software, provided that any derivative works are also licensed under the GPLv3. This ensures that all modifications and improvements remain open and available to the community.


## Author

Jake Krajewski

---

*μ Shell - A minimal yet powerful shell for the modern terminal*
