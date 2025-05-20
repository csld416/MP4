#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

void usage() {
    printf("Usage: gen2\n");
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 1) {
        usage();
    }

    // Create directory
    if (mkdir("test3") < 0) {
        printf("mkdir test3 failed\n");
        exit(1);
    }

    // Create d1 and d2 inside test3
    if (mkdir("test3/d1") < 0 || mkdir("test3/d2") < 0) {
        printf("mkdir d1 or d2 failed\n");
        exit(1);
    }

    // Create symlink chain: d1ln_1 -> d1, ..., d1ln_4 -> d1ln_3
    if (symlink("test3/d1", "test3/d1ln_1") < 0) {
        printf("symlink d1ln_1 failed\n");
        exit(1);
    }

    if (symlink("test3/d1ln_1", "test3/d1ln_2") < 0) {
        printf("symlink d1ln_2 failed\n");
        exit(1);
    }

    if (symlink("test3/d1ln_2", "test3/d1ln_3") < 0) {
        printf("symlink d1ln_3 failed\n");
        exit(1);
    }

    if (symlink("test3/d1ln_3", "test3/d1ln_4") < 0) {
        printf("symlink d1ln_4 failed\n");
        exit(1);
    }

    exit(0);
}
