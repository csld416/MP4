#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void usage() {
    fprintf(2, "Usage: chmod (+|-)r|w|rw file_name\n");
    exit(1);
}

int parse_mode(char *s) {
    int mode = 0;
    if (s[0] == '+' || s[0] == '-') {
        for (int i = 1; s[i]; i++) {
            if (s[i] == 'r') mode |= 1;
            else if (s[i] == 'w') mode |= 2;
            else usage();
        }
        return (s[0] == '+') ? mode : -mode;
    }
    usage();
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) usage();

    int mode = parse_mode(argv[1]);
    int res = chmod(mode, argv[2]);

    if (res < 0)
        fprintf(2, "chmod: cannot chmod %s\n", argv[2]);

    exit(0);
}