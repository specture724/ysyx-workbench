#ifndef MINIRV_EMU_H
#define MINIRV_EMU_H

#include <cstddef>
#include <cstdint>

// Reference model for RV32I (currently addi / jalr).
//
// There is a single unified RAM: instructions and data share the same
// byte-addressed memory. PC is a byte address and starts at 0 after ref_init().

// Reset PC to 0 and clear the general-purpose registers.
void ref_init ();

// Execute one instruction. Returns 1 after executing EBREAK (the program
// asks to stop), 0 otherwise. May throw std::runtime_error on an illegal
// instruction or an out-of-range / unaligned fetch.
uint8_t ref_inst_cycle ();

// Set the PC explicitly (used by the loader to enter at a given address).
void ref_set_pc (uint32_t pc);

// Copy a flat image (e.g. `objcopy -O binary` output) into RAM at `base`.
// Throws std::runtime_error if the image does not fit.
void ref_load_ram (const uint8_t *data, std::size_t size, uint32_t base = 0);

// Read-only view of the unified RAM.
const uint8_t *ref_get_ram ();
uint32_t       ref_ram_size ();

// difftest / debug interface.
uint32_t *ref_get_regs ();
uint32_t  ref_get_pc ();

#endif // MINIRV_EMU_H
