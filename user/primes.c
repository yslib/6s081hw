#include "kernel/types.h"
#include "user/user.h"
int main(int argc, char *argv[]) {
  int i;
  int pid;
  pid = fork();
  if (pid == 0) {
    printf("thread(proc):%d\n", pid);
    pid = fork();
    if (pid == 0) {
      printf("thread(proc):%d\n", pid);
    } else if (pid > 0) {
      wait(0);
    } else {
      printf("fork failed\n");
    }
  } else if (pid > 0) {
    wait(0);
  } else {
    printf("fork failed\n");
  }
  exit(0);
}
