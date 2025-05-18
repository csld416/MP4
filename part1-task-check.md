# MP4 Part I – Current Progress Recap (Access Control & Symbolic Links, 40 pts)

This document summarizes the completed and remaining tasks for MP4 Part I in xv6.

---

## ✅ Completed & Remaining Tasks – Detailed Breakdown

| #  | Task                                                       | Status             | Notes                                                                    |     |   |               |             |
| -- | ---------------------------------------------------------- | ------------------ | ------------------------------------------------------------------------ | --- | - | ------------- | ----------- |
| 1  | symln command & sys\_symlink()                             | ✅ Done             | Provided by TA; no changes needed unless extending behavior later        |     |   |               |             |
| 2  | Add `mode` field to inode/stat structures                  | ✅ Done             | You replaced `minor` with `mode` in both `struct inode` and `dinode`     |     |   |               |             |
| 3  | Initialize mode in fs.c and mkfs.c                         | ✅ Done             | Added `mode = M_ALL` in `ialloc()`, `ilock()`, `iupdate()`, and `mkfs.c` |     |   |               |             |
| 4  | Modify `ls` to show permission (`rw`, etc.)                | ✅ Done             | You patched `ls.c` to decode and print `st.mode` as a 2-letter string    |     |   |               |             |
| 5  | Modify `struct stat` and `stati()` to include `mode`       | ✅ Done             | You updated `stat.h` and copied `ip->mode` to `st->mode`                 |     |   |               |             |
| 6  | Verify that `ls` respects permission logic and format      | ✅ Done             | Confirmed output like `a 2 12 0 rw`                                      |     |   |               |             |
| 7  | Implement `chmod` syscall in `sysfile.c`                   | ❌ Pending          | You will add a new syscall to update `ip->mode`                          |     |   |               |             |
| 8  | Add user-level `chmod` program (`chmod.c`)                 | ❌ Pending          | Needs to parse CLI args, call your syscall, print usage errors           |     |   |               |             |
| 9  | Modify syscall table and `usys.pl` for `chmod`             | ❌ Pending          | Register syscall ID and declare in user space                            |     |   |               |             |
| 10 | Recursive `chmod -R` support                               | ❌ Pending          | Must walk directory tree and apply permission changes                    |     |   |               |             |
| 11 | Validate and print errors for bad `chmod` input            | ❌ Pending          | Must print: \`Usage: chmod \[-R] (+                                      | -)r | w | rw file\_name | dir\_name\` |
| 12 | Modify `open()` to enforce permission                      | ❌ Pending          | Enforce read/write based on `ip->mode`; deny unauthorized access         |     |   |               |             |
| 13 | Support `O_NOACCESS` in `open()`                           | ❌ Pending          | Return `fd` without read/write; do **not** follow symlink                |     |   |               |             |
| 14 | Ensure `fstat()` works with `O_NOACCESS` files             | ❌ Pending          | Confirm metadata (e.g., size, mode) can still be read                    |     |   |               |             |
| 15 | Symlink permission behavior (e.g., chmod affects target)   | ❌ Pending          | Ensure `chmod` on a symlink changes the target, not the link itself      |     |   |               |             |
| 16 | `ls` behavior on symlinks (type 4, mode `rw`)              | ✅ Done (partially) | `ls` already shows correct type/mode — symlinks always show `rw`         |     |   |               |             |
| 17 | `chmod` works even if dir becomes unreadable mid-traversal | ❌ Pending          | Must handle `-R` edge cases where permission is removed during walk      |     |   |               |             |

---

## ✅ Next Recommended Step

Start Checkpoint 3: Implement `sys_chmod()` in `sysfile.c` and begin `chmod.c`

Let me know if you want:

* System call registration scaffolding
* Command-line parsing templates for `chmod -R +w target`
* Recursive directory traversal logic
