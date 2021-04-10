#include "kernel/types.h"
#include "user/user.h"

int main(int argc,char ** argv){
  symlink(0,0);
  return 0;
}
