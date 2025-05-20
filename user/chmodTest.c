#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define check(cond, name) do { \
    if (cond) { \
        printf("%s: PASS\n", name); \
        score++; \
    } else { \
        printf("%s: FAIL\n", name); \
    } \
} while (0)

void setup()
{
    mkdir("ctest");
    mkdir("ctest/d");
    int fd = open("ctest/d/f", O_CREATE | O_RDWR);
    write(fd, "x", 1);
    close(fd);

    symlink("ctest/d", "ctest/symlink_dir");
    symlink("ctest/d/f", "ctest/symlink_file");
}

void reset()
{
    chmod(+1, "ctest/d");
    chmod(+1, "ctest/d/f");
    chmod(+1, "ctest/symlink_dir");
    chmod(+1, "ctest/symlink_file");
    chmod(+1, "ctest");
}

int is_readable(char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return 0;
    close(fd);
    return 1;
}

int is_file_readable(char *path)
{
    int fd = open(path, O_RDONLY);
    char buf[1];
    int n = read(fd, buf, 1);
    close(fd);
    return n > 0;
}

int main()
{
    setup();

    int score = 0;
    int ret;

    // Case 1: chmod -R -r d (and d is readable)
    reset();
    chmod(+1, "ctest/d"); // ensure readable
    ret = fork();
    if (ret == 0) {
        char *argv[] = { "chmod", "-R", "-r", "ctest/d", 0 };
        exec("chmod", argv);
        exit(1);
    } else {
        wait(0);
        check(!is_readable("ctest/d/f"), "Case1 -R -r d (readable)");
    }

    // Case 2: chmod -R +r d (and d is unreadable)
    reset();
    chmod(-1, "ctest/d");
    ret = fork();
    if (ret == 0) {
        char *argv[] = { "chmod", "-R", "+r", "ctest/d", 0 };
        exec("chmod", argv);
        exit(1);
    } else {
        wait(0);
        check(is_readable("ctest/d"), "Case2 -R +r d (unreadable)");
    }

    // Case 3: chmod -R -r symlink_dir (target is unreadable)
    reset();
    chmod(-1, "ctest/d");
    ret = fork();
    if (ret == 0) {
        char *argv[] = { "chmod", "-R", "-r", "ctest/symlink_dir", 0 };
        exec("chmod", argv);
        exit(1);
    } else {
        wait(0);
        check(!is_readable("ctest/symlink_dir"), "Case3 -R -r symlink_dir");
    }

    // Case 4: chmod +w symlink_file (even if target unreadable)
    reset();
    chmod(-1, "ctest/d/f");
    ret = fork();
    if (ret == 0) {
        char *argv[] = { "chmod", "+w", "ctest/symlink_file", 0 };
        exec("chmod", argv);
        exit(1);
    } else {
        wait(0);
        check(is_file_readable("ctest/d/f"), "Case4 +w symlink_file");
    }

    // Case 5: chmod -R +rw symlink_dir (target dir ok)
    reset();
    chmod(-1, "ctest/d");
    chmod(-1, "ctest/d/f");
    ret = fork();
    if (ret == 0) {
        char *argv[] = { "chmod", "-R", "+rw", "ctest/symlink_dir", 0 };
        exec("chmod", argv);
        exit(1);
    } else {
        wait(0);
        check(is_readable("ctest/d/f"), "Case5 -R +rw symlink_dir");
    }

    printf("Final Score: %d / 5\n", score);
    exit(0);
}