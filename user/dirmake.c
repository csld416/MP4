#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

#define MAXPATH 128

void safestrcpy(char *dst, const char *src, int n) {
  if(n <= 0)
    return;
  while(--n > 0 && *src)
    *dst++ = *src++;
  *dst = '\0';
}

void itoa(int num, char *buf) {
  // Converts a positive int to string in buf
  int i = 0;
  char tmp[16];
  if (num == 0) {
    buf[0] = '0';
    buf[1] = '\0';
    return;
  }
  while (num > 0) {
    tmp[i++] = (num % 10) + '0';
    num /= 10;
  }
  int j = 0;
  while (i > 0) {
    buf[j++] = tmp[--i];
  }
  buf[j] = '\0';
}

void make_path(char *prefix, int i, char *out) {
  char numbuf[16];
  itoa(i, numbuf);
  safestrcpy(out, prefix, MAXPATH);
  int len = strlen(out);
  if (len + strlen(numbuf) >= MAXPATH)
    return;
  safestrcpy(out + len, numbuf, MAXPATH - len);
}

void usage() {
  write(2, "Usage: dirmake n\n", 17);
  exit(1);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    usage();
  }

  int n = atoi(argv[1]);
  if (n <= 0 || n >= 100) {
    usage();
  }

  char prev[MAXPATH] = {0};
  char curr[MAXPATH] = {0};

  make_path("d", 1, curr);
  if (mkdir(curr) < 0) {
    write(2, "mkdir failed\n", 13);
    exit(1);
  }

  safestrcpy(prev, curr, MAXPATH);

  for (int i = 2; i <= n; i++) {
    make_path("d", i, curr);
    if (symlink(prev, curr) < 0) {
      write(2, "symlink failed\n", 15);
      exit(1);
    }
    safestrcpy(prev, curr, MAXPATH);
  }

  exit(0);
}