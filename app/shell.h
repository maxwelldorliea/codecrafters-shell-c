#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>

typedef struct token_s {
	char *cmd;
	char *args[10];
} token_t;

token_t *tokenize(char* s);
void freeToken(token_t *token);
void echo(token_t *token);

#endif
