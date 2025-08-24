#ifndef SHELL_PARSER_H
#define SHELL_PARSER_H

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
} TokenType;

typedef struct {
  TokenType type;
  char *text;
} Token;

void tokenize(const char *input);
Token *peek();
Token *consume();
int match(TokenType type);

// === AST ===

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
  NODE_SUBSTITUTE, // Fixed typo: was NODE_SUBSTITUE
  NODE_BANG,       // Fixed typo: was NODE_SUBSTITUE
  NODE_JOB,        // Fixed typo: was NODE_SUBSTITUE
} NodeType;

typedef struct Redirection {
  int fd;
  NodeType type;
  char *filename; // remains valid if not a close
  int close_fd;   // NEW: 1 if this is a close (e.g., 2>&-)
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

char *process_quotes(const char *word);
ASTNode *parse_sequence();
void print_ast(ASTNode *node, int depth);

#endif
