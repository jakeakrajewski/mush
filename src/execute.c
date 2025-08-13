#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "builtins.h"
#include "execute.h"
#include "input.h"
#include "launch.h"
#include "prompt.h"
#include "substitution.h"
#include "tokenizer.h"
#include "job_control.h"

extern int debug_substitution;

int exec_substitute_node(ASTNode *node, int silent) {
  if (debug_substitution) {
    fprintf(
        stderr,
        "DEBUG execute: Processing NODE_SUBSTITUTE, executing left child\n");
  }
  return execute(node->left, silent);
}

int exec_bang_node(ASTNode *node, int silent) {
  int status = execute(node->left, 1);
  return status == 0 ? 1 : 0;
}

int exec_sequence_node(ASTNode *node, int silent) {
  execute(node->left, silent);
  return execute(node->right, silent);
}

int exec_and_node(ASTNode *node, int silent) {
  int status = execute(node->left, silent);
  if (status == 0)
    return execute(node->right, silent);
  else
    return status;
}

int exec_or_node(ASTNode *node, int silent) {
  int status = execute(node->left, 1);
  if (status != 0)
    return execute(node->right, 0);
  else
    return status;
}

int exec_pipe_node(ASTNode *node, int silent) {
  ASTNode *commands[64];
  int cmd_count = 0;
  ASTNode *current = node;
  while (current && current->type == NODE_PIPE) {
    commands[cmd_count++] = current->left;
    current = current->right;
  }
  if (current) {
    commands[cmd_count++] = current;
  }

  int pipefds[2 * (cmd_count - 1)];
  for (int i = 0; i < cmd_count - 1; i++) {
    if (pipe(pipefds + i * 2) < 0) {
      if (!silent)
        perror("pipe");
      return 1;
    }
  }

  pid_t pids[cmd_count];
  for (int i = 0; i < cmd_count; i++) {
    pids[i] = fork();
    if (pids[i] < 0) {
      if (!silent)
        perror("fork");
      for (int j = 0; j < 2 * (cmd_count - 1); j++)
        close(pipefds[j]);
      return 1;
    }

    if (pids[i] == 0) {
      if (i > 0) {
        if (dup2(pipefds[(i - 1) * 2], STDIN_FILENO) < 0) {
          if (!silent)
            perror("dup2 stdin");
          exit(1);
        }
      }

      if (i < cmd_count - 1) {
        if (dup2(pipefds[i * 2 + 1], STDOUT_FILENO) < 0) {
          if (!silent)
            perror("dup2 stdout");
          exit(1);
        }
      }

      for (int j = 0; j < 2 * (cmd_count - 1); j++)
        close(pipefds[j]);

      int status = execute(commands[i], silent);
      exit(status);
    }
  }

  for (int i = 0; i < 2 * (cmd_count - 1); i++) {
    close(pipefds[i]);
  }

  int status = 0;
  for (int i = 0; i < cmd_count; i++) {
    int wstatus;
    waitpid(pids[i], &wstatus, 0);
    if (i == cmd_count - 1) {
      status = WIFEXITED(wstatus) ? WEXITSTATUS(wstatus) : 1;
    }
  }
  return status;
}

int exec_subshell_node(ASTNode *node, int silent) {
  pid_t pid = fork();
  if (pid == 0) {
    int subshell_status = execute(node->left, silent);
    exit(subshell_status);
  } else if (pid > 0) {
    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
  } else {
    if (!silent)
      perror("fork");
    return 1;
  }
}

int apply_redirections(ASTNode *node, int saved_stdin, int saved_stdout,
                       int saved_stderr) {
  Redirection *r = node->redirs;
  while (r) {
    int fd_target;

    if (r->filename[0] == '&') {
      if (r->filename[1] == '-') {
        if (close(r->fd) == -1) {
          perror("close");
          return 1;
        }
      } else if (isdigit(r->filename[1])) {
        int target_fd = atoi(r->filename + 1);
        if (dup2(target_fd, r->fd) == -1) {
          perror("dup2");
          return 1;
        }
      } else {
        fprintf(stderr, "Invalid redirection target: %s\n", r->filename);
        return 1;
      }
      r = r->next;
      continue;
    }

    switch (r->type) {
    case NODE_READ:
      fd_target = O_RDONLY;
      break;
    case NODE_WRITE:
      fd_target = O_WRONLY | O_CREAT | O_TRUNC;
      break;
    case NODE_APPEND:
      fd_target = O_WRONLY | O_CREAT | O_APPEND;
      break;
    case NODE_READWRITE:
      fd_target = O_RDWR | O_CREAT;
      break;
    case NODE_ERR:
      fd_target = O_WRONLY | O_CREAT | O_TRUNC;
      break;
    case NODE_WRITE_ERR:
      fd_target = O_WRONLY | O_CREAT | O_TRUNC;
      break;
    default:
      fprintf(stderr, "Unknown redirection type\n");
      return 1;
    }

    int fd = open(r->filename, fd_target, 0644);
    if (fd == -1) {
      perror(r->filename);
      return 1;
    }

    if (dup2(fd, r->fd) == -1) {
      perror("dup2");
      close(fd);
      return 1;
    }

    close(fd);
    r = r->next;
  }
  return 0;
}

