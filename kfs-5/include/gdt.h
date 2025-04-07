#ifndef _GDT_H
#define _GDT_H

#include <stdint.h>
#include <stddef.h>

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
#define GDT_SELECTOR_CODE_PL3   0x18
#define GDT_SELECTOR_DATA_PL3   0x20
#define GDT_SELECTOR_TSS        0x28

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t limit_high :4;
    uint8_t flags :4;
    uint8_t base_high;
} __attribute__((packed, aligned(8)));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct tss {
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp, esi, edi;
    uint16_t es, _res1;
    uint16_t cs, _res2;
    uint16_t ss, _res3;
    uint16_t ds, _res4;
    uint16_t fs, _res5;
    uint16_t gs, _res6;
    uint16_t ldtr, _res7;
    uint16_t iomap;
} __attribute__((packed));

extern void gdt_init(void);
extern void tss_init(void);

#endif
