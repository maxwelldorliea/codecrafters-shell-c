#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>


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
  if (token->redirectPath != NULL) {
      freopen(token->redirectPath, token->redirectMode, token->redirectLocation);
  }
  if (token && strcmp(token->cmd, "exit") == 0) {
      int code = 0;
      if (token->args[1]) code = atoi(token->args[1]);
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
  freopen("/dev/tty", "w", stdout);
  }while (1);

  return 0;
}

void freeToken(token_t *token) {
  if (token == NULL) return;
  if (token->cmd) free(token->cmd);
  if (token->redirectPath != NULL) free(token->redirectPath);
  if (token->args[0]) for(int i = 0; token->args[i]; i++) free(token->args[i]);
  if (token) free(token);
}

int isRedirected(token_t *token, char *s) {
  if (strcmp(s, ">") == 0 || strcmp(s, "1>") == 0) {
    token->redirectLocation = stdout;
    token->redirectMode = "w+";
    return 1;
  } else  if (strcmp(s, ">>") == 0 || strcmp(s, "1>>") == 0) {
    token->redirectLocation = stdout;
    token->redirectMode = "a+";
    return 1;
  } else if (strcmp(s, "2>") == 0) {
    token->redirectLocation = stderr;
    token->redirectMode = "w+";
    return 1;
  } else if (strcmp(s, "2>>") == 0) {
    token->redirectLocation = stderr;
    token->redirectMode = "a+";
    return 1;
  }
  return 0;
}

token_t *tokenize(char *s) {
  size_t s_len = strlen(s) + 1;
  char str[s_len];
  strcpy(str, s);
  int i = 0, t_count = 0;
  char curr_token[100];
  char quote = '\0';
  int redirect = 0;
  token_t *token = malloc(sizeof(*token));
  token->redirectPath = NULL;
  token->redirectLocation = NULL;
  token->redirectMode = NULL;
  for (int idx = 0; str[idx]; idx++) {
    if (i == 0 && (s[idx] == '"' || s[idx] == '\'')) {
      if (s[idx] == '"') quote = '"';
      else quote = '\'';
      continue;
    }
    if ((!quote || quote == '"') && str[idx] == '\\' && str[idx + 1]) {
      char next_char = str[idx + 1];
      if (quote == '"') {
        if (next_char == '"' || next_char == '\\'
          || next_char == '$') {
          idx++;
          curr_token[i++] = str[idx];
          continue;
        }
      } else {
      idx++;
      curr_token[i++] = str[idx];
      continue;
      }
    }
    if (quote && str[idx] == quote && str[idx + 1] != ' ') continue;
    if (i == 0 && s[idx] == ' ' && !quote) continue;
    if (str[idx] == '\n') continue;
    if (str[idx] == ' ' && !quote) {
      curr_token[i] = '\0';
      i = 0;
      if (t_count) {
        if (isRedirected(token,curr_token)) {
          redirect = 1;
        } else {
          if (redirect) {
            token->redirectPath = strdup(curr_token);
            break;
          } else {
            token->args[t_count++] = strdup(curr_token);
          }
        }
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
    if (str[idx] == quote && str[idx + 1] == quote) {
      idx += 2;
      while(str[idx] && str[idx] != quote) {
        curr_token[i++] = str[idx++];
      }
    }
    if (str[idx] == quote) {
      curr_token[i] = '\0';
      i = 0;
      quote = '\0';
      if (t_count) {
        if (isRedirected(token,curr_token)) {
          redirect = 1;
        } else {
          if (redirect) {
            token->redirectPath = strdup(curr_token);
            break;
          } else {
            token->args[t_count++] = strdup(curr_token);
          }
        }
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
      continue;
    }
    curr_token[i++] = str[idx];
  }
  if (i == 0) {
    token->args[t_count] = NULL;
    return token;
  }
  curr_token[i] = '\0';
  if (t_count) {
    if (isRedirected(token,curr_token)) {
      redirect = 1;
          puts("I found it max");
    } else {
      if (redirect) token->redirectPath = strdup(curr_token);
      else
        token->args[t_count++] = strdup(curr_token);
    }
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
    if (token->args[i + 1] != NULL) printf(" ");
  }
  putchar('\n');
}

void pwd(void) {
  puts(getenv("PWD"));
}

void cd(token_t *token) {
  char *pwd_path;
  size_t pwd_len;
  size_t arg_len;
  char *home_dir;
  size_t home_dir_len;
  char *arg;
  char path[200];

  if (token->args[1] == NULL) {
    printf("%s: %s: No such file or directory\n", token->cmd, token->args[1]);
    return;
  }

  switch (token->args[1][0]) {
    case '/':
      if (access(token->args[1], R_OK) == 0) {
        setenv("PWD", token->args[1], 1);
      } else {
        printf("%s: %s: No such file or directory\n", token->cmd, token->args[1]);
      }
      break;
    case '~':
      arg = token->args[1];
      arg_len = strlen(arg);
      home_dir = getenv("HOME");
      home_dir_len = strlen(home_dir);
      arg++;
      if (arg_len == 2) { // cd ~/ avoid adding the trailing / to the path
        arg++;
        arg_len -= 1;
      }
      strcpy(path, home_dir);
      strcat(path, arg);
      path[arg_len + home_dir_len] = '\0';
      if (access(path, R_OK) == 0) {
        setenv("PWD", path, 1);
      } else {
        printf("%s: %s: No such file or directory\n", token->cmd, token->args[1]);
      }
      break;
    case '.':
      pwd_path = getenv("PWD");
      pwd_len = strlen(pwd_path);
      arg_len = strlen(token->args[1]);
      if (token->args[1][1] == '/') {
        char *arg = token->args[1];
        arg++;
        strcpy(path, pwd_path);
        strcat(path, arg);
        path[pwd_len + arg_len] = '\0';
        if (access(path, R_OK) == 0) {
          setenv("PWD", path, 1);
        } else {
          printf("%s: %s: No such file or directory\n", token->cmd, token->args[1]);
        }
        break;
      } else if (token->args[1][1] == '.' && token->args[1][2] == '/') {
        char *arg = token->args[1];
        char pwdc[pwd_len + 1];
        strcpy(pwdc, pwd_path);
        pwdc[pwd_len + 1] = '\0';
        int i = 0;
        while (arg[i] == '.' && arg[i + 1] == '.' && arg[i + 2] == '/') {
          while (pwdc[pwd_len] != '/') pwd_len--;
          pwdc[pwd_len] = '\0';
          if (isalnum(arg[i + 3])) {
            arg++;
            arg++;
            arg_len -= 2;
            break;
          }
          arg++;
          arg++;
          arg++;
          arg_len -= 3;
        }
        if (arg) {
          char path[pwd_len + arg_len];
          strcpy(path, pwdc);
          strcat(path, arg);
          path[pwd_len + arg_len] = '\0';
          if (access(path, R_OK) == 0) {
            setenv("PWD", path, 1);
          } else {
            printf("%s: %s: No such file or directory\n", token->cmd, token->args[1]);
          }
        } else {
          if (access(pwdc, R_OK) == 0) {
            setenv("PWD", pwdc, 1);
          } else {
            printf("%s: %s: No such file or directory\n", token->cmd, token->args[1]);
          }
          break;
        }
      }
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
