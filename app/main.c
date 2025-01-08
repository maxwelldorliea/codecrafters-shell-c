#include <stdio.h>
#include <string.h>

int main() {
  // Flush after every printf
  setbuf(stdout, NULL);


  do {
  printf("$ ");
  char input[100];
  fgets(input, 100, stdin);
  input[strlen(input) - 1] = '\0';
  printf("%s: command not found\n", input);
  }while (1);

  // Wait for user input
  return 0;
}
