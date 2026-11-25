#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "verilated.h"
#include "Vtop.h"
#include "Vtop___024root.h"
#include "consts.hpp"
#include "pmem.hpp"
static Vtop *top = nullptr;

extern "C" void ebreak_handler() {
    std::printf("[DPI-C] ebreak instruction detected. Ending simulation.\n");
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

    constexpr int kMaxCycles = 30;
    int cycle = 0;
    while (!Verilated::gotFinish() && cycle < kMaxCycles) {
        top->inst = pmem_read(top->pc);
        top->eval();
        print_state(cycle);
        dut_single_cycle();
        ++cycle;
    }

    bool ok = true;
    if (!Verilated::gotFinish()) {
        std::printf("timeout: EBREAK was not reached in %d cycles\n", kMaxCycles);
        ok = false;
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
        {1, 0x00000100u, "x1"},
        {2, 0x00000123u, "x2"},
        {3, 0x00000123u, "x3"},
        {4, 0x00000045u, "x4"},
        {5, 0x00000045u, "x5"},
        {6, 0x00000023u, "x6"},
        {7, 0x00004523u, "x7"},
        {8, 0x00000000u, "x8"},
        {9, 0x00000067u, "x9"},
        {10, 0x00000089u, "x10"},
        {11, 0x00000067u, "x11"},
        {12, 0x00000089u, "x12"},
        {13, 0x67894523u, "x13"},
    };

    std::printf("\nfinal pc = 0x%08x\n", pc);
    for (const auto &c : checks) {
        const uint32_t got = rf[c.index];
        const bool pass = (got == c.expect);
        std::printf("  %-2s = 0x%08x (%-10u) %s\n",
                    c.name, got, got, pass ? "ok" : "MISMATCH");
        ok &= pass;
    }

    if (pc != EBREAK_PC) {
        std::printf("  pc   = 0x%08x (expect 0x%08x) MISMATCH\n",
                    pc, EBREAK_PC);
        ok = false;
    }

    const uint32_t mem_word = pmem[0x100 >> 2];
    const uint32_t expect_mem = 0x67894523u;
    const bool mem_ok = (mem_word == expect_mem);
    std::printf("  mem[0x100] = 0x%08x (expect 0x%08x) %s\n",
                mem_word, expect_mem, mem_ok ? "ok" : "MISMATCH");
    ok &= mem_ok;

    std::printf("%s\n", ok ? "PASS" : "FAIL");

    top->final();
    delete top;
    return ok ? 0 : 1;
}
