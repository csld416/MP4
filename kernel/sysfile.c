//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//
// 1
#include "types.h"
// 2
#include "riscv.h"
// 3
#include "defs.h"
// 4
#include "param.h"
// 5
#include "stat.h"
// 6
#include "spinlock.h"
// 7
#include "proc.h"
// 8
#include "fs.h"
// 9
#include "sleeplock.h"
//
#include "file.h"
//
#include "fcntl.h"
//
#include "buf.h"

#define MAXPATH 128

struct inode *namex(char *path, int nameiparent, char *name);

static uint64 sys_open_internal(char *path, int omode);

// Fetch the nth word-sized system call argument as a file descriptor
// and return both the descriptor and the corresponding struct file.
static int argfd(int n, int *pfd, struct file **pf)
{
    int fd;
    struct file *f;

    if (argint(n, &fd) < 0)
        return -1;
    if (fd < 0 || fd >= NOFILE || (f = myproc()->ofile[fd]) == 0)
        return -1;
    if (pfd)
        *pfd = fd;
    if (pf)
        *pf = f;
    return 0;
}

// Allocate a file descriptor for the given file.
// Takes over file reference from caller on success.
static int fdalloc(struct file *f)
{
    int fd;
    struct proc *p = myproc();

    for (fd = 0; fd < NOFILE; fd++)
    {
        if (p->ofile[fd] == 0)
        {
            p->ofile[fd] = f;
            return fd;
        }
    }
    return -1;
}

uint64 sys_dup(void)
{
    struct file *f;
    int fd;

    if (argfd(0, 0, &f) < 0)
        return -1;
    if ((fd = fdalloc(f)) < 0)
        return -1;
    filedup(f);
    return fd;
}

uint64 sys_read(void)
{
    struct file *f;
    int n;
    uint64 p;

    if (argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argaddr(1, &p) < 0)
        return -1;
    return fileread(f, p, n);
}

uint64 sys_write(void)
{
    struct file *f;
    int n;
    uint64 p;

    if (argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argaddr(1, &p) < 0)
        return -1;

    return filewrite(f, p, n);
}

uint64 sys_close(void)
{
    int fd;
    struct file *f;

    if (argfd(0, &fd, &f) < 0)
        return -1;
    myproc()->ofile[fd] = 0;
    fileclose(f);
    return 0;
}

uint64 sys_fstat(void)
{
    struct file *f;
    uint64 st; // user pointer to struct stat

    if (argfd(0, 0, &f) < 0 || argaddr(1, &st) < 0)
        return -1;
    return filestat(f, st);
}

// Create the path new as a link to the same inode as old.
uint64 sys_link(void)
{
    char name[DIRSIZ], new[MAXPATH], old[MAXPATH];
    struct inode *dp, *ip;

    if (argstr(0, old, MAXPATH) < 0 || argstr(1, new, MAXPATH) < 0)
        return -1;

    begin_op();
    if ((ip = namei(old)) == 0)
    {
        end_op();
        return -1;
    }

    ilock(ip);
    if (ip->type == T_DIR)
    {
        iunlockput(ip);
        end_op();
        return -1;
    }

    ip->nlink++;
    iupdate(ip);
    iunlock(ip);

    if ((dp = nameiparent(new, name)) == 0)
        goto bad;
    ilock(dp);
    if (dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0)
    {
        iunlockput(dp);
        goto bad;
    }
    iunlockput(dp);
    iput(ip);

    end_op();

    return 0;

bad:
    ilock(ip);
    ip->nlink--;
    iupdate(ip);
    iunlockput(ip);
    end_op();
    return -1;
}

