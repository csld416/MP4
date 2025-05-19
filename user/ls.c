// 1
#include "kernel/types.h"
//
#include "kernel/stat.h"
//
#include "user/user.h"
//
#include "kernel/fs.h"
//
#include "kernel/fcntl.h"

#define MAXPATH 128

void safestrcpy(char *dst, const char *src, int n)
{
    if (n <= 0)
        return;
    while (--n > 0 && *src)
        *dst++ = *src++;
    *dst = '\0';
}

char *fmtname(char *path)
{
    static char buf[DIRSIZ + 1];
    char *p;

    // Find first character after last slash.
    for (p = path + strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    // Return blank-padded name.
    if (strlen(p) >= DIRSIZ)
        return p;
    memmove(buf, p, strlen(p));
    memset(buf + strlen(p), ' ', DIRSIZ - strlen(p));
    return buf;
}

/* TODO: Access Control & Symbolic Link */
void ls(char *path)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if (stat(path, &st) < 0 || !(st.mode & M_READ))
    {
        fprintf(2, "ls: cannot open %s\n", path);
        return;
    }
    else
    {
        printf("[DEBUG] stat type=%d ino=%d size=%d\n", st.type, st.ino,
               st.size);
    }
    fd = open(path, 0); // try O_RDONLY
    if (fd < 0)
    {
        fprintf(2, "ls: cannot open %s\n", path);
        return;
    }

    switch (st.type)
    {
    case T_FILE:
    {
        char perm[3];
        perm[0] = (st.mode & M_READ) ? 'r' : '-';
        perm[1] = (st.mode & M_WRITE) ? 'w' : '-';
        perm[2] = '\0';
        printf("%s %d %d %d %s\n", fmtname(path), st.type, st.ino, st.size,
               perm);
        break;
    }

    case T_DIR:
    {
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf))
        {
            printf("ls: path too long\n");
            break;
        }
        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';

        while (read(fd, &de, sizeof(de)) == sizeof(de))
        {
            if (de.inum == 0)
                continue;
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;

            struct stat entry_st;
            if (stat(buf, &entry_st) < 0)
            {
                printf("ls: cannot stat %s\n", buf);
                continue;
            }

            char perm[3];
            perm[0] = (entry_st.mode & M_READ) ? 'r' : '-';
            perm[1] = (entry_st.mode & M_WRITE) ? 'w' : '-';
            perm[2] = '\0';

            printf("%s %d %d %d %s\n", fmtname(buf), entry_st.type,
                   entry_st.ino, entry_st.size, perm);
        }
        break;
    }

    case T_SYMLINK:
    {
        char target[MAXPATH] = {0};
        printf("[DEBUG] Handling symlink: %s\n", path);

        close(fd);
        printf("[DEBUG] Closed original fd\n");

        if ((fd = open(path, O_NOACCESS)) < 0)
        {
            fprintf(2, "ls: cannot open symlink %s\n", path);
            break;
        }
        printf("[DEBUG] Reopened symlink %s with O_NOACCESS, fd=%d\n", path,
               fd);

        int n;
        if ((n = read(fd, target, MAXPATH - 1)) < 0)
        {
            fprintf(2, "ls: cannot read symlink %s\n", path);
            close(fd);
            break;
        }
        target[n] = '\0';
        close(fd);

        printf("[DEBUG] Symlink target resolved to: %s\n", target);

        struct stat target_st;
        if (stat(target, &target_st) < 0)
        {
            printf("[DEBUG] stat failed on target: %s\n", target);
            char perm[3] = {'r', 'w', '\0'};
            printf("%s %d %d %d %s -> %s (unresolved)\n", fmtname(path),
                   st.type, st.ino, st.size, perm, target);
            break;
        }

        printf("[DEBUG] Target stat succeeded: type=%d ino=%d size=%d\n",
               target_st.type, target_st.ino, target_st.size);

        if (target_st.type == T_DIR)
        {
            printf("[DEBUG] Target is a directory. Recursing into: %s\n",
                   target);
            ls(target);
        }
        else
        {
            char perm[3] = {'r', 'w', '\0'};
            printf("%s %d %d %d %s -> %s\n", fmtname(path), st.type, st.ino,
                   st.size, perm, target);
        }
        break;
    }
    }

    close(fd);
}

int main(int argc, char *argv[])
{
    int i;

    if (argc < 2)
    {
        ls(".");
        exit(0);
    }
    for (i = 1; i < argc; i++)
        ls(argv[i]);
    exit(0);
}
