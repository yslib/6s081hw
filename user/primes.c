#include "kernel/types.h"
#include "user/user.h"
void primes(int left) {
  int n, prime;
  int p[2];
  if (!read(left, &prime, 4)) {
    close(left);
    return;
  }
  printf("prime %d\n", prime);
  pipe(p);
  int pid = fork();
  if (pid == 0) {
    close(p[1]);
    primes(p[0]);
  } else if (pid > 0) {
    close(p[0]);
    while (read(left, &n, 4) > 0) {
      if (n % prime != 0) {
        write(p[1], &n, 4);
      }
    }
    close(p[1]);
    close(left);
    wait(0);
  }
}
int main(int argc, char *argv[]) {
  int p[2];
  int i;
  close(0);
  pipe(p);
  for (i = 2; i <= 35; i++) {
    write(p[1], &i, 4);
  }
  close(p[1]);
  primes(p[0]);
  exit(0);
}
