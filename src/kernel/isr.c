#include "isr.h"
#include "tty.h"
#include "pic.h"
#include "io.h"
#include "printk.h"
#include "pmm.h"
#include "paging.h"
#include "panic.h"
#include "sched.h"
#include "init.h"
#include "proc.h"
#include "signal.h"

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

void division_error_handle(void)
{
    printk("divison exception\n");
    signal_pending_set(current, SIGFPE);
}

void debug_handle(void)
{
    printk("debug exception\n");
}

void invalid_opcode_handle(void)
{
    printk("invalid opcode\n");
    signal_pending_set(current, SIGILL);
}

void device_not_avail_handle(void)
{
    printk("device not available\n");
    signal_pending_set(current, SIGILL);
}

void gpf_handle(void)
{
    printk("general protection fault\n");
    signal_pending_set(current, SIGILL);
}

void page_fault_handle(uintptr_t fault_addr, int error_code) 
{
    uint32_t *pte, *pde;

    if (user_space(fault_addr)) {
        if (pgtab_allocated(fault_addr)) {
            pte = pte_from_va(fault_addr);
            if (is_rdwr_cow(*pte)) {
                cow_handle(pte, fault_addr);
                return;
            }
            if (entry_reserved(*pte)) {
                MAKE_PRESENT_PTE(*pte);
                return;
            }
        }
    } else {
        if (!pf_is_usermode(error_code)) {
            pde = pde_from_va(fault_addr);
            if (entry_reserved(*pde)) {
                MAKE_PRESENT_PDE(*pde);
                return;
            }
            do_panic("PF???????????");
        }
    }
    printk("segfault!\n");
    signal_pending_set(current, SIGSEGV);
}

void pit_handle(void)
{
    pic_send_eoi(PIT_IRQ);
    if (current->time_slice_remaining != 0) {
        if (current->time_slice_remaining <= 1)
            schedule();
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