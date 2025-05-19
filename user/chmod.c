#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define MAXPATH 128

static void
safestrcpy(char *dst, const char *src, int n)
{
    int i;
    for (i = 0; i < n - 1 && src[i]; i++)
        dst[i] = src[i];
    dst[i] = '\0';
}

// Print usage and exit
void
usage(void) {
    fprintf(2, "Usage: chmod [-R] (+|-)(r|w|rw|wr) file_name|dir_name\n");
    exit(1);
}

// Parse mode string into operation and bitmask
int
parse_mode(char *s, int *op, int *bits) {
    if (!(s[0] == '+' || s[0] == '-'))
        return -1;
    *op = (s[0] == '+') ? 1 : -1;
    *bits = 0;
    for (int i = 1; s[i]; i++) {
        if (s[i] == 'r') *bits |= 1;
        else if (s[i] == 'w') *bits |= 2;
        else return -1;
    }
    return 0;
}

// Single chmod: positive op adds, negative op removes
int chmod_single(int op, int bits, char *path) {
    struct stat st;
    char resolved[MAXPATH], tmp[MAXPATH];
    int fd, n;

    // start at the user-passed path
    safestrcpy(resolved, path, sizeof(resolved));

    // chase symlinks
    while(1) {
        if(stat(resolved,&st)<0) {
            fprintf(2,"chmod: cannot stat %s\n", resolved);
            return -1;
        }
        if(st.type != T_SYMLINK)
            break;
        if((fd = open(resolved,O_NOACCESS))<0) {
            fprintf(2,"chmod: cannot open symlink %s\n", resolved);
            return -1;
        }
        if((n = read(fd, tmp, sizeof(tmp)-1)) <= 0) {
            close(fd);
            fprintf(2,"chmod: cannot read symlink %s\n", resolved);
            return -1;
        }
        tmp[n] = '\0';
        close(fd);
        safestrcpy(resolved, tmp, sizeof(resolved));
    }
    // finally invoke the kernel syscall
    return chmod(op>0 ? (st.mode|bits) : (st.mode&~bits), resolved);
}


// Recursive chmod helper
void
chmod_recursive(int op, int bits, char *path) {
    struct stat st;
    if (stat(path, &st) < 0) {
        fprintf(2, "chmod: cannot chmod %s\n", path);
        return;
    }
    int is_symlink = (st.type == T_SYMLINK);
    int is_dir = (st.type == T_DIR);

    // Resolve symlink once
    if (is_symlink) {
        char target[512];
        int fd = open(path, O_NOACCESS);
        if (fd >= 0) {
            read(fd, target, sizeof(target));
            close(fd);
            chmod_recursive(op, bits, target);
        }
        return;  // Do not chmod the symlink itself
    }

    // Pre-order: ensure directory is readable when adding
    if (is_dir && op == 1) {
        if (!(st.mode & 1)) {
            if (chmod_single(1, 1, path) < 0) {
                fprintf(2, "chmod: cannot chmod %s\n", path);
                return;
            }
            st.mode |= 1;
        }
    }

    // Determine traversal order: post-order for removing 'r'
    int post_order = is_dir && op == -1 && (bits & 1);
    if (post_order) {
        // Recurse into children first
        int fd = open(path, O_NOACCESS);
        if (fd < 0) {
            fprintf(2, "chmod: cannot chmod %s\n", path);
        } else {
            struct dirent de;
            char buf[512];
            while (read(fd, &de, sizeof(de)) == sizeof(de)) {
                if (de.inum == 0) continue;
                if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                    continue;
                safestrcpy(buf, path, sizeof(buf));
                int pathlen = strlen(buf);
                buf[pathlen++] = '/';
                int namelen = strlen(de.name);
                if (namelen > DIRSIZ) namelen = DIRSIZ;
                memmove(buf + pathlen, de.name, namelen);
                buf[pathlen + namelen] = '\0';
                chmod_recursive(op, bits, buf);
            }
            close(fd);
        }
        // Then chmod this directory
        if (chmod_single(op, bits, path) < 0)
            fprintf(2, "chmod: cannot chmod %s\n", path);

    } else {
        // Pre-order: chmod current entry
        if (chmod_single(op, bits, path) < 0)
            fprintf(2, "chmod: cannot chmod %s\n", path);
        // Then recurse if directory
        if (is_dir) {
            int fd = open(path, 0);
            if (fd < 0) {
                fprintf(2, "chmod: cannot chmod %s\n", path);
                return;
            }
            struct dirent de;
            char buf[512];
            while (read(fd, &de, sizeof(de)) == sizeof(de)) {
                if (de.inum == 0) continue;
                if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                    continue;
                safestrcpy(buf, path, sizeof(buf));
                int pathlen = strlen(buf);
                buf[pathlen++] = '/';
                int namelen = strlen(de.name);
                if (namelen > DIRSIZ) namelen = DIRSIZ;
                memmove(buf + pathlen, de.name, namelen);
                buf[pathlen + namelen] = '\0';
                chmod_recursive(op, bits, buf);
            }
            close(fd);
        }
    }
}

int
main(int argc, char *argv[]) {
    int recursive = 0;
    int op = 0, bits = 0;
    char *mode_str, *target;

    if (argc == 4 && strcmp(argv[1], "-R") == 0) {
        recursive = 1;
        mode_str  = argv[2];
        target    = argv[3];
    } else if (argc == 3) {
        mode_str = argv[1];
        target   = argv[2];
    } else {
        usage();
    }

    if (parse_mode(mode_str, &op, &bits) < 0)
        usage();

    if (recursive)
        chmod_recursive(op, bits, target);
    else if (chmod_single(op, bits, target) < 0)
        fprintf(2, "chmod: cannot chmod %s\n", target);

    exit(0);
}
