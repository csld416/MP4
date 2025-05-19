#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int main() {
    int fd = open("f", O_NOACCESS);
    if (fd < 0) {
        printf("open failed\n");
        exit(1);
    }

    char buf[16];
    int n = read(fd, buf, sizeof(buf));
    if (n < 0)
        printf("read failed as expected\n");
    else {
        buf[n] = '\0';
        printf("read succeeded: %s\n", buf);
    }

    close(fd);
    exit(0);
}