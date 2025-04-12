#include "interrupt.h"
#include "tty.h"
#include "pic.h"
#include "io.h"
#include "printk.h"
#include "pmm.h"
#include "paging.h"
#include "panic.h"
#include "sched.h"
#include "proc.h"

/* This is valid only for US QWERTY keyboards. */
static const char key_map[128] =
{
    0,  0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',   
  '\t', /* <-- Tab */
  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',     
    0, /* <-- control key */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',  0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
  '*',
    0,  /* Alt */
  ' ',  /* Space bar */
    0,  /* Caps lock */
    0,  /* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,  /* < ... F10 */
    0,  /* 69 - Num lock*/
    0,  /* Scroll Lock */
    0,  /* Home key */
    0,  /* Up Arrow */
    0,  /* Page Up */
  '-',
    0,  /* Left Arrow */
    0,
    0,  /* Right Arrow */
  '+',
    0,  /* 79 - End key*/
    0,  /* Down Arrow */
    0,  /* Page Down */
    0,  /* Insert Key */
    0,  /* Delete Key */
    0,   0,   0,
    0,  /* F11 Key */
    0,  /* F12 Key */
    0,  /* All other keys are undefined */
};

/* This is valid only for US QWERTY keyboards. */
static const char shift_key_map[128] =
{
    0,  0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
  '\t', /* <-- Tab */
  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, /* <-- control key */
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~',  0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
  '*',
    0,  /* Alt */
  ' ',  /* Space bar */
    0,  /* Caps lock */
    0,  /* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,  /* < ... F10 */
    0,  /* 69 - Num lock*/
    0,  /* Scroll Lock */
    0,  /* Home key */
    0,  /* Up Arrow */
    0,  /* Page Up */
  '-',
    0,  /* Left Arrow */
    0,
    0,  /* Right Arrow */
  '+',
    0,  /* 79 - End key*/
    0,  /* Down Arrow */
    0,  /* Page Down */
    0,  /* Insert Key */
    0,  /* Delete Key */
    0,   0,   0,
    0,  /* F11 Key */
    0,  /* F12 Key */
    0,  /* All other keys are undefined */
};

static __attribute__((noreturn)) void panic_handle(const char *msg, struct interrupt_frame *iframe) 
{
    struct panic_info panic_info;

    panic_info.eax = iframe->eax;
    panic_info.ebp = iframe->ebp;
    panic_info.ebx = iframe->ebx;
    panic_info.ecx = iframe->ecx;
    panic_info.edi = iframe->edi;
    panic_info.edx = iframe->edx;
    panic_info.eflags = iframe->eflags;
    panic_info.eip = iframe->eip;
    panic_info.esi = iframe->esi;
    if (panic_is_user(iframe->cs))
        panic_info.esp = iframe->esp + 4;
    else
        panic_info.esp = iframe->_esp_dummy + (offsetof(struct interrupt_frame, eflags) \
                     - offsetof(struct interrupt_frame, eax));
    panic(msg, &panic_info);
    __builtin_unreachable();
}

void division_error_handle(struct interrupt_frame iframe)
{
    panic_handle("division error exception", &iframe);
}

void debug_handle(struct interrupt_frame iframe)
{
    (void)iframe;
    printk("debug exception\n");
}

void nmi_handle(struct interrupt_frame iframe)
{
    panic_handle("fatal hardware error", &iframe);
}

void breakpoint_handle(struct interrupt_frame iframe)
{
    (void)iframe;
    printk("breakpoint exception\n");
}

void overflow_handle(struct interrupt_frame iframe)
{
    panic_handle("overflow exception", &iframe);
}

void bound_range_handle(struct interrupt_frame iframe)
{
    panic_handle("bound range exception", &iframe);
}

void invalid_opcode_handle(struct interrupt_frame iframe)
{
    panic_handle("invalid opcode", &iframe);
}

void device_not_avail_handle(struct interrupt_frame iframe)
{
    panic_handle("device not available", &iframe);
}

void double_fault_handle(struct interrupt_frame iframe)
{
    panic_handle("double fault", &iframe);
}

void coprocessor_handle(struct interrupt_frame iframe)
{
    panic_handle("coprocessor segment overrun", &iframe);
}

void invalid_tss_handle(struct interrupt_frame iframe)
{
    panic_handle("invalid tss", &iframe);
}

void segment_not_present_handle(struct interrupt_frame iframe)
{
    panic_handle("segment not present", &iframe);
}

void stack_fault_handle(struct interrupt_frame iframe)
{
    panic_handle("stack fault", &iframe);
}

