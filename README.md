# KFS

An x86 kernel written from scratch, based on the Kernel From Scratch project at 42 School.

## Documentation (Korean)

- [0. Introduction](https://zerone015.github.io/posts/kfs-0-intro/)
- [1. Boot](https://zerone015.github.io/posts/kfs-1-boot/)
- [2. Physical Memory](https://zerone015.github.io/posts/kfs-2-pm/)
- [3. Virtual Address Space](https://zerone015.github.io/posts/kfs-3-vas/)
- [4. Heap](https://zerone015.github.io/posts/kfs-4-heap/)
- [5. Interrupts and System Calls](https://zerone015.github.io/posts/kfs-5-interrupts-and-syscalls/)
- [6. Scheduling](https://zerone015.github.io/posts/kfs-6-scheduling/)
- [7. Process](https://zerone015.github.io/posts/kfs-7-process/)
- [8. Signal](https://zerone015.github.io/posts/kfs-8-signal/)
- [9. Testing](https://zerone015.github.io/posts/kfs-9-test/)

## Installation

```bash
sudo apt install build-essential bison flex texinfo wget xz-utils nasm qemu-system-x86 gcc-multilib
./scripts/toolchain.sh  # builds the i686-elf cross compiler into ./toolchain
```

## Build and Run

```bash
make
qemu-system-i386 -kernel kernel.elf -m <RAM_SIZE>   # set the RAM size you want, such as 512M
```

## Test

```bash
./scripts/test.sh
```