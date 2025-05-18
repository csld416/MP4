# MP4 Part I – Current Progress Recap (Access Control & Symbolic Links, 40 pts)

This document summarizes the completed and remaining tasks for MP4 Part I in xv6.

---

## ✅ Completed & Remaining Tasks – Detailed Breakdown

| #  | Task                                                       | Status             | Notes                                                                    |
| -- | ---------------------------------------------------------- | ------------------ | ------------------------------------------------------------------------ |
| 1  | symln command & sys_symlink()                             | ✅ Done             | Provided by TA; no changes needed unless extending behavior later        |
| 2  | Add `mode` field to inode/stat structures                  | ✅ Done             | You replaced `minor` with `mode` in both `struct inode` and `dinode`     |
| 3  | Initialize mode in fs.c and mkfs.c                         | ✅ Done             | Added `mode = M_ALL` in `ialloc()`, `ilock()`, `iupdate()`, and `mkfs.c` |
| 4  | Modify `ls` to show permission (`rw`, etc.)                | ✅ Done             | You patched `ls.c` to decode and print `st.mode` as a 2-letter string    |
| 5  | Modify `struct stat` and `stati()` to include `mode`       | ✅ Done             | You updated `stat.h` and copied `ip->mode` to `st->mode`                 |
| 6  | Verify that `ls` respects permission logic and format      | ✅ Done             | Confirmed output like `a 2 12 0 rw`                                      |
| 7  | Implement `chmod` syscall in `sysfile.c`                   | ✅ Done             | Updates `ip->mode`, returns 0 or -1                                      |
| 8  | Add user-level `chmod` program (`chmod.c`)                 | ✅ Done             | CLI parsing, syscall invocation, supports `+r`, `-rw`, etc.              |
| 9  | Modify syscall table and `usys.pl` for `chmod`             | ✅ Done             | Registered syscall ID and declared `chmod()` in user space               |
| 10 | Recursive `chmod -R` support                               | ✅ Done             | Walks directory tree and applies permission changes                     |
| 11 | Validate and print errors for bad `chmod` input            | ✅ Done             | Prints: `Usage: chmod [-R] (+|-)r|w|rw file_name|dir_name`               |
| 12 | Modify `open()` to enforce permission                      | ✅ Done             | Enforces read/write based on `ip->mode`; denies unauthorized access      |
| 13 | Support `O_NOACCESS` in `open()`                           | ✅ Done             | Returns `fd` without read/write; does **not** follow symlinks            |
| 14 | Ensure `fstat()` works with `O_NOACCESS` files             | ✅ Done             | Metadata (e.g., size, mode) is still readable                            |
| 15 | Symlink permission behavior (e.g., chmod affects target)   | ✅ Done             | `chmod` on symlink affects the target file/dir                           |
| 16 | `ls` behavior on symlinks (type 4, mode `rw`)              | ✅ Done (partially) | Symlink shows type 4 and `rw`, still resolving content is pending        |
| 17 | `chmod` works even if dir becomes unreadable mid-traversal | ✅ Done             | Handled edge cases using `O_NOACCESS` when walking directory tree        |
| 18 | Follow symlink in `open()` unless `O_NOACCESS`             | ❌ Pending          | Add loop to resolve symlink (up to 10 hops), avoid infinite loop         |
| 19 | Handle `ls` on symlink to file or dir                      | ❌ Pending          | Show metadata for file, or recurse into dir if it has read permission    |
| 20 | Fail `ls` when any path component lacks read permission    | ❌ Pending          | `ls symlink` should fail if parent dirs or target lack `r`               |

---
