#ifndef CONSTS_HPP
#define CONSTS_HPP

#include <cstdint>

static constexpr uint32_t RESET_PC   = 0x00000000u;
static constexpr uint32_t PMEM_WORDS = 1024;
static constexpr uint32_t NOP        = 0x00000013u; // addi x0, x0, 0
static constexpr uint32_t EBREAK_PC  = 0x00000040u;

#endif