void gpf_handle(struct interrupt_frame iframe)
{
    panic_handle("general protection fault", &iframe);
}

static inline void invalid_access_handle() 
{
    printk("segfault\n");
    // TODO: 프로세스 종료 루틴 연결
}

static void cow_handle(uint32_t *old_pte, uintptr_t fault_addr, struct interrupt_frame *iframe) 
{
    size_t pfn;
    void *new;
    uint32_t *new_pte;
    
    pfn = pfn_from_pte(*old_pte);
    if (page_ref[pfn] > 1) {
        new = vb_alloc(PAGE_SIZE);
        if (!new)
            panic_handle("out of memory", iframe);
        memcpy32(new, (void *)addr_from_pte(old_pte), PAGE_SIZE / 4);
        new_pte = (uint32_t *)pte_from_addr(new);
        *old_pte = page_from_pte(*new_pte) | (*old_pte & 0xDFF) | PG_RDWR;
        vb_unmap(new);
    } else {
        *old_pte = (*old_pte & ~PG_COW_RDWR) | PG_RDWR;
    }
    page_ref[pfn]--;
    tlb_flush(fault_addr);
}

void page_fault_handle(struct interrupt_frame iframe) 
{
    uintptr_t fault_addr;
    uint32_t *pte, *pde;

    asm volatile ("mov %%cr2, %0" : "=r" (fault_addr) :: "memory");
    if (pf_is_user(iframe.error_code)) {
        if (has_pgtab(fault_addr)) {
            pte = (uint32_t *)pte_from_addr(fault_addr);
            if (is_rdwr_cow(*pte)) {
                cow_handle(pte, fault_addr, &iframe);
                return;
            }
            if (pg_is_reserve(iframe.error_code, *pte)) {
                MAKE_PRESENT_PTE(*pte);
                return;
            }
        }
        invalid_access_handle();
    } else {
        pde = (uint32_t *)pde_from_addr(fault_addr);
        if (pg_is_reserve(iframe.error_code, *pde)) {
            MAKE_PRESENT_PDE(*pde);
            return;
        }
        panic_handle("page fault", &iframe);
    }
}

void floating_point_handle(struct interrupt_frame iframe)
{
    panic_handle("floating point exception", &iframe);
}

void alignment_check_handle(struct interrupt_frame iframe)
{
    panic_handle("alignment check exception", &iframe);
}

void machine_check_handle(struct interrupt_frame iframe)
{
    panic_handle("machine check exception", &iframe);
}

void simd_floating_point_handle(struct interrupt_frame iframe)
{
    panic_handle("simd floating point exception", &iframe);
}

void virtualization_handle(struct interrupt_frame iframe)
{
    panic_handle("virtualization exception", &iframe);
}

void control_protection_handle(struct interrupt_frame iframe)
{
    panic_handle("control protection exception", &iframe);
}

void fpu_error_handle(struct interrupt_frame iframe)
{
    panic_handle("FPU error interrupt", &iframe);
}

void pit_handle(struct interrupt_frame iframe)
{
    pic_send_eoi(PIT_IRQ);
    if (current->time_slice_remaining != 0) {
        if (current->time_slice_remaining <= 1)
            schedule(&iframe);
        else 
            current->time_slice_remaining--;
    }
}

void keyboard_handle(void)
{
	static uint8_t shift_flag;
	uint8_t keycode;
	char c;

	keycode = inb(PS2CTRL_DATA_PORT);
	if ((keycode & BREAKCODE_MASK) == 0) {
		c = key_map[keycode];
		if (keycode == LEFT_SHIFT_PRESS || keycode == RIGHT_SHIFT_PRESS) {
			shift_flag = 1;
		}
		else if (keycode >= F1_PRESS && keycode <= F6_PRESS) {
			tty_change(keycode - F1_PRESS);
		}
		else if (c == '\b') {
			tty_delete_input();	
		}
		else if (c == '\n') {
			tty_enter_input();
			printk(TTY_PROMPT);
		}
		else if (c == '\t') {
			for (size_t i = 0; i < TAB_SIZE; i++)
				tty_add_input(' ');
		}
		else if (c) {
			if (shift_flag)
				c = shift_key_map[keycode];
			tty_add_input(c);
		}
	} else {
		if (keycode == LEFT_SHIFT_RELEASE || keycode == RIGHT_SHIFT_RELEASE)
			shift_flag = 0;
	}
	pic_send_eoi(KEYBOARD_IRQ);
}

int syscall_handle(struct syscall_frame sframe)
{
    printk("%s\n", sframe.iframe.ebx);
    return 0;
}