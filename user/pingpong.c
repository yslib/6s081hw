#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  int pid;
  int id;
  int p[4];
  char r = 'a';
  char r1;
  pipe(p);
  pipe(p+2);
  pid = fork();
  id = getpid();
  char c = 'y';
  if (pid == 0){
    read(p[0],&r,1);
    if(r == c){
      printf("%d: received ping\n",id);
      write(p[3],&c,1);
    }
  }else if(pid > 0){
    write(p[1],&c,1);
    int count = read(p[2],&r1,1);
    if(r1 == c){
      printf("%d: received pong\n",id);
    }
    wait(0);
  }else{
    printf("fork failed\n");
  }
  exit(0);
}
