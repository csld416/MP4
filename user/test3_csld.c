#include "kernel/types.h"
#include "user/user.h"

// Helper to print and run a command
void run(char *cmd, char *args[]) {
    // Print command
    printf("$ %s", cmd);
    for (int i = 1; args[i] != 0; i++) {
        printf(" %s", args[i]);
    }
    printf("\n");

    // Fork and exec
    int pid = fork();
    if (pid == 0) {
        exec(cmd, args);
        printf("exec %s failed\n", cmd);
        exit(1);
    } else {
        wait(0);
    }
}

int main() {
    // Step 1: Generate test directory
    char *gen_args[] = {"gen", "3", 0};
    run("gen", gen_args);

    // Step 2: Create symlinks
    char *symln1[] = {"symln", "test4/dirX", "test4/dirXln1", 0};
    char *symln2[] = {"symln", "test4/dirX", "test4/dirXln2", 0};
    char *symln3[] = {"symln", "test4/dirX/a", "test4/aln1", 0};
    run("symln", symln1);
    run("symln", symln2);
    run("symln", symln3);

    // Step 3: chmod -R -rw test4/dirXln1
    char *chmod1[] = {"chmod", "-R", "-rw", "test4/dirXln1", 0};
    run("chmod", chmod1);

    // Step 4: chmod -R -rw test4/dirXln2 (expected to fail)
    char *chmod2[] = {"chmod", "-R", "-rw", "test4/dirXln2", 0};
    run("chmod", chmod2);

    // Step 5: Restore permissions on original dir
    char *chmod3[] = {"chmod", "-R", "+rw", "test4/dirX", 0};
    run("chmod", chmod3);

    exit(0);
}