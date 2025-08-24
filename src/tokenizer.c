#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *process_quotes(const char *word);

typedef enum {
  TOKEN_WORD,
  TOKEN_AND,
  TOKEN_OR,
  TOKEN_SEMI,
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_END,
  TOKEN_PIPE,
  TOKEN_WRITE,
  TOKEN_APPEND,
  TOKEN_READ,
  TOKEN_ERR,
  TOKEN_WRITE_ERR,
  TOKEN_READWRITE,
  TOKEN_SUBSTITUTE,
  TOKEN_BANG,
  TOKEN_JOB,
} TokenType;

typedef struct {
  TokenType type;
  char *text;
} Token;

struct ASTNode;

#define MAX_TOKENS 1024

Token tokens[MAX_TOKENS];
int token_count = 0;
int pos = 0;

void add_token(TokenType type, const char *text) {
  tokens[token_count++] = (Token){type, strdup(text)};
}

char *expand_tilde(const char *word) {
    if (word[0] != '~') {
        return strdup(word);
    }
    
    char *home = getenv("HOME");
    if (!home) {
        return strdup(word);  // If no HOME, return as-is
    }
    
    if (word[1] == '\0' || word[1] == '/') {
        // Handle ~ or ~/path
        size_t home_len = strlen(home);
        size_t word_len = strlen(word);
        char *result = malloc(home_len + word_len);  // word_len includes the ~
        strcpy(result, home);
        if (word[1] == '/') {
            strcat(result, word + 1);  // Skip the ~ but keep the /
        }
        return result;
    }
    
    return strdup(word);
}

