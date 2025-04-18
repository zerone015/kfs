.DEFAULT_GOAL = all

NAME = kernel.elf

CC-NATIVE = gcc
CC = i686-elf-gcc
AS = nasm

CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -MMD -MP -I$(HEADER-DIR)
SFLAGS = -felf32 -I$(HEADER-DIR)
LFLAGS = -ffreestanding -O2 -nostdlib

LINKER = linker.ld

HEADER-DIR = ./include
SRC-DIR = ./src
OBJ-DIR = ./build
ASM-DIR = ./asm

GEN-DIR = ./gen
OFFSET-HEADER = $(HEADER-DIR)/offsets.inc
GEN-BINARY = $(GEN-DIR)/gen_offsets
GEN-SRC = $(GEN-DIR)/gen_offsets.c

SRC = $(SRC-DIR)/kernel.c $(SRC-DIR)/tty.c $(SRC-DIR)/printk.c $(SRC-DIR)/utils.c $(SRC-DIR)/pic.c $(SRC-DIR)/idt.c $(SRC-DIR)/vga.c $(SRC-DIR)/gdt.c \
	$(SRC-DIR)/pmm.c $(SRC-DIR)/vmm.c $(SRC-DIR)/panic.c $(SRC-DIR)/interrupt.c $(SRC-DIR)/hmm.c $(SRC-DIR)/sched.c $(SRC-DIR)/rbtree.c $(SRC-DIR)/pit.c \
	$(SRC-DIR)/pid.c $(SRC-DIR)/exec.c $(SRC-DIR)/syscall.c $(SRC-DIR)/proc.c
ASM = $(ASM-DIR)/boot.asm $(ASM-DIR)/panic.asm $(ASM-DIR)/interrupt.asm $(ASM-DIR)/syscall.asm $(ASM-DIR)/sched.asm
OBJ = $(SRC:$(SRC-DIR)/%.c=$(OBJ-DIR)/%.o) $(ASM:$(ASM-DIR)/%.asm=$(OBJ-DIR)/_%.o)
DEP = $(SRC:$(SRC-DIR)/%.c=$(OBJ-DIR)/%.d)
-include $(DEP)

$(OFFSET-HEADER): $(GEN-SRC)
	$(CC-NATIVE) $(GEN-SRC) -o $(GEN-BINARY) -I$(HEADER-DIR)
	$(GEN-BINARY) > $@
	rm -f $(GEN-BINARY)

$(OBJ-DIR)/_%.o: $(ASM-DIR)/%.asm
	$(AS) $(SFLAGS) $< -o $@ 

$(OBJ-DIR)/%.o: $(SRC-DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

all: $(OBJ-DIR) $(NAME)

$(NAME): $(OFFSET-HEADER) $(OBJ) $(LINKER)
	$(CC) -T $(LINKER) -o $@ $(LFLAGS) $(OBJ) -lgcc

$(OBJ-DIR):
	mkdir -p $(OBJ-DIR)

clean:
	rm -f $(wildcard $(OBJ-DIR)/*.o) $(wildcard $(OBJ-DIR)/*.d)

fclean: clean
	rm -f $(NAME)

.PHONY: all clean fclean