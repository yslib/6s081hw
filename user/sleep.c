#include "kernel/types.h"
#include "user/user.h"


int main(int argc, char *argv[]) {
  int t;
  if (argc == 2) {
    t = atoi(argv[1]);
    sleep(t);
  }
  exit(0);
}
