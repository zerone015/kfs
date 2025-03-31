#ifndef _PANIC_H
#define _PANIC_H

#include <stdint.h>
#include "interrupt.h"

extern char stack_top;

extern void panic(const char *msg, struct interrupt_frame *iframe);
extern void panic_trigger(const char *msg);

#endif