#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#define PATH_MAX_LEN 100
#define FILENAME_MAX_LEN 100


void find(const char *path, const char *name) {
  if (path == (void *)0)
    return;

  //printf("current dir: %s\n",path);
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if ((fd = open(path, 0)) < 0) {
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }

  strcpy(buf,path);
  p = buf + strlen(buf);
  *p++='/';

  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    if (de.inum == 0)
      continue;
    if(de.name[0] == '.' || de.name[1] == '.')
      continue;
    if (stat(buf, &st) < 0) {
      //printf("ls: cannot stat %s\n", buf);
      continue;
    }
    memmove(p, de.name, DIRSIZ);
    p[DIRSIZ] = 0;
    printf("item: %s : %s : %d\n",buf,de.name,st.type);
    switch (st.type) {
    case T_FILE:
      if(strcmp(de.name,name) == 0){
        printf("%s\n", p);
      }
      break;
    case T_DIR:
      if(strcmp(de.name,name) == 0){
        printf("%s\n", p);
      }
      find(buf,name);
      break;
    }
  }

  close(fd);
}

int main(int argc, char **argv) {
  if (argc != 3) {
    exit(0);
  }
  char path[100];
  char filename[100];
  strcpy(path, argv[1]);
  strcpy(filename, argv[2]);
  find(path, filename);
  exit(0);
}
