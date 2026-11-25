#ifndef PMEM_HPP
#define PMEM_HPP

#include <array>
#include <cstdint>

#include "consts.hpp"

extern std::array<uint32_t, PMEM_WORDS> pmem;

void pmem_init();

extern "C" int  pmem_read(int addr);
extern "C" void pmem_write(int waddr, int wdata, signed char wmask);

#endif
