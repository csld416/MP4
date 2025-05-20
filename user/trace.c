#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

#define MAXPATH 128

void safestrcpy(char *dst, const char *src, int n) {
    int i;
    for (i = 0; i < n - 1 && src[i]; i++)
        dst[i] = src[i];
    dst[i] = '\0';
}

void trace(char *start) {
    char buf[MAXPATH];
    safestrcpy(buf, start, MAXPATH);

    int depth = 0;
    while (1) {
        if (depth++ > 20) {
            printf("\n[ERROR] too many symlink levels, possible loop\n");
            return;
        }
        if (depth > 0)
            printf(" -> ");
        printf("%s", buf);
        depth++;

        int fd = open(buf, O_NOACCESS);
        if (fd < 0) {
            printf("\n[ERROR] cannot open %s\n", buf);
            return;
        }

        struct stat st;
        if (fstat(fd, &st) < 0) {
            close(fd);
            printf("\n[ERROR] cannot stat %s\n", buf);
            return;
        }

        if (st.type != T_SYMLINK) {
            close(fd);
            break;
        }

        char target[MAXPATH];
        int n = read(fd, target, sizeof(target) - 1);
        close(fd);
        if (n <= 0) {
            printf("\n[ERROR] read failed\n");
            return;
        }
        target[n] = '\0';

        safestrcpy(buf, target, MAXPATH);
    }

    printf("\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(2, "Usage: trace <path>\n");
        exit(1);
    }
    trace(argv[1]);
    exit(0);
}