void tokenize(const char *input) {
  token_count = 0;
  pos = 0;

  while (*input) {
    if (isspace(*input)) {
      input++;
      continue;
    }

    // Check for digit(s) followed by redirection operator without space
    if (isdigit(*input)) {
      const char *start = input;
      while (isdigit(*input))
        input++;

      // Lookahead at redirection operator after digits
      if (strncmp(input, ">>", 2) == 0 || strncmp(input, "<<", 2) == 0 ||
          strncmp(input, "<>", 2) == 0) {
        // tokenize fd number first
        char *fdnum = strndup(start, input - start);
        add_token(TOKEN_WORD, fdnum);
        free(fdnum);

        if (strncmp(input, ">>", 2) == 0) {
          add_token(TOKEN_APPEND, ">>");
          input += 2;
        } else if (strncmp(input, "<>", 2) == 0) {
          add_token(TOKEN_READWRITE, "<>");
          input += 2;
        }
        continue;
      } else if (*input == '>' || *input == '<') {
        // tokenize fd number first
        char *fdnum = strndup(start, input - start);
        add_token(TOKEN_WORD, fdnum);
        free(fdnum);

        // Then tokenize 1-char redirection operator
        if (*input == '>') {
          add_token(TOKEN_WRITE, ">");
        } else if (*input == '<') {
          add_token(TOKEN_READ, "<");
        }
        input++;
        continue;
      } else {
        // no redirection operator after digits, treat whole as word
        char *word = strndup(start, input - start);
        add_token(TOKEN_WORD, word);
        free(word);
        continue;
      }
    }

    if (strncmp(input, "$(", 2) == 0) {
      add_token(TOKEN_SUBSTITUTE, "$(");
      input += 2;
    } else if (*input == '(') {
      add_token(TOKEN_LPAREN, "(");
      input++;
    } else if (*input == ')') {
      add_token(TOKEN_RPAREN, ")");
      input++;
    } else if (*input == '!') {
      add_token(TOKEN_BANG, "!");
      input++;
    } else if (*input == ';') {
      add_token(TOKEN_SEMI, ";");
      input++;
    } else if (strncmp(input, "&&", 2) == 0) {
      add_token(TOKEN_AND, "&&");
      input += 2;
    } else if (*input == '&') {
      add_token(TOKEN_JOB, "&");
      input++;
    } else if (strncmp(input, "||", 2) == 0) {
      add_token(TOKEN_OR, "||");
      input += 2;
    } else if (strncmp(input, ">>", 2) == 0) {
      input += 2;
      // Check if there's an immediate filename
      const char *start = input;
      while (*input && !isspace(*input) && *input != ';' && *input != '&' &&
             *input != '|' && *input != '(' && *input != ')')
        input++;
      char *word = strndup(start, input - start);
      add_token(TOKEN_APPEND, ">>");
      if (strlen(word) > 0)
        add_token(TOKEN_WORD, word);
      free(word);
    } else if (strncmp(input, "2>", 2) == 0) {
      input += 2;
      const char *start = input;
      while (*input && !isspace(*input) && *input != ';' && *input != '&' &&
             *input != '|' && *input != '(' && *input != ')')
        input++;
      char *word = strndup(start, input - start);
      add_token(TOKEN_ERR, "2>");
      if (strlen(word) > 0)
        add_token(TOKEN_WORD, word);
      free(word);
    } else if (strncmp(input, "<>", 2) == 0) {
      input += 2;
      const char *start = input;
      while (*input && !isspace(*input) && *input != ';' && *input != '&' &&
             *input != '|' && *input != '(' && *input != ')')
        input++;
      char *word = strndup(start, input - start);
      add_token(TOKEN_READWRITE, "<>");
      if (strlen(word) > 0)
        add_token(TOKEN_WORD, word);
      free(word);
    } else if (*input == '>') {
      input++;
      const char *start = input;
      while (*input && !isspace(*input) && *input != ';' && *input != '&' &&
             *input != '|' && *input != '(' && *input != ')')
        input++;
      char *word = strndup(start, input - start);
      add_token(TOKEN_WRITE, ">");
      if (strlen(word) > 0)
        add_token(TOKEN_WORD, word);
      free(word);
    } else if (*input == '<') {
      input++;
      const char *start = input;
      while (*input && !isspace(*input) && *input != ';' && *input != '&' &&
             *input != '|' && *input != '(' && *input != ')')
        input++;
      char *word = strndup(start, input - start);
      add_token(TOKEN_READ, "<");
      if (strlen(word) > 0)
        add_token(TOKEN_WORD, word);
      free(word);
    } else if (*input == '|') {
      add_token(TOKEN_PIPE, "|");
      input++;
    } else {
      const char *start = input;
      int in_single_quote = 0;
      int in_double_quote = 0;

      while (*input && (in_single_quote || in_double_quote ||
                        (!isspace(*input) && *input != ';' && *input != '|' &&
                         *input != '(' && *input != ')' && *input != '!' &&
                         *input != '&' && *input != '<' && *input != '>'))) {

        if (*input == '\'' && !in_double_quote) {
          in_single_quote = !in_single_quote;
        } else if (*input == '"' && !in_single_quote) {
          in_double_quote = !in_double_quote;
        } else if (*input == '\\' && !in_single_quote) {
          // Skip escaped character
          input++;
          if (*input)
            input++;
          continue;
        }
        input++;
      }

      if (in_single_quote || in_double_quote) {
        fprintf(stderr, "mu: unterminated quoted string\n");
        return;
      }

     if (start != input) {
          char *raw_word = strndup(start, input - start);
          char *processed_word = process_quotes(raw_word);
          char *expanded_word = expand_tilde(processed_word);
          add_token(TOKEN_WORD, expanded_word);
          free(raw_word);
          free(processed_word);
          free(expanded_word);
      }
    }
  }

  add_token(TOKEN_END, "<end>");
}

typedef enum {
  NODE_COMMAND,
  NODE_AND,
  NODE_OR,
  NODE_SEQUENCE,
  NODE_SUBSHELL,
  NODE_PIPE,
  NODE_WRITE,
  NODE_APPEND,
  NODE_READ,
  NODE_ERR,
  NODE_WRITE_ERR,
  NODE_READWRITE,
  NODE_SUBSTITUTE,
  NODE_BANG,
  NODE_JOB,
} NodeType;

typedef struct Redirection {
  int fd;
  NodeType type;
  char *filename;
  int close_fd;
  struct Redirection *next;
} Redirection;

typedef struct Arg {
  int is_substitution;
  union {
    char *text;
    struct ASTNode *substitution_node;
  };
} Arg;

