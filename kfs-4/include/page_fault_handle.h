#ifndef _PAGE_FAULT_HANDLE_H
#define _PAGE_FAULT_HANDLE_H

#include <stdint.h>

#define K_NO_PRESENT_MASK 0x5

extern void page_fault_handler(void);
extern void page_fault_handle(uint32_t error_code);

#endif
