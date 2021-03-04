#include "kernel/param.h"
#include "kernel/types.h"
#include "user/user.h"
#define BUF_LEN 512
#define PROG_LEN 256
int main(int argc, char **argv) {
  // receiving std input by words and interpret them as the argv of the program
  char in[BUF_LEN];
  char *argbuf[MAXARG];
  char *p = 0;
  int count = 0;
  int first = 1;
  int i;
  for (i = 0; i < MAXARG; i++) {
    argbuf[i] = 0;
  }
  if (argc == 1) {
    exit(0);
  } else {
    for(i = 0;i<MAXARG && i+1 < argc;i++){
      argbuf[count++] = argv[i+1];
    }
    while (*gets(in, sizeof(in)) != '\0') {
      //printf("prog: %s, stdin: %s\n", prog, in);
      p = ((*in) == '\0' ? 0 : in);
      while (*p) {
        if (*p == ' ') {
          *p = '\0'; // replace ' ' with '\0' to indicate a single argument
          first = 1;
        } else if (*p != ' ' && first == 1) {
          first = 0;
          argbuf[count++] = p;
        }
        p++;
      }
      for (i = 0; i < MAXARG; i++) {
        printf("arg %d: %s\n",i,argbuf[i]);
      }
      if (fork() == 0) {
        exec(argbuf[0], argbuf);
        break;
      } else {
        wait(0);
      }
    }
  }
  exit(0);
}
