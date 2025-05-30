#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define MAXPATH 128

char g_original_path[MAXPATH];

void safestrcpy(char *dst, const char *src, int n)
{
    if (n <= 0)
        return;
    while (--n > 0 && *src)
        *dst++ = *src++;
    *dst = '\0';
}

void path_join(char *dest, const char *prefix, const char *name)
{
    safestrcpy(dest, prefix, MAXPATH);
    int len = strlen(dest);
    if (len < MAXPATH - 1)
    {
        dest[len] = '/';
        dest[len + 1] = '\0';
        safestrcpy(dest + len + 1, name, MAXPATH - len - 1);
    }
}

void usage(void)
{
    fprintf(2, "Usage: chmod [-R] (+|-)(r|w|rw|wr) file_name|dir_name\n");
    exit(1);
}

int resolve(const char *in, char out[MAXPATH])
{
    char buf[MAXPATH], tmp[MAXPATH];
    safestrcpy(buf, in, sizeof(buf));
    while (1)
    {
        int fd = open(buf, O_NOACCESS);
        if (fd < 0)
            return -1;

        struct stat st;
        if (fstat(fd, &st) < 0)
        {
            close(fd);
            return -1;
        }

        if (st.type != T_SYMLINK)
        {
            close(fd);
            break;
        }

        int n = read(fd, tmp, sizeof(tmp) - 1);
        close(fd);
        if (n <= 0)
            return -1;

        tmp[n] = '\0';
        safestrcpy(buf, tmp, sizeof(buf));
    }

    safestrcpy(out, buf, MAXPATH);
    return 0;
}

int parse_mode(char *s, int *op, int *bits)
{
    if (!(s[0] == '+' || s[0] == '-'))
        return -1;
    *op = (s[0] == '+') ? 1 : -1;
    *bits = 0;
    for (int i = 1; s[i]; i++)
    {
        if (s[i] == 'r')
            *bits |= M_READ;
        else if (s[i] == 'w')
            *bits |= M_WRITE;
        else
            return -1;
    }
    return 0;
}

int chmod_single(int op, int bits, char *path)
{
    struct stat st;
    int fd = open(path, O_NOACCESS);
    if (fd < 0)
    {
        fprintf(2, "chmod: cannot open %s\n", path);
        return -1;
    }

    if (fstat(fd, &st) < 0)
    {
        close(fd);
        fprintf(2, "chmod: cannot stat %s\n", path);
        return -1;
    }
    close(fd);

    int new_mode = (op > 0) ? (st.mode | bits) : (st.mode & ~bits);
    return chmod(new_mode, path);
}

