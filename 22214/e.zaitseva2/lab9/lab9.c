#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
  pid_t pid = fork();

  if (pid == -1) {
    perror("error in fork");
    exit(1);
  } else if (pid == 0) {
    execlp("cat", "cat", "lab9.c", NULL);
  } else {
    wait(NULL);
    printf("\nI am here\n");
  }

  return 0;
}