#include "interrupt.h"
#include "tty.h"
#include "pic.h"
#include "io.h"
#include "printk.h"
#include "pmm.h"
#include "paging.h"
#include "panic.h"

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

static void __do_panic(const char *msg, struct interrupt_frame *iframe)
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
    if (__from_user(iframe->cs))
        panic_info.esp = iframe->esp + 4;
    else
        panic_info.esp = iframe->_esp_dummy + (offsetof(struct interrupt_frame, eflags) \
                     - offsetof(struct interrupt_frame, eax));
    panic(msg, &panic_info);
}

void division_error_handle(struct interrupt_frame iframe)
{
    __do_panic("division error exception", &iframe);
}

void debug_handle(struct interrupt_frame iframe)
{
    (void)iframe;
    printk("debug exception\n");
}

void nmi_handle(struct interrupt_frame iframe)
{
    __do_panic("fatal hardware error", &iframe);
}

void breakpoint_handle(struct interrupt_frame iframe)
{
    (void)iframe;
    printk("breakpoint exception\n");
}

void overflow_handle(struct interrupt_frame iframe)
{
    __do_panic("overflow exception", &iframe);
}

void bound_range_handle(struct interrupt_frame iframe)
{
    __do_panic("bound range exception", &iframe);
}

void invalid_opcode_handle(struct interrupt_frame iframe)
{
    __do_panic("invalid opcode", &iframe);
}

void device_not_avail_handle(struct interrupt_frame iframe)
{
    __do_panic("device not available", &iframe);
}

void double_fault_handle(struct interrupt_frame iframe)
{
    __do_panic("double fault", &iframe);
}

void coprocessor_handle(struct interrupt_frame iframe)
{
    __do_panic("coprocessor segment overrun", &iframe);
}

void invalid_tss_handle(struct interrupt_frame iframe)
{
    __do_panic("invalid tss", &iframe);
}

void segment_not_present_handle(struct interrupt_frame iframe)
{
    __do_panic("segment not present", &iframe);
}

void stack_fault_handle(struct interrupt_frame iframe)
{
    __do_panic("stack fault", &iframe);
}

void gpf_handle(struct interrupt_frame iframe)
{
    __do_panic("general protection fault", &iframe);
}

void page_fault_handle(struct interrupt_frame iframe) 
{
    uintptr_t fault_addr;
    uint32_t *pde;

    asm volatile ("mov %%cr2, %0" : "=r" (fault_addr) :: "memory");
    pde = (uint32_t *)pde_from_addr(fault_addr);
    if (!__is_reserve(iframe.error_code, *pde))
        __do_panic("page fault", &iframe);
    *pde = __make_pde(*pde);
}

void floating_point_handle(struct interrupt_frame iframe)
{
    __do_panic("floating point exception", &iframe);
}

void alignment_check_handle(struct interrupt_frame iframe)
{
    __do_panic("alignment check exception", &iframe);
}

void machine_check_handle(struct interrupt_frame iframe)
{
    __do_panic("machine check exception", &iframe);
}

void simd_floating_point_handle(struct interrupt_frame iframe)
{
    __do_panic("simd floating point exception", &iframe);
}

void virtualization_handle(struct interrupt_frame iframe)
{
    __do_panic("virtualization exception", &iframe);
}

void control_protection_handle(struct interrupt_frame iframe)
{
    __do_panic("control protection exception", &iframe);
}

void fpu_error_handle(struct interrupt_frame iframe)
{
    __do_panic("FPU error interrupt", &iframe);
}

void pit_handle(struct interrupt_frame iframe)
{
    (void)iframe;
    printk("pit handler\n");
    pic_send_eoi(PIT_IRQ);
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