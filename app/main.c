#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>


int main(int argc, char **args, char **env) {
  // Flush after every printf
  setbuf(stdout, NULL);
  pid_t pid;
  int wstatus;

  do {
  printf("$ ");
  char input[100];
  fgets(input, 100, stdin);
  input[strlen(input) - 1] = '\0';
  token_t *token = tokenize(input);
  if (token && strcmp(token->cmd, "exit") == 0) {
      int code = atoi(token->args[1]);
      freeToken(token);
      exit(code);
  } else if (token && strcmp(token->cmd, "echo") == 0) {
      echo(token);
  } else if (token && strcmp(token->cmd, "type") == 0) {
      type(token);
  } else if (token && strcmp(token->cmd, "pwd") == 0) {
      pwd();
  } else if (token && strcmp(token->cmd, "cd") == 0) {
      cd(token);
  } else if (token->isExe == 0 && token->cmd[0] != '/' && token->cmd[0] != '.') {
    printf("%s: command not found\n", input);
  } else {
      if ((pid = fork()) == 0) {
      execve(token->cmd, token->args, env);
      exit(0);
      }
  }
  wait(NULL);
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
  for (int idx = 0; str[idx]; idx++) {
    if (str[idx] == '\n') continue;
    if (str[idx] == ' ') {
      curr_token[i] = '\0';
      i = 0;
      if (t_count) {
        token->args[t_count] = strdup(curr_token);
        t_count++;
      } else {
        char *filepath = find_cmd_path(curr_token);
        if (filepath) {
          token->cmd = strdup(filepath);
          free(filepath);
          token->isExe = 1;
        } else {
          token->cmd = strdup(curr_token);
          token->isExe = 0;
        }
        token->args[t_count] = strdup(curr_token);
        t_count++;
      }
      continue;
    }
    curr_token[i++] = str[idx];
  }
  curr_token[i] = '\0';
  if (t_count) {
    token->args[t_count++] = strdup(curr_token);
  } else {
    char *filepath = find_cmd_path(curr_token);
    if (filepath) {
      token->cmd = strdup(filepath);
      free(filepath);
      token->isExe = 1;
    } else {
      token->cmd = strdup(curr_token);
      token->isExe = 0;
    }
    token->args[t_count++] = strdup(curr_token);
  }
  token->args[t_count] = NULL;
  return token;
}

void type(token_t *token) {
  char *builtin[] = {"exit", "echo", "type", "pwd", "cd", NULL};
  // start index at one because the first arg(arg[0]) is the cmd itself
  for (int i = 1; token->args[i]; i++) {
    int found = 0;
    for (int j = 0; builtin[j]; j++) {
      if (strcmp(token->args[i], builtin[j]) == 0) {
        printf("%s is a shell builtin\n", token->args[i]);
        found = 1;
        break;
      }
      if (j == 0) {
        char *filepath = find_cmd_path(token->args[i]);
        if (filepath != NULL) {
          char* fpath = filepath;
          printf("%s is %s\n", token->args[i], fpath);
          free(filepath);
          found = 1;
          break;
        }
      };
    }
    if (!found) printf("%s: not found\n", token->args[i]);
  }
}

void echo(token_t *token) {
  // start index at one because the first arg(arg[0]) is the cmd itself
  for (int i = 1; token->args[i]; i++) {
    printf("%s", token->args[i]);
    if (token->args[i + 1]) printf(" ");
  }
  putchar('\n');
}

void pwd(void) {
  puts(getenv("PWD"));
}

void cd(token_t *token) {
  if (token->args[1] != NULL && access(token->args[1], R_OK) == 0) {
    setenv("PWD", token->args[1], 1);
  } else {
    printf("%s: %s: No such file or directory\n", token->cmd, token->args[1]);
  }
}

char* find_cmd_path(char *s) {
  // cmds to ignore finding path for
  char *special_cmd[] = {"echo", "pwd", "cd", NULL};
  int i = 0;
  char *path;
  char *fpath;
  size_t s_len;

  if (s[0] == '/' || s[0] == '.') return NULL;

  while (special_cmd[i]) {
    if (strcmp(special_cmd[i++], s) == 0) return NULL;
  }

  path = getenv("PATH");
  fpath = strtok(path, ":");
  s_len = strlen(s);
  while(fpath) {
    size_t fpath_len = strlen(fpath);
    char filepath[s_len + fpath_len + 2];
    strcpy(filepath, fpath);
    strcat(filepath, "/");
    strcat(filepath, s);
    filepath[fpath_len + s_len + 2] = '\0';
    if (access(filepath, R_OK) == 0) {
      return strdup(filepath);
    }
    fpath = strtok(NULL, ":");
  }
  char filepath[s_len + 6];
  strcpy(filepath, "/bin/");
  strcat(filepath, s);
  filepath[s_len + 6] = '\0';
  if (access(filepath, R_OK) == 0) {
    return strdup(filepath);
  }
  return NULL;
}
