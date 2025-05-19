# 🧪 MP4 Test Case Summary

---

## ✅ Part I: Access Control & Symbolic Links (40 pts)

### 📄 Test Case 0: Basic chmod, ls, symlink behavior

**Commands:**

```sh
$ gen 0
$ ls test1
$ ls test1/a
$ chmod -rw test1/a
$ chmod -rw test1/b
$ ls test1/d
$ chmod -r test1
$ ls test1
$ mp4_1 0
```

**Expected Output:**

```
$ gen 0
$ ls test1
.              1 22 96 rw
..             1 1 1024 rw
a              2 23 3 rw
b              2 24 3 rw
c              2 25 3 rw
d              1 26 48 rw
$ ls test1/a
a              2 23 3 rw
$ chmod -rw test1/a
$ chmod -rw test1/b
$ ls test1/d
.              1 26 48 rw
..             1 22 96 rw
a              2 27 3 rw
$ chmod -r test1
$ ls test1
ls: cannot open test1
$ mp4_1 0
open test1 failed
open test1/a failed
open test1/d failed
```

---

### 📄 Test Case 1: Invalid chmod, symlink collision, partial read permissions

**Commands:**

```sh
$ gen 1
$ chmod +-rw test2/d1/f1
$ chmod -R +w test2/d100
$ chmod -R -r test2/d2
$ ls test2
$ ls test2/d2
$ ls test2/d1/f1
$ symln test2 test2_fake
$ symln test1 test2_fake
$ ls test2_fake
$ ls test2_fake/d2
$ ls test2_fake/d100
$ mp4_1 1
```

**Expected Output:**

```sh
$ gen 1
$ chmod +-rw test2/d1/f1
Usage: chmod [-R] (+|-)(r|w|rw|wr) file_name|dir_name
$ chmod -R +w test2/d100
chmod: cannot chmod test2/d100
$ chmod -R -r test2/d2
$ ls test2
.              1 23 80 rw
..             1 1 1024 rw
d1             1 24 48 rw
d2             1 25 48 -w
d3             1 26 48 rw
$ ls test2/d2
ls: cannot open test2/d2
$ ls test2/d1/f1
f1             2 27 3 rw
$ symln test2 test2_fake
$ symln test1 test2_fake
symlink test1 test2_fake: failed
$ ls test2_fake
.              1 23 80 rw
..             1 1 1024 rw
d1             1 24 48 rw
d2             1 25 48 -w
d3             1 26 48 rw
$ ls test2_fake/d2
ls: cannot open test2_fake/d2
$ ls test2_fake/d100
ls: cannot open test2_fake/d100
$ mp4_1 1
open test2_fake/d2/f2 failed
read test2_fake/d1/f1 failed
type of test2_fake/d1/f1 is 2
```

---

### 📄 Test Case 2: Deep symbolic links & recursive chmod

**Commands:**

```sh
$ gen 2
$ symln test3/d1 test3/d1ln_1
$ symln test3/d1ln_1 test3/d1ln_2
$ symln test3/d1ln_2 test3/d1ln_3
$ symln test3/d1ln_3 test3/d1ln_4
$ chmod -R +rw test3/d1ln_4
$ chmod -R -rw test3/d1ln_4
$ chmod +r test3/d1/D/F
$ symln test3/d1/D/F test3/Fln
$ ls test3/Fln
$ chmod +r test3/d1
$ ls test3
$ ls test3/d1ln_4
$ mp4_1 2
```

**Expected Output 2(excerpt):**