typedef struct ASTNode {
  NodeType type;
  struct ASTNode *left;
  struct ASTNode *right;
  struct Arg *args;
  int argc;

  Redirection *redirs;
} ASTNode;

ASTNode *parse_sequence();

Token *peek() { return &tokens[pos]; }

Token *consume() { return &tokens[pos++]; }

int match(TokenType type) {
  if (peek()->type == type) {
    consume();
    return 1;
  }
  return 0;
}

int is_all_digits(const char *s) {
  for (; *s; s++)
    if (!isdigit(*s))
      return 0;
  return 1;
}

NodeType node_type_for_token_type(TokenType t) {
  switch (t) {
  case TOKEN_READ:
    return NODE_READ;
  case TOKEN_WRITE:
    return NODE_WRITE;
  case TOKEN_APPEND:
    return NODE_APPEND;
  case TOKEN_READWRITE:
    return NODE_READWRITE;
  case TOKEN_ERR:
    return NODE_ERR;
  case TOKEN_WRITE_ERR:
    return NODE_WRITE_ERR;
  default:
    return NODE_COMMAND; // or error
  }
}

// Helper to create a new Redirection node and append it to the list
void add_redirection(ASTNode *node, int fd, NodeType type,
                     const char *filename) {
  Redirection *new_redir = malloc(sizeof(Redirection));
  new_redir->fd = fd;
  new_redir->type = type;
  new_redir->filename = strdup(filename);
  new_redir->next = NULL;

  if (!node->redirs) {
    node->redirs = new_redir;
  } else {
    Redirection *cur = node->redirs;
    while (cur->next)
      cur = cur->next;
    cur->next = new_redir;
  }
}

ASTNode *parse_substitute() {
  if (!match(TOKEN_SUBSTITUTE)) {
    printf("No substitution token found\n");
    return NULL;
  }

  ASTNode *inner = parse_sequence();

  if (!match(TOKEN_RPAREN)) {
    fprintf(stderr, "Expected ')' to close substitution\n");
    return NULL;
  }

  ASTNode *node = malloc(sizeof(ASTNode));
  node->type = NODE_SUBSTITUTE;
  node->left = inner;
  node->right = NULL;
  node->args = NULL;
  node->redirs = NULL;
  return node;
}

