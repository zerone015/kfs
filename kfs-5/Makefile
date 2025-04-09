.DEFAULT_GOAL = all

NAME = kernel.elf

CC = i686-elf-gcc
AS = nasm

CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -MMD -MP -I$(HEADER-DIR)
SFLAGS = -felf32
LFLAGS = -ffreestanding -O2 -nostdlib

LINKER = linker.ld

HEADER-DIR = ./include
SRC-DIR = ./src
OBJ-DIR = ./build
ASM-DIR = ./asm

SRC = $(SRC-DIR)/kernel.c $(SRC-DIR)/tty.c $(SRC-DIR)/printk.c $(SRC-DIR)/utils.c $(SRC-DIR)/pic.c $(SRC-DIR)/idt.c $(SRC-DIR)/vga.c $(SRC-DIR)/gdt.c \
	$(SRC-DIR)/pmm.c $(SRC-DIR)/vmm.c $(SRC-DIR)/panic.c $(SRC-DIR)/interrupt.c $(SRC-DIR)/hmm.c $(SRC-DIR)/sched.c $(SRC-DIR)/rbtree.c $(SRC-DIR)/pit.c \
	$(SRC-DIR)/proc.c $(SRC-DIR)/pid.c $(SRC-DIR)/exec.c
ASM = $(ASM-DIR)/boot.s $(ASM-DIR)/interrupt_handler.s $(ASM-DIR)/do_panic.s
OBJ = $(SRC:$(SRC-DIR)/%.c=$(OBJ-DIR)/%.o) $(ASM:$(ASM-DIR)/%.s=$(OBJ-DIR)/%.o)
DEP = $(SRC:$(SRC-DIR)/%.c=$(OBJ-DIR)/%.d)
-include $(DEP)

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
	rm -f $(wildcard $(OBJ-DIR)/*.o) $(wildcard $(OBJ-DIR)/*.d)

fclean: clean
	rm -f $(NAME)

.PHONY: all clean fclean