#ifndef _PROC_H
#define _PROC_H

#include <stdint.h>
#include <stddef.h>
#include "paging.h"
#include "interrupt.h"

extern uint16_t *page_ref;

#define INIT_PROCESS_PID    0

void proc_init(void);
void init_process(void) __attribute__((noreturn));
int fork(struct syscall_frame *sframe);
void exit(int status) __attribute__((noreturn));

#endif