ASTNode *parse_command() {
  if (match(TOKEN_LPAREN)) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_SUBSHELL;
    node->left = parse_sequence();
    node->right = NULL;
    node->args = NULL;
    node->redirs = NULL;

    if (!match(TOKEN_RPAREN)) {
      fprintf(stderr, "Expected ')'\n");
      return NULL;
    }

    return node;
  }

  if (peek()->type != TOKEN_WORD && peek()->type != TOKEN_SUBSTITUTE) {
    return NULL; // Not a command start token
  }

  ASTNode *node = calloc(1, sizeof(ASTNode));
  node->type = NODE_COMMAND;
  node->left = node->right = NULL;
  node->redirs = NULL;

  struct Arg *args = malloc(sizeof(Arg) * 64);
  int argc = 0;

  while (peek()->type == TOKEN_WORD || peek()->type == TOKEN_WRITE ||
         peek()->type == TOKEN_APPEND || peek()->type == TOKEN_READ ||
         peek()->type == TOKEN_ERR || peek()->type == TOKEN_WRITE_ERR ||
         peek()->type == TOKEN_READWRITE || peek()->type == TOKEN_SUBSTITUTE) {

    int fd = -1;

    // Handle explicit FD prefix (like 2>)
    if (peek()->type == TOKEN_WORD && is_all_digits(peek()->text)) {
      Token *fd_token = peek();
      Token *next = &tokens[pos + 1];

      if (next->type == TOKEN_WRITE || next->type == TOKEN_APPEND ||
          next->type == TOKEN_READ || next->type == TOKEN_ERR ||
          next->type == TOKEN_WRITE_ERR || next->type == TOKEN_READWRITE) {
        fd = atoi(fd_token->text);
        consume(); // consume the fd token
      }
    }

    Token *tok = peek();

    if (tok->type == TOKEN_WRITE || tok->type == TOKEN_APPEND ||
        tok->type == TOKEN_READ || tok->type == TOKEN_ERR ||
        tok->type == TOKEN_WRITE_ERR || tok->type == TOKEN_READWRITE) {

      TokenType redir_tok = consume()->type;

      if (peek()->type != TOKEN_WORD) {
        fprintf(stderr, "Expected filename after redirection\n");
        return NULL;
      }

      if (fd == -1) {
        switch (redir_tok) {
        case TOKEN_READ:
          fd = 0;
          break;
        case TOKEN_WRITE:
        case TOKEN_APPEND:
        case TOKEN_ERR:
        case TOKEN_WRITE_ERR:
          fd = 1;
          break;
        case TOKEN_READWRITE:
          fd = 0;
          break;
        default:
          fd = 1;
          break;
        }
      }

      Token *file_tok = consume();

      char *expanded_filename = expand_tilde(file_tok->text);

      Redirection *r = malloc(sizeof(Redirection));
      r->fd = fd;
      r->filename = expanded_filename;
      r->next = NULL;

      switch (redir_tok) {
      case TOKEN_WRITE:
        r->type = NODE_WRITE;
        break;
      case TOKEN_APPEND:
        r->type = NODE_APPEND;
        break;
      case TOKEN_READ:
        r->type = NODE_READ;
        break;
      case TOKEN_ERR:
        r->type = NODE_ERR;
        break;
      case TOKEN_WRITE_ERR:
        r->type = NODE_WRITE_ERR;
        break;
      case TOKEN_READWRITE:
        r->type = NODE_READWRITE;
        break;
      default:
        r->type = NODE_WRITE;
        break;
      }

      // Append redirection to node->redirs
      if (!node->redirs) {
        node->redirs = r;
      } else {
        Redirection *tail = node->redirs;
        while (tail->next)
          tail = tail->next;
        tail->next = r;
      }
    } else if (tok->type == TOKEN_SUBSTITUTE) {
      args[argc].is_substitution = 1;
      args[argc].substitution_node = parse_substitute();
      argc++;
    } else if (tok->type == TOKEN_WORD) {
      args[argc].is_substitution = 0;
      args[argc].text = strdup(consume()->text);
      argc++;
    }
  }

  node->argc = argc;
  node->args = args;

  return node;
}

ASTNode *parse_pipeline() {
  ASTNode *left = parse_command();

  while (match(TOKEN_PIPE)) {
    ASTNode *right = parse_command();

    if (!right) {
      fprintf(stderr, "mu: syntax error near unexpected token '|'\n");
      return NULL;
    }

    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_PIPE;
    node->left = left;
    node->right = right;
    node->args = NULL;
    node->redirs = NULL;
    node->argc = 0;
    left = node;
  }

  return left;
}

ASTNode *parse_prefix() {
  if (match(TOKEN_BANG)) {
    ASTNode *child = parse_prefix();
    if (!child) {
      fprintf(stderr, "mu: syntax error: expected command after '!'\n");
      return NULL;
    }
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_BANG;
    node->left = child;
    node->right = NULL;
    return node;
  }

  ASTNode *pipe = parse_pipeline();
  if (!pipe) {
    return NULL; // propagate error
  }
  return pipe;
}

ASTNode *parse_and_or() {
  ASTNode *left;

  if (match(TOKEN_LPAREN)) {
    ASTNode *subshell = malloc(sizeof(ASTNode));
    subshell->type = NODE_SUBSHELL;
    subshell->left = parse_sequence();
    subshell->right = NULL;
    subshell->args = NULL;
    subshell->redirs = NULL;
    subshell->argc = 0;

    if (!match(TOKEN_RPAREN)) {
      fprintf(stderr, "Expected ')'\n");
      return NULL;
    }
    left = subshell;
  } else {
    left = parse_prefix();
  }

  while (peek()->type == TOKEN_AND || peek()->type == TOKEN_OR) {
    TokenType op = consume()->type;

    ASTNode *right;

    if (match(TOKEN_LPAREN)) {
      ASTNode *subshell = malloc(sizeof(ASTNode));
      subshell->type = NODE_SUBSHELL;
      subshell->left = parse_sequence();
      subshell->right = NULL;
      subshell->args = NULL;
      subshell->redirs = NULL;
      subshell->argc = 0;

      if (!match(TOKEN_RPAREN)) {
        fprintf(stderr, "Expected ')'\n");
        return NULL;
      }
      right = subshell;
    } else {
      right = parse_prefix();
    }

    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = (op == TOKEN_AND) ? NODE_AND : NODE_OR;
    node->left = left;
    node->right = right;
    node->args = NULL;
    node->redirs = NULL;
    node->argc = 0;
    left = node;
  }

  // Check for background job indicator
  if (match(TOKEN_JOB)) {
    ASTNode *job_node = malloc(sizeof(ASTNode));
    job_node->type = NODE_JOB;
    job_node->left = left;
    job_node->right = NULL;
    job_node->args = NULL;
    job_node->redirs = NULL;
    job_node->argc = 0;
    left = job_node;
  }

  return left;
}