```sh
$ gen 2
$ symln test3/d1 test3/d1ln_1
$ symln test3/d1ln_1 test3/d1ln_2
$ symln test3/d1ln_2 test3/d1ln_3
$ symln test3/d1ln_3 test3/d1ln_4
$ chmod -R +rw test3/d1ln_4
$ chmod -R -rw test3/d1ln_4
$ chmod +r test3/d1/D/F
chmod: cannot chmod test3/d1/D/F
$ symln test3/d1/D/F test3/Fln
$ ls test3/Fln
ls: cannot open test3/Fln
$ chmod +r test3/d1
$ ls test3
.              1 23 128 rw
..             1 1 1024 rw
d1             1 24 96 r-
d1ln_1         4 30 13 rw
d1ln_2         4 31 17 rw
d1ln_3         4 32 17 rw
d1ln_4         4 33 17 rw
Fln            4 34 17 rw
$ ls test3/d1ln_4
.              1 24 96 r-
..             1 23 128 rw
D              1 25 48 --
f1             2 26 3 --
f2             2 27 3 --
f3             2 28 3 --
$ mp4_1 2
type of test3/d1ln_4 is 4
type of the target file test3/d1ln_4 pointing to is 1
open test3/Fln (1) failed
type of test3/Fln is 4
```

---

### 📄 Test Case 3: Symmetric symlinks, partial permission recovery

**Commands:**

```sh
$ gen 3
$ symln test4/dirX test4/dirXln1
$ symln test4/dirX test4/dirXln2
$ symln test4/dirX/a test4/aln1
$ chmod -R -rw test4/dirXln1
$ chmod -R -rw test4/dirXln2
$ symln test4/dirX/a test4/aln2
$ ls test4/aln1
$ ls test4/aln2
$ ls test4
$ ls test4/dirX
$ ls test4/dirXln1/c
$ ls test4/dirXln2
$ chmod -R +rw test4/dirX
$ ls test4
$ ls test4/aln1
$ ls test4/dirXln1
$ mp4_1 3
```

**Expected Output 3(excerpt):**

```sh
$ gen 3
$ symln test4/dirX test4/dirXln1
$ symln test4/dirX test4/dirXln2
$ symln test4/dirX/a test4/aln1
$ chmod -R -rw test4/dirXln1
$ chmod -R -rw test4/dirXln2
chmod: cannot chmod test4/dirXln2
$ symln test4/dirX/a test4/aln2
$ ls test4/aln1
ls: cannot open test4/aln1
$ ls test4/aln2
ls: cannot open test4/aln2
$ ls test4
.              1 23 112 rw
..             1 1 1024 rw
dirX           1 24 80 --
dirXln1        4 28 15 rw
dirXln2        4 29 15 rw
aln1           4 30 17 rw
aln2           4 31 17 rw
$ ls test4/dirX
ls: cannot open test4/dirX
$ ls test4/dirXln1/c
ls: cannot open test4/dirXln1/c
$ ls test4/dirXln2
ls: cannot open test4/dirXln2
$ chmod -R +rw test4/dirX
$ ls test4
.              1 23 112 rw
..             1 1 1024 rw
dirX           1 24 80 rw
dirXln1        4 28 15 rw
dirXln2        4 29 15 rw
aln1           4 30 17 rw
aln2           4 31 17 rw
$ ls test4/aln1
aln1           4 30 17 rw
$ ls test4/dirXln1
.              1 24 80 rw
..             1 23 112 rw
a              2 25 3 rw
b              2 26 3 rw
c              2 27 3 rw
$ mp4_1 3
hello os2025
```

---

## ✅ Part II: RAID 1 Simulation (60 pts)

### 💽 Test Case: Normal Write

**Commands:**

```sh
$ echo .
$ mp4_2_write_failure_test 0
```

**Expected Messages:**

* BW\_DIAG: correct mapping of PBN0 and PBN1
* Normal write succeeds to both disks

---

### 💽 Test Case: Simulated Disk 0 Failure

**Commands:**

```sh
$ echo .
$ mp4_2_write_failure_test 1
```

**Expected Messages:**

* BW\_ACTION: SKIP\_PBN0 due to simulated Disk 0 failure
* BW\_ACTION: ATTEMPT\_PBN1

---

### 💽 Test Case: RAID1 Mirroring Verification

**Commands:**

```sh
$ echo .
$ mp4_2_mirror_test
```

**Expected Output:**

* "Bwrite Mirroring Test: PASS"

---

### 💽 Test Case: Bread Read Fallback (Disk Failure)

**Commands:**

```sh
$ echo .
$ mp4_2_disk_failure_test
```

**Expected Output:**

* "Bread Disk Failure Fallback Test: PASS"
