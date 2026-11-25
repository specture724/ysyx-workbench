#include <algorithm>
#include <cstdint>
#include <cstdio>

#include "consts.hpp"
#include "pmem.hpp"

std::array<uint32_t, PMEM_WORDS> pmem;

void pmem_init() {
    pmem.fill(NOP);

    const uint32_t program[] = {
        0x10000093u, // addi x1, x0, 0x100
        0x12300113u, // addi x2, x0, 0x123
        0x0020a023u, // sw   x2, 0(x1)
        0x0000a183u, // lw   x3, 0(x1)
        0x04500213u, // addi x4, x0, 0x45
        0x004080a3u, // sb   x4, 1(x1)
        0x0010c283u, // lbu  x5, 1(x1)
        0x0000c303u, // lbu  x6, 0(x1)
        0x0000a383u, // lw   x7, 0(x1)
        0x06700493u, // addi x9, x0, 0x67
        0x009081a3u, // sb   x9, 3(x1)
        0x08900513u, // addi x10, x0, 0x89
        0x00a08123u, // sb   x10, 2(x1)
        0x0030c583u, // lbu  x11, 3(x1)
        0x0020c603u, // lbu  x12, 2(x1)
        0x0000a683u, // lw   x13, 0(x1)
        0x12345737u, // lui  x14, 0x12345
        0xfffff7b7u, // lui  x15, 0xfffff
        0x00001837u, // lui  x16, 0x00001
        0x12345037u, // lui  x0,  0x12345
        0x800008b7u, // lui  x17, 0x80000
        0x12345937u, // lui  x18, 0x12345
        0x67890913u, // addi x18, x18, 0x678
        0x800009b7u, // lui  x19, 0x80000
        0x10098993u, // addi x19, x19, 0x100
        0x00100073u, // ebreak
        0x07f00413u, // addi x8, x0, 0x7f
    };

    std::copy(program, program + sizeof(program) / sizeof(program[0]),
              pmem.begin());
}

extern "C" int pmem_read(int raddr) {
    const uint32_t addr = static_cast<uint32_t>(raddr);
    const uint32_t index = addr >> 2;
    if (index >= PMEM_WORDS) {
        std::fprintf(stderr, "pmem_read: address 0x%08x out of range\n", addr);
        return NOP;
    }
    return static_cast<int>(pmem[index]);
}

// wmask 是已经按字内字节位置排好的 4 位 byte mask（bit0 = byte0），
// 例如 SB 地址 0x...01 -> 0x02，SW -> 0x0f。
extern "C" void pmem_write(int waddr, int wdata, signed char wmask) {
    const uint32_t addr = static_cast<uint32_t>(waddr);
    const uint32_t index = addr >> 2;
    if (index >= PMEM_WORDS) {
        std::fprintf(stderr, "pmem_write: address 0x%08x out of range\n", addr);
        return;
    }

    const uint8_t m = static_cast<uint8_t>(wmask);
    uint32_t mask = 0;
    for (int i = 0; i < 4; i++) {
        if (m & (1u << i))
            mask |= 0xFFu << (8 * i);
    }

    pmem[index] = (pmem[index] & ~mask) |
                  (static_cast<uint32_t>(wdata) & mask);
}