ASTNode *parse_sequence() {
  ASTNode *left = parse_and_or();

  while (match(TOKEN_SEMI)) {
    ASTNode *right = parse_and_or();
    if (!right)
      break;

    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_SEQUENCE;
    node->left = left;
    node->right = right;
    node->args = NULL;
    node->redirs = NULL;
    node->argc = 0;
    left = node;
  }

  return left;
}



char *process_quotes(const char *word) {
  size_t len = strlen(word);
  char *result = malloc(len + 1); // worst case same length
  size_t out_pos = 0;
  size_t i = 0;

  while (i < len) {
    if (word[i] == '\'') {
      // Single quotes: everything literal until closing quote
      i++; // skip opening quote
      while (i < len && word[i] != '\'') {
        result[out_pos++] = word[i++];
      }
      if (i < len)
        i++; // skip closing quote
    } else if (word[i] == '"') {
      // Double quotes: allow escapes and variable substitution
      i++; // skip opening quote
      while (i < len && word[i] != '"') {
        if (word[i] == '\\' && i + 1 < len) {
          char next = word[i + 1];
          if (next == '"' || next == '\\' || next == '$' || next == '`' ||
              next == '\n') {
            result[out_pos++] = next;
            i += 2;
          } else {
            // Backslash is literal if not escaping special char
            result[out_pos++] = word[i++];
          }
        } else {
          result[out_pos++] = word[i++];
        }
      }
      if (i < len)
        i++; // skip closing quote
    } else if (word[i] == '\\' && i + 1 < len) {
      // Unquoted backslash escape
      result[out_pos++] = word[i + 1];
      i += 2;
    } else {
      result[out_pos++] = word[i++];
    }
  }

  result[out_pos] = '\0';
  return result;
}

void print_ast(ASTNode *node, int depth) {
  for (int i = 0; i < depth; i++)
    printf("  ");
  if (!node) {
    printf("(null)\n");
    return;
  }

  switch (node->type) {
  case NODE_COMMAND:
    printf("CMD:");
    for (int i = 0; i < node->argc; i++) {
      if (node->args[i].is_substitution)
        printf(" $(...)");
      else
        printf(" %s", node->args[i].text);
    }
    printf("\n");
    break;

  case NODE_AND:
    printf("AND\n");
    break;
  case NODE_OR:
    printf("OR\n");
    break;
  case NODE_SEQUENCE:
    printf("SEQ\n");
    break;
  case NODE_SUBSHELL:
    printf("SUBSHELL\n");
    break;
  case NODE_PIPE:
    printf("PIPE\n");
    break;
  case NODE_BANG:
    printf("BANG\n");
    break;
  case NODE_WRITE:
    printf("WRITE\n");
    break;
  case NODE_APPEND:
    printf("APPEND\n");
    break;
  case NODE_READ:
    printf("READ\n");
    break;
  case NODE_ERR:
    printf("ERR\n");
    break;
  case NODE_WRITE_ERR:
    printf("WRITE_ERR\n");
    break;
  case NODE_READWRITE:
    printf("READWRITE\n");
    break;
  case NODE_JOB:
    printf("JOB\n");
    break;
  default:
    printf("UNKNOWN\n");
    break;
  }

  if (node->left)
    print_ast(node->left, depth + 1);
  if (node->right)
    print_ast(node->right, depth + 1);
}
