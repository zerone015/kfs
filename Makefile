.DEFAULT_GOAL = all

NAME = kernel.elf

CC-NATIVE = gcc -m32
CC = i686-elf-gcc
AS = nasm

CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -MMD -MP -I$(HEADER-DIR)
SFLAGS = -felf32 -I$(HEADER-DIR)
LFLAGS = -ffreestanding -O2 -nostdlib

LINKER = linker.ld

HEADER-DIR = include
SRC-DIR = src
OBJ-DIR = build
ASM-DIR = asm

SRC = $(SRC-DIR)/kernel.c $(SRC-DIR)/tty.c $(SRC-DIR)/printk.c $(SRC-DIR)/utils.c $(SRC-DIR)/pic.c $(SRC-DIR)/idt.c $(SRC-DIR)/vga.c $(SRC-DIR)/gdt.c \
	$(SRC-DIR)/pmm.c $(SRC-DIR)/vmm.c $(SRC-DIR)/panic.c $(SRC-DIR)/interrupt.c $(SRC-DIR)/hmm.c $(SRC-DIR)/sched.c $(SRC-DIR)/rbtree.c $(SRC-DIR)/pit.c \
	$(SRC-DIR)/pid.c $(SRC-DIR)/exec.c $(SRC-DIR)/syscall.c $(SRC-DIR)/proc.c $(SRC-DIR)/daemon.c $(SRC-DIR)/signal.c $(SRC-DIR)/pci.c $(SRC-DIR)/ata.c
ASM = $(ASM-DIR)/boot.asm $(ASM-DIR)/panic.asm $(ASM-DIR)/interrupt.asm $(ASM-DIR)/syscall.asm $(ASM-DIR)/sched.asm
OBJ = $(SRC:$(SRC-DIR)/%.c=$(OBJ-DIR)/%.o) $(ASM:$(ASM-DIR)/%.asm=$(OBJ-DIR)/_%.o)

GEN-DIR = gen
OFFSET-HEADER = $(HEADER-DIR)/offsets.inc
GEN-BINARY = $(GEN-DIR)/gen_offsets
GEN-SRC = $(GEN-DIR)/gen_offsets.c
GEN-OBJ = $(GEN-SRC:$(GEN-DIR)/%.c=$(GEN-DIR)/%.o)

DEP = $(SRC:$(SRC-DIR)/%.c=$(OBJ-DIR)/%.d) $(GEN-SRC:$(GEN-DIR)/%.c=$(GEN-DIR)/%.d) 
ASM-DEP = $(ASM:$(ASM-DIR)/%.asm=$(OBJ-DIR)/_%.d)
-include $(DEP)
-include $(ASM-DEP)

$(OBJ-DIR):
	mkdir -p $(OBJ-DIR)

$(OFFSET-HEADER): $(GEN-OBJ)
	$(CC-NATIVE) -o $(GEN-BINARY) $^
	$(GEN-BINARY) > $@
	rm -f $(GEN-BINARY)

$(GEN-DIR)/%.o: $(GEN-DIR)/%.c
	$(CC-NATIVE) -MMD -MP -I$(HEADER-DIR) -c $< -o $@

$(OBJ-DIR)/_%.d: $(ASM-DIR)/%.asm
	nasm -M -I$(HEADER-DIR) -MP -MT $(OBJ-DIR)/_$*.o $< > $@

$(OBJ-DIR)/_%.o: $(ASM-DIR)/%.asm
	$(AS) $(SFLAGS) $< -o $@ 

$(OBJ-DIR)/%.o: $(SRC-DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

all: $(OBJ-DIR) $(OFFSET-HEADER) $(ASM-DEP) $(NAME)

$(NAME): $(OBJ) $(LINKER)
	$(CC) -T $(LINKER) -o $@ $(LFLAGS) $(OBJ) -lgcc

clean:
	rm -f $(OBJ-DIR)/*.o $(OBJ-DIR)/*.d
	rm -f $(GEN-DIR)/*.o $(GEN-DIR)/*.d

fclean: clean
	rm -f $(NAME)

.PHONY: all clean fclean