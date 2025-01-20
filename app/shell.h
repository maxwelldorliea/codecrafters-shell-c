#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

typedef struct token_s {
	char *cmd;
	char *args[10];
	int isExe;
	char *redirectPath;
	FILE *redirectLocation;
	char *redirectMode;
} token_t;

token_t *tokenize(char* s);
void freeToken(token_t *token);
void echo(token_t *token);
void type(token_t *token);
void pwd(void);
void cd(token_t *token);
char* find_cmd_path(char *s);
int isRedirected(token_t *token, char *s);

#endif
