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

ISA_TEST_OBJ := $(BUILD_DIR)/isa_test.o
ISA_TEST_ELF := $(BUILD_DIR)/isa_test.elf
ISA_TEST_BIN := $(BUILD_DIR)/isa_test.bin

.PHONY: all example isa-test check-isa run clean

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

$(ISA_TEST_OBJ): examples/isa_test.S
	@mkdir -p $(BUILD_DIR)
	$(RV_CC) $(RV_FLAGS) -c $< -o $@

$(ISA_TEST_ELF): $(ISA_TEST_OBJ)
	$(RV_LD) -m elf32lriscv --image-base=0 -Ttext=0 --no-relax -e _start -o $@ $<

$(ISA_TEST_BIN): $(ISA_TEST_ELF)
	$(RV_OBJCOPY) -O binary $< $@

isa-test: $(ISA_TEST_BIN)
	@echo "isa test binary: $(ISA_TEST_BIN)"

check-isa: $(EMU) $(ISA_TEST_BIN)
	$(EMU) --bin $(ISA_TEST_BIN) --trace \
		--expect x0=0 --expect a0=5 --expect a1=0xfffffffd --expect a2=2 \
		--expect a3=0x12345000 --expect a4=5 --expect a5=2 --expect a6=7 \
		--expect a7=42 --expect s0=0xab --expect s1=0xab05 --expect s2=0xfd \
		--expect t0=0x200 --expect t1=0xab --expect ra=0x48 --expect pc=0x50

run: $(EMU)
	@$(EMU) $(ARGS)

clean:
	rm -f $(EMU) $(EXAMPLE_OBJ) $(EXAMPLE_ELF) $(EXAMPLE_BIN) \
	      $(ISA_TEST_OBJ) $(ISA_TEST_ELF) $(ISA_TEST_BIN)
