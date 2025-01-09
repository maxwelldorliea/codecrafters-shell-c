#include <stdio.h>
#include <string.h>
#include "shell.h"

int main() {
  // Flush after every printf
  setbuf(stdout, NULL);

  do {
  printf("$ ");
  char input[100];
  fgets(input, 100, stdin);
  input[strlen(input) - 1] = '\0';
  token_t *token = tokenize(input);
  if (token && strcmp(token->cmd, "exit") == 0) {
      int code = atoi(token->args[0]);
      freeToken(token);
      exit(code);
  } else {
    printf("%s: command not found\n", input);
  }
  freeToken(token);
  }while (1);

  return 0;
}

void freeToken(token_t *token) {
  if (token == NULL) return;
  if (token->cmd) free(token->cmd);
  if (token->args[0]) for(int i = 0; token->args[i]; i++) free(token->args[i]);
  if (token) free(token);
}

token_t *tokenize(char *s) {
  size_t s_len = strlen(s) + 1;
  char str[s_len];
  strcpy(str, s);
  int i = 0, t_count = 0;
  char curr_token[100];
  token_t *token = malloc(sizeof(*token));
  for (int idx = 0; str[idx] != '\0'; idx++) {
    if (str[idx] == '\n') continue;
    if (str[idx] == ' ') {
      curr_token[i] = '\0';
      i = 0;
      if (t_count) {
        token->args[t_count - 1] = strdup(curr_token);
        t_count++;
      } else {
        token->cmd = strdup(curr_token);
        t_count++;
      }
    }
    curr_token[i++] = str[idx];
  }
  curr_token[i] = '\0';
  if (t_count) token->args[t_count - 1] = strdup(curr_token);
  else token->cmd = strdup(curr_token);
  token->args[t_count] = NULL;
  return token;
}
