#ifndef _PROC_H
#define _PROC_H

#include <stdint.h>
#include <stddef.h>
#include "paging.h"
#include "interrupt.h"

extern uint16_t *page_ref;

void proc_init(void);
void init_process(void) __attribute__((noreturn));
int fork(struct syscall_frame *sframe);

#endif