#ifndef _EXEC_H
#define _EXEC_H

#define USER_STACK_BASE         0xBFF00000
#define USER_STACK_SIZE         0x100000
#define USER_STACK_GUARD_BASE   (0xBFF00000 - PAGE_SIZE)
#define USER_STACK_GUARD_SIZE   PAGE_SIZE
#define USER_STACK_TOP          (USER_STACK_BASE + USER_STACK_SIZE)
#define USER_CODE_BASE          PAGE_SIZE
#define USER_CODE_SIZE          (PAGE_SIZE * 2)
#define TEST_SIGNAL_BASE        (USER_CODE_BASE + USER_CODE_SIZE)
#define TEST_SIGNAL_SIZE        (PAGE_SIZE)

#define code_section(addr)   (((addr) >= USER_CODE_BASE) && ((addr) < (USER_CODE_BASE + USER_CODE_SIZE)))

void exec_fn(void (*func)());

#endif