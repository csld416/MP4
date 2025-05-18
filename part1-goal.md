# MP4 Part I: Access Control & Symbolic Links (40 pts)

This document outlines the **development stages**, **tasks**, and **checkpoints** for implementing Part I of the MP4 assignment in xv6.

---

## ✅ Overview

You need to implement:

* Access control via `chmod` syscall and command
* Permission-enforcing `open()`
* Enhanced `ls` output
* Symlink behavior integration (partially done)

---

## 🛠️ Development Stages & Checkpoints

### ✅ Checkpoint 0 – Baseline

* Confirm tests fail due to missing features
* Run: `make grade`
* Expected: 0/40 score
* Git:

  ```bash
  git checkout -b mp4-baseline
  ```

---

### ✅ Checkpoint 1 – Add `mode` Field

* Add `mode` to `struct inode` (reuse `minor` or another field)
* Initialize default mode (rw) in `ialloc()` and `mkfs.c`
* Git:

  ```bash
  git checkout -b add-mode-field
  ```

---

### ✅ Checkpoint 2 – Modify `ls`

* Modify `user/ls.c` to display `mode` field
* Print `rw`, `r-`, `-w`, or `--` at end of `ls` output
* Git:

  ```bash
  git checkout -b ls-shows-mode
  ```

---

### ✅ Checkpoint 3 – Implement `sys_chmod` + `chmod` Command

* Implement `sys_chmod()` in `sysfile.c`
* Register syscall in `syscall.c`, `syscall.h`, `usys.pl`
* Write `user/chmod.c`
* Handle parsing: `chmod [-R] (+|-)r|w|rw filename`
* Git:

  ```bash
  git checkout -b chmod-works
  ```

---

### ✅ Checkpoint 4 – Enforce Permissions in `open()`

* Modify `open()` to check against `inode->mode`
* Support `O_NOACCESS`: return fd, but disallow read/write, and don’t follow symlink
* Git:

  ```bash
  git checkout -b enforce-open-mode
  ```

---

### ✅ Checkpoint 5 – Symlink Behavior

* `ls x`: show symlink metadata if x is a symlink
* `ls x` (where x → dir): show contents of target dir
* `open()` follows symlink unless `O_NOACCESS`
* Git:

  ```bash
  git checkout -b symlink-behavior
  ```

---

### ✅ Checkpoint 6 – Recursive `chmod -R` Edge Cases

* Implement traversal that handles:

  * Removing read permission during walk
  * Starting traversal even if dir isn’t readable
* Git:

  ```bash
  git checkout -b chmod-recursive-robust
  ```

---

## 📦 Final Test

Run:

```bash
make grade
```

Expect full score if everything is correctly implemented.

---

## ✅ Git Strategy Summary

Use branches as named checkpoints:

```bash
git checkout -b mp4-baseline       # clean start
git checkout -b chmod-works        # after chmod done
git checkout -b open-permission    # after open() secured
```

Each branch = one recoverable development stage.

---

Let me know if you want test logs, template commits, or `.gitignore` for xv6.
