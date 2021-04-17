#include "kernel/types.h"
#include "user/user.h"

int main(int argc,char ** argv){
  symlink("ls","ls_link");
  return 0;
}
