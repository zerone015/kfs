#ifndef _EXEC_H
#define _EXEC_H

#define USER_STACK_BASE     0xBFF00000
#define USER_STACK_SIZE     0x100000
#define USER_STACK_TOP      (USER_STACK_BASE + USER_STACK_SIZE - 4)
#define USER_CODE_BASE      PAGE_SIZE
#define USER_CODE_SIZE      PAGE_SIZE

void exec_fn(void (*func)()) __attribute__((noreturn));
void init_process(void) __attribute__((noreturn));

#endif