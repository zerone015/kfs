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

C_SOURCES := $(shell find $(SRC-DIR) -type f -name "*.c")
ASM_SOURCES := $(shell find $(SRC-DIR) -type f -name "*.asm")

C_OBJ := $(C_SOURCES:$(SRC-DIR)/%.c=$(OBJ-DIR)/%.o)

ASM_OBJ := $(ASM_SOURCES:$(SRC-DIR)/%.asm=$(OBJ-DIR)/_%.o)

OBJ := $(C_OBJ) $(ASM_OBJ)

DEP := $(OBJ:.o=.d)

-include $(DEP)

GEN-DIR = gen
OFFSET-HEADER = $(HEADER-DIR)/offsets.inc
GEN-BINARY = $(GEN-DIR)/gen_offsets
GEN-SRC = $(GEN-DIR)/gen_offsets.c
GEN-OBJ = $(GEN-DIR)/gen_offsets.o

$(GEN-DIR)/%.o: $(GEN-DIR)/%.c
	$(CC-NATIVE) -MMD -MP -I$(HEADER-DIR) -c $< -o $@

$(OFFSET-HEADER): $(GEN-OBJ)
	$(CC-NATIVE) -o $(GEN-BINARY) $^
	$(GEN-BINARY) > $@
	rm -f $(GEN-BINARY)

$(OBJ-DIR):
	mkdir -p $(OBJ-DIR)

$(OBJ-DIR)/%.o: $(SRC-DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ-DIR)/_%.o: $(SRC-DIR)/%.asm
	@mkdir -p $(dir $@)
	$(AS) $(SFLAGS) $< -o $@

all: $(OBJ-DIR) $(OFFSET-HEADER) $(NAME)

$(NAME): $(OBJ) $(LINKER)
	$(CC) -T $(LINKER) -o $@ $(LFLAGS) $(OBJ) -lgcc

clean:
	rm -rf $(OBJ-DIR)
	rm -f $(GEN-DIR)/*.o $(GEN-DIR)/*.d

fclean: clean
	rm -f $(NAME)

.PHONY: all clean fclean
