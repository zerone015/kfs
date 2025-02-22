NAME = yosonos.bin

CC = i686-elf-gcc
AS = nasm

CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra
SFLAGS = -felf32
LFLAGS = -ffreestanding -O2 -nostdlib

LINKER = linker.ld

SRCS-C = kernel.c tty.c printf.c putchar.c strlen.c pic.c idt.c keyboard_handle.c vga.c
SRCS-S = boot.s outb.s inb.s idt_load.s keyboard_handler.s
OBJS = $(SRCS-C:%.c=%.o) $(SRCS-S:%.s=%.o)

%.o: %.s
	$(AS) $(SFLAGS) $< -o $@
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

all: $(NAME)

$(NAME): $(OBJS) $(LINKER)
	$(CC) -T $(LINKER) -o $@ $(LFLAGS) $(OBJS) -lgcc

clean:
	rm -f $(wildcard *.o)

fclean: clean 
	rm -f $(NAME)

.PHONY: all clean fclean
