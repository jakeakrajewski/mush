#ifndef EXECUTE_H
#define EXECUTE_H

#import "tokenizer.h"

int execute(ASTNode *node, int silent);
int mu_execute_logical_commands(char *line);

#endif
