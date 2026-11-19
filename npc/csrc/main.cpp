#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "verilated.h"
#include "Vtop.h"
#include "Vtop___024root.h"


static constexpr uint32_t RESET_PC   = 0x80000000u;
static constexpr uint32_t PMEM_WORDS = 1024;
static constexpr uint32_t NOP        = 0x00000013u; // addi x0, x0, 0

static std::array<uint32_t, PMEM_WORDS> pmem;
static Vtop *top = nullptr;

static void pmem_init() {
    pmem.fill(NOP);

    const uint32_t program[] = {
        0xffc08167u, // jalr x2, -4(x1)
        0x00100193u, // addi x3, x0, 1
        0x00200213u, // addi x4, x0, 2
        0x00300293u, // addi x5, x0, 3
        0x02a00313u, // addi x6, x0, 42
        0xffc08067u, // jalr x0, -4(x1)
        0x00700393u, // addi x7, x0, 7
    };

    std::copy(program, program + sizeof(program) / sizeof(program[0]),
              pmem.begin());
}

static uint32_t pmem_read(uint32_t addr) {
    const uint32_t index = (addr - RESET_PC) >> 2;
    if (index >= PMEM_WORDS) {
        std::fprintf(stderr, "pmem_read: address 0x%08x out of range\n", addr);
        return NOP;
    }
    return pmem[index];
}

static void dut_single_cycle() {
    top->clk = 0;
    top->eval();
    top->clk = 1;
    top->eval();
}

static void dut_reset(int cycles) {
    top->rst = 1;
    for (int i = 0; i < cycles; i++)
        dut_single_cycle();
    top->rst = 0;
}

static void print_state(int cycle) {
    auto *root = top->rootp;
    std::printf("cycle %2d: pc=0x%08x inst=0x%08x  wen=%0d x%0d<=0x%08x\n",
                cycle,
                static_cast<unsigned>(top->pc),
                static_cast<unsigned>(top->inst),
                static_cast<unsigned>(root->top__DOT__rf_wen),
                static_cast<unsigned>(root->top__DOT__rf_waddr),
                static_cast<unsigned>(root->top__DOT__rf_wdata));
}

int main(int argc, char *argv[]) {
    Verilated::commandArgs(argc, argv);
    top = new Vtop;

    pmem_init();
    dut_reset(5);

    top->rootp->top__DOT__u_rf__DOT__rf[1] = 0x80000015u;

    constexpr int kInstructions = 3;
    for (int cycle = 0; cycle < kInstructions; cycle++) {
        top->inst = pmem_read(top->pc);
        top->eval();
        print_state(cycle);
        dut_single_cycle();
    }

    auto *root = top->rootp;
    const uint32_t pc = top->pc;
    const auto &rf = root->top__DOT__u_rf__DOT__rf;

    struct Check {
        int         index;
        uint32_t    expect;
        const char *name;
    };
    const Check checks[] = {
        {0, 0x00000000u, "x0"},
        {1, 0x80000015u, "x1"},
        {2, 0x80000004u, "x2"},
        {3, 0x00000000u, "x3"},
        {4, 0x00000000u, "x4"},
        {5, 0x00000000u, "x5"},
        {6, 0x0000002au, "x6"},
        {7, 0x00000000u, "x7"},
    };

    bool ok = true;
    std::printf("\nfinal pc = 0x%08x\n", pc);
    for (const auto &c : checks) {
        const uint32_t got = rf[c.index];
        const bool pass = (got == c.expect);
        std::printf("  %-2s = 0x%08x (%-10u) %s\n",
                    c.name, got, got, pass ? "ok" : "MISMATCH");
        ok &= pass;
    }

    const uint32_t expect_pc = 0x80000010u;
    if (pc != expect_pc) {
        std::printf("  pc   = 0x%08x (expect 0x%08x) MISMATCH\n",
                    pc, expect_pc);
        ok = false;
    }

    std::printf("%s\n", ok ? "PASS" : "FAIL");

    top->final();
    delete top;
    return ok ? 0 : 1;
}
