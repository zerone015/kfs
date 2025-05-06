# KFS     

## GOAL

Move beyond abstract understanding by actively designing and implementing every part of the kernel from the ground up.

## DONE

- **TTY**: basic terminal I/O
- **memory management**
  - paging
  - physical memory allocator
  - virtual address space allocator
  - heap memory allocator
- **system call interface**  
  `fork`, `waitpid`, `exit`, `signal`, `kill`, `sigreturn`
- **interrupt handling**
  - exceptions, keyboard, pit
- **scheduling**
  - non-preemptive kernel
  - preemptive multitasking
  - single processor, single core, round-robin
- **signal handling**
  - ISO C standard signal
  - partial POSIX signal
- **kernel panic handling**
  - dump registers + stack and halt

## TO DO
- FPU, MMX, SSE support
- SMP support with multi-core support
- IPC (shared memory, pipe, Unix domain sockets)
- virtual file system (VFS) and actual file system
- swap space management, page replacement policy, and `swapd`
- full system call implementation
- dynamic kernel module loading
- ELF parser and loader
- port musl libc
- port POSIX shell and DOOM
- user-space multithreading support
- page coloring support
- improve heap allocator (replace with slab allocator)
- improve scheduler to CFS (Completely Fair Scheduler)
- port to 64-bit architecture (x86_64) with structural extension