int exec_command_node(ASTNode *node, int silent) {
  if (debug_substitution) {
    fprintf(stderr, "DEBUG execute: Processing NODE_COMMAND with %d args\n",
            node->argc);
  }

  // Save original FDs
  int saved_stdin = dup(STDIN_FILENO);
  int saved_stdout = dup(STDOUT_FILENO);
  int saved_stderr = dup(STDERR_FILENO);

  int status = 0;
  if (apply_redirections(node, saved_stdin, saved_stdout, saved_stderr) == 1) {
    status = 1;
    goto cleanup_fds;
  }

  char **argv = malloc(sizeof(char *) * (node->argc + 64));
  int argc = 0;

  for (int i = 0; i < node->argc; i++) {
    if (node->args[i].is_substitution) {
      char *output = execute_substitution(node->args[i].substitution_node);

      if (output && *output) {
        char *saveptr = NULL;
        char *token = strtok_r(output, " \t\r\n", &saveptr);
        while (token != NULL) {
          argv[argc++] = strdup(token);
          token = strtok_r(NULL, " \t\r\n", &saveptr);
        }
      }
      free(output);
    } else {
      argv[argc++] = strdup(node->args[i].text);
    }
  }
  argv[argc] = NULL;

  if (debug_substitution) {
    fprintf(stderr, "DEBUG execute: Final argv has %d elements:\n", argc);
    for (int i = 0; i < argc; i++) {
      fprintf(stderr, "DEBUG execute: argv[%d] = '%s'\n", i, argv[i]);
    }
    fprintf(stderr, "DEBUG execute: argv[%d] = NULL\n", argc);
  }

  status = mu_execute(argv);

  for (int i = 0; i < argc; i++)
    free(argv[i]);
  free(argv);

cleanup_fds:
  dup2(saved_stdin, STDIN_FILENO);
  dup2(saved_stdout, STDOUT_FILENO);
  dup2(saved_stderr, STDERR_FILENO);
  close(saved_stdin);
  close(saved_stdout);
  close(saved_stderr);

  return status;
}

char **ast_to_argv(ASTNode *node) {
    if (!node || node->type != NODE_COMMAND)
        return NULL;

    char **argv = calloc(node->argc + 1, sizeof(char *));
    if (!argv) return NULL;

    for (int i = 0; i < node->argc; i++) {
        if (node->args[i].is_substitution) {
            // You might want to handle substitution here, but for now:
            argv[i] = strdup("SUBST"); // placeholder
        } else {
            argv[i] = strdup(node->args[i].text);
        }
    }
    argv[node->argc] = NULL;
    return argv;
}

char *argv_join(char **argv) {
    if (!argv || !argv[0])
        return strdup("");

    size_t len = 0;
    for (int i = 0; argv[i]; i++)
        len += strlen(argv[i]) + 1; // space or null terminator

    char *result = malloc(len);
    if (!result) return NULL;

    result[0] = '\0';
    for (int i = 0; argv[i]; i++) {
        strcat(result, argv[i]);
        if (argv[i + 1])
            strcat(result, " ");
    }
    return result;
}


job *create_job_from_ast(ASTNode *node) {
    job *j = calloc(1, sizeof(job));
    j->stdin = STDIN_FILENO;
    j->stdout = STDOUT_FILENO;
    j->stderr = STDERR_FILENO;
    j->pgid = 0;
    j->notified = 0;
    j->next = first_job;
    first_job = j;

    // Convert AST command to process list
    process *p = calloc(1, sizeof(process));
    p->argv = ast_to_argv(node); // reuse your exec_command_node logic
    p->next = NULL;

    j->first_process = p;

    // Optional: store raw command string for user messages
    j->command = strdup(argv_join(p->argv));

    return j;
}

int exec_job_node(ASTNode *node, int silent) {
    // Background job means run the child node without waiting.
    // Build a job struct from it.
    job *j = create_job_from_ast(node->left); // You'll need this helper
    if (!j) {
        if (!silent)
            fprintf(stderr, "mu: failed to create job\n");
        return 1;
    }

    launch_job(j, 0); // 0 = background
    return 0;
}

int execute(ASTNode *node, int silent) {
  if (!node) {
    if (debug_substitution) {
      fprintf(stderr, "DEBUG execute: node is NULL, returning 1\n");
    }
    return 1;
  }

  if (debug_substitution) {
    fprintf(stderr, "DEBUG execute: Processing node type %d\n", node->type);
  }

  switch (node->type) {
  case NODE_COMMAND:
    return exec_command_node(node, silent);
  case NODE_AND:
    return exec_and_node(node, silent);
  case NODE_OR:
    return exec_or_node(node, silent);
  case NODE_SEQUENCE:
    return exec_subshell_node(node, silent);
  case NODE_SUBSHELL:
    return exec_subshell_node(node, silent);
  case NODE_PIPE:
    return exec_pipe_node(node, silent);
  case NODE_BANG:
    return exec_bang_node(node, silent);
  case NODE_SUBSTITUTE:
    return exec_substitute_node(node, silent);
  case NODE_JOB:
    return exec_job_node(node, silent);

  default:
    if (!silent)
      fprintf(stderr, "Unknown AST node type: %d\n", node->type);
    return 1;
  }
}

int mu_execute_logical_commands(char *line) {
  tokenize(line);
  ASTNode *tree = parse_sequence();

  print_ast(tree, 0);
  if (!tree) {
    fprintf(stderr, "mu: parse error\n");
    return 1;
  }

  return execute(tree, 0);
}
