.DEFAULT_GOAL = all

CC-NATIVE = gcc -m32
CC = $(if $(wildcard toolchain/bin/i686-elf-gcc),toolchain/bin/i686-elf-gcc,i686-elf-gcc)
AS = nasm

CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -MMD -MP -I$(HEADER-DIR)
SFLAGS = -felf32 -I$(HEADER-DIR)
LFLAGS = -ffreestanding -O2 -nostdlib

LINKER = linker.ld
HEADER-DIR = include
SRC-DIR = src

# Test sources
TEST-SRC-pmm             = pmm_test.c
TEST-SRC-vmm             = vmm_test.c
TEST-SRC-malloc          = hmm_test.c
TEST-SRC-fork            = fork_test.c
TEST-SRC-wait_exit       = wait_exit_test.c
TEST-SRC-signal          = signal_test.c
TEST-SRC-panic           = panic_test.asm
TEST-SRC-panic_exception = panic_test.asm

ifeq ($(TEST),)
OBJ-DIR = build
NAME = kernel.elf
else
OBJ-DIR = build/test-$(TEST)
NAME = $(OBJ-DIR)/kernel.elf
CFLAGS += -DKFS_TEST -DKFS_TEST_$(shell echo $(TEST) | tr a-z A-Z)
TEST-OBJ = $(OBJ-DIR)/test/$(TEST-SRC-$(TEST)).o
endif

C_SOURCES := $(shell find $(SRC-DIR) -type f -name "*.c")
ASM_SOURCES := $(shell find $(SRC-DIR) -type f -name "*.asm")

C_OBJ := $(C_SOURCES:$(SRC-DIR)/%.c=$(OBJ-DIR)/%.o)

ASM_OBJ := $(ASM_SOURCES:$(SRC-DIR)/%.asm=$(OBJ-DIR)/_%.o)

OBJ := $(C_OBJ) $(ASM_OBJ) $(TEST-OBJ)

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

$(OBJ-DIR)/test/%.c.o: test/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ-DIR)/test/%.asm.o: test/%.asm
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
