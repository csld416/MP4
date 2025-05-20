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

        // Close the initial fd, then re-open the link itself w/o following
        close(fd);
        fd = open(path, O_NOACCESS);
        if (fd < 0)
        {
            fprintf(2, "ls: cannot open %s\n", path);
            break;
        }
        if (fstat(fd, &st) < 0)
        {
            fprintf(2, "ls: cannot stat %s\n", path);
            close(fd);
            break;
        }

        // Read the link’s content (the target path)
        int n = read(fd, target, MAXPATH - 1);
        if (n < 0)
        {
            fprintf(2, "ls: cannot read %s\n", path);
            close(fd);
            break;
        }
        target[n] = '\0';
        close(fd);

        // If it points to a directory, list that directory instead
        struct stat st_target;
        if (stat(target, &st_target) == 0 && st_target.type == T_DIR)
        {
            ls(target);
        }
        else
        {
            // Otherwise, print the link itself.  Permissions are always 'rw'
            char perm[3] = "rw";
            printf("%s %d %d %d %s\n",
                   fmtname(path), // link name
                   st.type,       // should be T_SYMLINK (4)
                   st.ino,        // link’s inode
                   st.size,       // link’s size (length of target path)
                   perm);
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
