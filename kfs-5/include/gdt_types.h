#ifndef _GDT_TYPES_H
#define _GDT_TYPES_H

#include <stdint.h>

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

#endif