int chmod_recursive(int op, int bits, char *resolved_path, char *display_path)
{
    struct stat st;
    int fd = open(resolved_path, O_NOACCESS);
    if (fd < 0)
    {
        fprintf(2, "chmod: cannot chmod %s\n", display_path);
        return -1;
    }
    if (fstat(fd, &st) < 0)
    {
        close(fd);
        fprintf(2, "chmod: cannot chmod %s\n", resolved_path);
        return -1;
    }
    close(fd);

    int is_dir = (st.type == T_DIR);

    // Step 1: If it's a directory and unreadable but needs to be traversed,
    // temporarily add read permission
    int temporarily_added_read = 0;
    if (is_dir && op < 0 && !(st.mode & M_READ))
    {
        if (op > 0 && (bits & M_READ))
        {
            // We're adding read permission → temporarily add r to enable
            // traversal
            if (chmod_single(+1, M_READ, resolved_path) < 0)
            {
                fprintf(2, "chmod: cannot chmod %s\n", g_original_path);
                return -1;
            }
            temporarily_added_read = 1;
            st.mode |= M_READ; // Update local stat to reflect it
        }
        else if (op < 0)
        {
            // printf("case4, yo what's up\n");
            fprintf(2, "chmod: cannot chmod %s\n", display_path);
            return -1;
        }
    }

    // Step 2: If directory, recurse into its children
    if (is_dir)
    {
        fd = open(resolved_path, O_RDONLY);
        if (fd < 0 && op > 0 && (bits & M_READ))
        {
            // Attempt to temporarily add read permission
            if (chmod_single(+1, M_READ, resolved_path) < 0)
            {
                fprintf(2, "chmod: cannot chmod %s\n", g_original_path);
                return -1;
            }
            fd = open(resolved_path, O_RDONLY);
            if (fd < 0)
            {
                fprintf(2, "chmod: cannot chmod %s\n", display_path);
                return -1;
            }
            temporarily_added_read = 1;
        }
        else if (fd < 0)
        {
            fprintf(2, "chmod: cannot chmod %s\n", display_path);
            return -1;
        }

        struct dirent de;
        char buf[512];
        while (read(fd, &de, sizeof(de)) == sizeof(de))
        {
            if (de.inum == 0 || strcmp(de.name, ".") == 0 ||
                strcmp(de.name, "..") == 0)
                continue;

            safestrcpy(buf, resolved_path, sizeof(buf));
            int pathlen = strlen(buf);
            buf[pathlen++] = '/';
            int namelen = strlen(de.name);
            if (namelen > DIRSIZ)
            {
                namelen = DIRSIZ;
            }
            memmove(buf + pathlen, de.name, namelen);
            buf[pathlen + namelen] = '\0';

            char next_display[MAXPATH];
            path_join(next_display, display_path, de.name);
            char resolved[MAXPATH];
            if (resolve(buf, resolved) < 0)
            {
                fprintf(2, "chmod: cannot resolve %s\n", buf);
                continue;
            }

            if (chmod_recursive(op, bits, resolved, next_display) < 0)
            {
                close(fd); // Don't leak FD
                return -1;
            }
        }
        close(fd);
    }

    // Step 3: Apply chmod to current path
    int pre_order = (bits == M_WRITE); // only +w or -w should trigger pre-order

    // Apply chmod in pre-order for +w or -w
    if (pre_order)
    {
        if (chmod_single(op, bits, resolved_path) < 0)
            fprintf(2, "chmod: cannot chmod %s\n", resolved_path);
    }

    // Step 4: If we added read permission temporarily, remove it now
    if (temporarily_added_read && (bits & M_READ))
    {
        if (op < 0 && chmod_single(-1, M_READ, resolved_path) < 0)
            fprintf(2, "chmod: cannot restore r from %s\n", resolved_path);
    }
    // Apply chmod in post-order for other cases
    if (!pre_order)
    {
        if (chmod_single(op, bits, resolved_path) < 0)
            fprintf(2, "chmod: cannot chmod %s\n", resolved_path);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    int recursive = 0, op = 0, bits = 0;
    char *mode_str, *target;

    if (argc == 4 && strcmp(argv[1], "-R") == 0)
    {
        recursive = 1;
        mode_str = argv[2];
        target = argv[3];
    }
    else if (argc == 3)
    {
        mode_str = argv[1];
        target = argv[2];
    }
    else
    {
        usage();
    }

    if (parse_mode(mode_str, &op, &bits) < 0)
        usage();

    // Save original user-provided path
    safestrcpy(g_original_path, target, MAXPATH);

    char realpath[MAXPATH];
    if (resolve(target, realpath) < 0)
    {
        fprintf(2, "chmod: cannot chmod %s\n", target);
        exit(1);
    }

    if (recursive)
    {
        struct stat st;
        int fd = open(realpath, O_NOACCESS);
        if (fd < 0 || fstat(fd, &st) < 0)
        {
            if (fd >= 0)
                close(fd);
            fprintf(2, "chmod: cannot chmod %s\n", target);
            exit(1);
        }
        close(fd);

        if (!(st.mode & M_READ) && op < 0)
        {
            // If unreadable, chmod -R should fail immediately
            fprintf(2, "chmod: cannot chmod %s\n", target);
            exit(1);
        }
        chmod_recursive(op, bits, realpath, target);
    }
    else if (chmod_single(op, bits, realpath) < 0)
    {
        fprintf(2, "chmod: cannot chmod %s\n", realpath);
    }
    exit(0);
}