NAME = kernel.elf

CC = i686-elf-gcc
AS = nasm

CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -I$(HEADER-DIR)
SFLAGS = -felf32
LFLAGS = -ffreestanding -O2 -nostdlib

LINKER = linker.ld

HEADER-DIR = ./include
SRC-DIR = ./src
OBJ-DIR = ./build
ASM-DIR = ./asm

SRC = $(SRC-DIR)/kernel.c $(SRC-DIR)/tty.c $(SRC-DIR)/printk.c $(SRC-DIR)/utils.c $(SRC-DIR)/pic.c $(SRC-DIR)/idt.c $(SRC-DIR)/keyboard_handle.c $(SRC-DIR)/vga.c $(SRC-DIR)/gdt.c \
	$(SRC-DIR)/pmm.c $(SRC-DIR)/panic.c
ASM = $(ASM-DIR)/boot.s $(ASM-DIR)/outb.s $(ASM-DIR)/inb.s $(ASM-DIR)/idt_load.s $(ASM-DIR)/keyboard_handler.s $(ASM-DIR)/gdt_load.s
OBJ = $(SRC:$(SRC-DIR)/%.c=$(OBJ-DIR)/%.o) $(ASM:$(ASM-DIR)/%.s=$(OBJ-DIR)/%.o)

$(OBJ-DIR)/%.o: $(ASM-DIR)/%.s
	$(AS) $(SFLAGS) $< -o $@

$(OBJ-DIR)/%.o: $(SRC-DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

all: $(OBJ-DIR) $(NAME)

$(NAME): $(OBJ) $(LINKER)
	$(CC) -T $(LINKER) -o $@ $(LFLAGS) $(OBJ) -lgcc

$(OBJ-DIR):
	mkdir -p $(OBJ-DIR)

clean:
	rm -f $(wildcard $(OBJ-DIR)/*.o)

fclean: clean
	rm -f $(NAME)

.PHONY: all clean fclean