// Is the directory dp empty except for "." and ".." ?
static int isdirempty(struct inode *dp)
{
    int off;
    struct dirent de;

    for (off = 2 * sizeof(de); off < dp->size; off += sizeof(de))
    {
        if (readi(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
            panic("isdirempty: readi");
        if (de.inum != 0)
            return 0;
    }
    return 1;
}

uint64 sys_unlink(void)
{
    struct inode *ip, *dp;
    struct dirent de;
    char name[DIRSIZ], path[MAXPATH];
    uint off;

    if (argstr(0, path, MAXPATH) < 0)
        return -1;

    begin_op();
    if ((dp = nameiparent(path, name)) == 0)
    {
        end_op();
        return -1;
    }

    ilock(dp);

    // Cannot unlink "." or "..".
    if (namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
        goto bad;

    if ((ip = dirlookup(dp, name, &off)) == 0)
        goto bad;
    ilock(ip);

    if (ip->nlink < 1)
        panic("unlink: nlink < 1");
    if (ip->type == T_DIR && !isdirempty(ip))
    {
        iunlockput(ip);
        goto bad;
    }

    memset(&de, 0, sizeof(de));
    if (writei(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
        panic("unlink: writei");
    if (ip->type == T_DIR)
    {
        dp->nlink--;
        iupdate(dp);
    }
    iunlockput(dp);

    ip->nlink--;
    iupdate(ip);
    iunlockput(ip);

    end_op();

    return 0;

bad:
    iunlockput(dp);
    end_op();
    return -1;
}

static struct inode *create(char *path, short type, short major, short mode)
{
    struct inode *ip, *dp;
    char name[DIRSIZ];

    if ((dp = nameiparent(path, name)) == 0)
        return 0;

    ilock(dp);

    if ((ip = dirlookup(dp, name, 0)) != 0)
    {
        iunlockput(dp);
        ilock(ip);
        if (type == T_FILE && (ip->type == T_FILE || ip->type == T_DEVICE))
            return ip;
        iunlockput(ip);
        return 0;
    }

    if ((ip = ialloc(dp->dev, type)) == 0)
        panic("create: ialloc");

    ilock(ip);
    ip->major = major;
    ip->mode = mode;
    ip->nlink = 1;
    iupdate(ip);

    if (type == T_DIR)
    {                // Create . and .. entries.
        dp->nlink++; // for ".."
        iupdate(dp);
        // No ip->nlink++ for ".": avoid cyclic ref count.
        if (dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
            panic("create dots");
    }

    if (dirlink(dp, name, ip->inum) < 0)
        panic("create: dirlink");

    iunlockput(dp);

    return ip;
}

/* TODO: Access Control & Symbolic Link */
uint64 sys_open(void)
{
    char path[MAXPATH];
    int omode;
    if (argstr(0, path, MAXPATH) < 0 || argint(1, &omode) < 0)
        return -1;
    return sys_open_internal(path, omode);
}

static uint64 sys_open_internal(char *path, int omode)
{
    struct inode *ip;
    struct file *f;
    int fd;

    begin_op();

    // ───────────────── 2) Regular open (no create) ─────────────────
    if (omode & O_CREATE)
    {
        // normal xv6 create path
        ip = create(path, T_FILE, 0, M_ALL);
        if (ip == 0)
        {
            end_op();
            return -1;
        }
    }
    // ───────────────── 2) Regular open (no create) ─────────────────
    else
    {
        // ─── 2a) O_NOACCESS: metadata‐only open ───
        if (omode & O_NOACCESS)
        {

            char parentPath[MAXPATH], leafTarget[DIRSIZ];

            // 1) find last slash in path[]
            int i = 0;
            for (; path[i]; i++) /* find length */
                ;
            char *slash = 0;
            for (int j = i - 1; j >= 0; j--)
            {
                if (path[j] == '/')
                {
                    slash = &path[j];
                    break;
                }
            }

            // 2) split into 'parent' and 'leaf'
            if (slash)
            {
                int plen = slash - path;
                if (plen >= sizeof(parentPath))
                    plen = sizeof(parentPath) - 1;
                memmove(parentPath, path, plen);
                parentPath[plen] = '\0';

                int llen = strlen(slash + 1);
                if (llen >= DIRSIZ)
                    llen = DIRSIZ - 1;
                memmove(leafTarget, slash + 1, llen);
                leafTarget[llen] = '\0';
            }
            else
            {
                safestrcpy(parentPath, ".", sizeof(parentPath));
                safestrcpy(leafTarget, path, sizeof(leafTarget));
            }

            // 3) look up the parent directory, *without* following leaf
            ip = namex(path, 1, leafTarget);
            if (!ip)
            {
                end_op();
                return -1;
            }
            ilock(ip);

            // 4) if parent itself is a symlink, chase it exactly once
            while (ip->type == T_SYMLINK)
            {
                char buf[MAXPATH];
                int n = readi(ip, 0, (uint64)buf, 0, MAXPATH - 1);
                iput(ip);
                end_op();
                if (n < 0)
                    return -1;
                buf[n] = '\0';
                begin_op();
                ip = namei(buf);
                if (ip == 0)
                {
                    end_op();
                    return -1;
                }
                ilock(ip);
                end_op();
            }

            // 5) must be a directory now
            if (ip->type != T_DIR)
            {
                iunlockput(ip);
                end_op();
                return -1;
            }
            struct inode *dp = ip;

            // 6) lookup the leaf entry (no symlink following here)
            ip = dirlookup(dp, leafTarget, 0);
            iunlockput(dp);
            if (!ip)
            {
                end_op();
                return -1;
            }
            //7) lock final node
            ilock(ip);
        }
        // ─── 2b) Normal open: follow final symlinks ───
        else
        {
            ip = namei(path); // follow symlinks
            if (ip == 0)
            {
                end_op();
                return -1;
            }
            ilock(ip);
            // if it turned out to be a symlink, read its target and recurse:
            while (ip->type == T_SYMLINK)
            {
                // read the link’s target string out of its data block
                char target[MAXPATH];
                int n = readi(ip, 0, (uint64)target, 0, MAXPATH - 1);
                iunlockput(ip);
                end_op();
                if (n < 0)
                    return -1;
                target[n] = '\0';
                // recurse: open the target path instead
                return sys_open_internal(target, omode);
            }
        }

        // ─── 2c) Permission checks (unless O_NOACCESS) ───
        if (!(omode & O_NOACCESS))
        {
            int want_read = !(omode & O_WRONLY);
            int want_write = (omode & O_WRONLY) || (omode & O_RDWR);
            int pass = !(want_read && !(ip->mode & M_READ)) &&
                       !(want_write && !(ip->mode & M_WRITE)) &&
                       !(ip->type == T_DIR && want_write);
            if (!pass)
            {
                iunlockput(ip);
                end_op();
                return -1;
            }
        }
    }

    // ─── 3) Special device‐inode check ───
    if (ip->type == T_DEVICE && (ip->major < 0 || ip->major >= NDEV))
    {
        iunlockput(ip);
        end_op();
        return -1;
    }

    // ─── 4) Allocate the in‐kernel file struct and a file descriptor ───
    if ((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0)
    {
        if (f)
            fileclose(f);
        iunlockput(ip);
        end_op();
        return -1;
    }

    // ─── 5) Initialize file struct and return ───
    f->type = (ip->type == T_DEVICE ? FD_DEVICE : FD_INODE);
    f->major = ip->major;
    f->ip = ip;
    f->off = 0;
    if ((omode & O_NOACCESS) && ip->type != T_SYMLINK)
    {
        f->readable = 0;
        f->writable = 0;
    }
    else
    {
        f->readable = !(omode & O_WRONLY);
        f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
    }
    if ((omode & O_TRUNC) && ip->type == T_FILE)
        itrunc(ip);

    iunlock(ip);
    end_op(); // end the transaction
    return fd;
}

uint64 sys_mkdir(void)
{
    char path[MAXPATH];
    struct inode *ip;

    begin_op();
    if (argstr(0, path, MAXPATH) < 0 ||
        (ip = create(path, T_DIR, 0, M_ALL)) == 0)
    {
        end_op();
        return -1;
    }
    iunlockput(ip);
    end_op();
    return 0;
}

uint64 sys_mknod(void)
{
    struct inode *ip;
    char path[MAXPATH];
    int major, minor;

    begin_op();
    if ((argstr(0, path, MAXPATH)) < 0 || argint(1, &major) < 0 ||
        argint(2, &minor) < 0 ||
        (ip = create(path, T_DEVICE, major, M_ALL)) == 0)
    {
        end_op();
        return -1;
    }
    iunlockput(ip);
    end_op();
    return 0;
}

uint64 sys_chdir(void)
{
    char path[MAXPATH];
    struct inode *ip;
    struct proc *p = myproc();

    begin_op();
    if (argstr(0, path, MAXPATH) < 0 || (ip = namei(path)) == 0)
    {
        end_op();
        return -1;
    }
    ilock(ip);
    if (ip->type != T_DIR)
    {
        iunlockput(ip);
        end_op();
        return -1;
    }
    iunlock(ip);
    iput(p->cwd);
    end_op();
    p->cwd = ip;
    return 0;
}

uint64 sys_exec(void)
{
    char path[MAXPATH], *argv[MAXARG];
    int i;
    uint64 uargv, uarg;

    if (argstr(0, path, MAXPATH) < 0 || argaddr(1, &uargv) < 0)
    {
        return -1;
    }
    memset(argv, 0, sizeof(argv));
    for (i = 0;; i++)
    {
        if (i >= NELEM(argv))
        {
            goto bad;
        }
        if (fetchaddr(uargv + sizeof(uint64) * i, (uint64 *)&uarg) < 0)
        {
            goto bad;
        }
        if (uarg == 0)
        {
            argv[i] = 0;
            break;
        }
        argv[i] = kalloc();
        if (argv[i] == 0)
            goto bad;
        if (fetchstr(uarg, argv[i], PGSIZE) < 0)
            goto bad;
    }

    int ret = exec(path, argv);

    for (i = 0; i < NELEM(argv) && argv[i] != 0; i++)
        kfree(argv[i]);

    return ret;

bad:
    for (i = 0; i < NELEM(argv) && argv[i] != 0; i++)
        kfree(argv[i]);
    return -1;
}

uint64 sys_pipe(void)
{
    uint64 fdarray; // user pointer to array of two integers
    struct file *rf, *wf;
    int fd0, fd1;
    struct proc *p = myproc();

    if (argaddr(0, &fdarray) < 0)
        return -1;
    if (pipealloc(&rf, &wf) < 0)
        return -1;
    fd0 = -1;
    if ((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0)
    {
        if (fd0 >= 0)
            p->ofile[fd0] = 0;
        fileclose(rf);
        fileclose(wf);
        return -1;
    }
    if (copyout(p->pagetable, fdarray, (char *)&fd0, sizeof(fd0)) < 0 ||
        copyout(p->pagetable, fdarray + sizeof(fd0), (char *)&fd1,
                sizeof(fd1)) < 0)
    {
        p->ofile[fd0] = 0;
        p->ofile[fd1] = 0;
        fileclose(rf);
        fileclose(wf);
        return -1;
    }
    return 0;
}

/* TODO: Access Control & Symbolic Link */
uint64 sys_chmod(void)
{
    char path[128];
    int mode_op;

    // args: mode_op (int), path (string)
    if (argint(0, &mode_op) < 0 || argstr(1, path, sizeof(path)) < 0)
        return -1;

    begin_op();
    struct inode *ip = namei(path);
    if (ip == 0)
    {
        end_op();
        return -1;
    }

    ilock(ip);
    ip->mode = mode_op;
    iupdate(ip);
    iunlock(ip);
    iput(ip);
    end_op();
    return 0;
}

/* TODO: Access Control & Symbolic Link */
uint64 sys_symlink(void)
{
    char target[MAXPATH], path[MAXPATH];
    struct inode *ip;

    // Get syscall arguments: target and link name
    if (argstr(0, target, MAXPATH) < 0 || argstr(1, path, MAXPATH) < 0)
    {
        return -1;
    }

    begin_op();

    // Create a new inode of type T_SYMLINK
    ip = create(path, T_SYMLINK, 0, M_ALL);
    if (ip == 0)
    {
        end_op();
        return -1;
    }

    // Write the target path into the symlink's data block
    if (writei(ip, 0, (uint64)target, 0, strlen(target)) != strlen(target))
    {
        iunlockput(ip);
        end_op();
        return -1;
    }

    iunlockput(ip);
    end_op();
    return 0;
}

uint64 sys_raw_read(void)
{
    int pbn;
    uint64 user_buf_addr;
    struct buf *b;

    if (argint(0, &pbn) < 0 || argaddr(1, &user_buf_addr) < 0)
    {
        return -1;
    }

    if (pbn < 0 || pbn >= FSSIZE)
    {
        return -1;
    }

    b = bget(ROOTDEV, pbn);
    if (b == 0)
    {
        return -1;
    }

    virtio_disk_rw(b, 0);

    struct proc *p = myproc();
    if (copyout(p->pagetable, user_buf_addr, (char *)b->data, BSIZE) < 0)
    {
        brelse(b);
        return -1;
    }

    brelse(b);
    return 0;
}

uint64 sys_get_disk_lbn(void)
{
    struct file *f;
    int fd;
    int file_lbn;
    uint disk_lbn;

    if (argfd(0, &fd, &f) < 0 || argint(1, &file_lbn) < 0)
    {
        return -1;
    }

    if (!f->readable)
    {
        return -1;
    }

    struct inode *ip = f->ip;

    ilock(ip);

    disk_lbn = bmap(ip, file_lbn);

    iunlock(ip);

    return (uint64)disk_lbn;
}

uint64 sys_raw_write(void)
{
    int pbn;
    uint64 user_buf_addr;
    struct buf *b;

    if (argint(0, &pbn) < 0 || argaddr(1, &user_buf_addr) < 0)
    {
        return -1;
    }

    if (pbn < 0 || pbn >= FSSIZE)
    {
        return -1;
    }

    b = bget(ROOTDEV, pbn);
    if (b == 0)
    {
        printf("sys_raw_write: bget failed for PBN %d\n", pbn);
        return -1;
    }
    struct proc *p = myproc();
    if (copyin(p->pagetable, (char *)b->data, user_buf_addr, BSIZE) < 0)
    {
        brelse(b);
        return -1;
    }

    b->valid = 1;
    virtio_disk_rw(b, 1);
    brelse(b);

    return 0;
}
