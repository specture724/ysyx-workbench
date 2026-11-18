CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -g -Wall -Wextra -Wpedantic

# RISC-V toolchain used only for the bundled example.
# Arch package: riscv64-linux-gnu-gcc (ships gcc / g++ / binutils).
RISCV_PREFIX ?= riscv64-linux-gnu-
RV_CC        := $(RISCV_PREFIX)gcc
RV_LD        := $(RISCV_PREFIX)ld
RV_OBJCOPY   := $(RISCV_PREFIX)objcopy
RV_FLAGS     ?= -march=rv32i -mabi=ilp32

BUILD_DIR := ../build
EMU       := $(BUILD_DIR)/emu
CPPFLAGS  += -Iinclude
SRCS      := src/emu_main.cpp src/loader.cpp src/minirvEMU.cpp
HDRS      := include/loader.h include/minirvEMU.h

EXAMPLE_OBJ := $(BUILD_DIR)/program.o
EXAMPLE_ELF := $(BUILD_DIR)/program.elf
EXAMPLE_BIN := $(BUILD_DIR)/program.bin

.PHONY: all example run clean

all: $(EMU)

$(EMU): $(SRCS) $(HDRS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SRCS) -o $@

$(EXAMPLE_OBJ): examples/program.S
	@mkdir -p $(BUILD_DIR)
	$(RV_CC) $(RV_FLAGS) -c $< -o $@

$(EXAMPLE_ELF): $(EXAMPLE_OBJ)
	$(RV_LD) -m elf32lriscv --image-base=0 -Ttext=0 -e _start -o $@ $<

$(EXAMPLE_BIN): $(EXAMPLE_ELF)
	$(RV_OBJCOPY) -O binary $< $@

example: $(EXAMPLE_BIN)
	@echo "example binary: $(EXAMPLE_BIN)"

run: $(EMU)
	@$(EMU) $(ARGS)

clean:
	rm -f $(EMU) $(EXAMPLE_OBJ) $(EXAMPLE_ELF) $(EXAMPLE_BIN)
