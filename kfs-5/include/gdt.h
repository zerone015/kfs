#ifndef _GDT_H
#define _GDT_H

#include <stdint.h>
#include <stddef.h>
#include "gdt_types.h"

#define GDT_SIZE 6

#define SEG_DESCTYPE(x)  ((x) << 0x04) // Descriptor type (0 for system, 1 for code/data)
#define SEG_PRES(x)      ((x) << 0x07) // Present
#define SEG_PRIV(x)     (((x) &  0x03) << 0x05)   // Set privilege level (0 - 3)

#define SEG_DATA_RD        0x00 // Read-Only
#define SEG_DATA_RDA       0x01 // Read-Only, accessed
#define SEG_DATA_RDWR      0x02 // Read/Write
#define SEG_DATA_RDWRA     0x03 // Read/Write, accessed
#define SEG_DATA_RDEXPD    0x04 // Read-Only, expand-down
#define SEG_DATA_RDEXPDA   0x05 // Read-Only, expand-down, accessed
#define SEG_DATA_RDWREXPD  0x06 // Read/Write, expand-down
#define SEG_DATA_RDWREXPDA 0x07 // Read/Write, expand-down, accessed
#define SEG_CODE_EX        0x08 // Execute-Only
#define SEG_CODE_EXA       0x09 // Execute-Only, accessed
#define SEG_CODE_EXRD      0x0A // Execute/Read
#define SEG_CODE_EXRDA     0x0B // Execute/Read, accessed
#define SEG_CODE_EXC       0x0C // Execute-Only, conforming
#define SEG_CODE_EXCA      0x0D // Execute-Only, conforming, accessed
#define SEG_CODE_EXRDC     0x0E // Execute/Read, conforming
#define SEG_CODE_EXRDCA    0x0F // Execute/Read, conforming, accessed
 
#define GDT_CODE_PL0 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_PRIV(0) | SEG_CODE_EXRD
#define GDT_DATA_PL0 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_PRIV(0) | SEG_DATA_RDWR
#define GDT_CODE_PL3 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_PRIV(3) | SEG_CODE_EXRD
#define GDT_DATA_PL3 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_PRIV(3) | SEG_DATA_RDWR

#define GDT_FLAGS               0xC
#define GDT_TSS_ACCESS          0x89
#define GDT_TSS_FLAGS           0x04

#define GDT_SELECTOR_CODE_PL0   0x08
#define GDT_SELECTOR_DATA_PL0   0x10
#define GDT_SELECTOR_CODE_PL3   0x1B
#define GDT_SELECTOR_DATA_PL3   0x23
#define GDT_SELECTOR_TSS        0x28

extern char stack_top;
extern struct gdt_entry gdt[GDT_SIZE];

void gdt_init(void);
void tss_init(void);

static inline void tss_esp0_update(uint32_t esp0)
{
    struct tss *tss;

    tss = (struct tss *)((gdt[5].base_high << 24) | (gdt[5].base_mid << 16) | gdt[5].base_low);
    tss->esp0 = esp0;
}

#endif
