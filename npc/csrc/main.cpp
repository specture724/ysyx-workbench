#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "verilated.h"
#include "Vtop.h"
#include "Vtop___024root.h"  // rootp internals: pc / gpr registers

void ref_init();
void ref_inst_cycle();
uint8_t *ref_get_regs();
uint8_t ref_get_pc();

static Vtop *top = nullptr;


static void dut_single_cycle() {
    top->clk = 0; top->eval();
    top->clk = 1; top->eval();
}

static void dut_reset(int n) {
    top->rst = 1;
    while (n--) dut_single_cycle();
    top->rst = 0;
}

static void dut_get_regs(uint8_t regs[4]) {
    regs[0] = top->rootp->top__DOT__u_gpr__DOT__r0;
    regs[1] = top->rootp->top__DOT__u_gpr__DOT__r1;
    regs[2] = top->rootp->top__DOT__u_gpr__DOT__r2;
    regs[3] = top->rootp->top__DOT__u_gpr__DOT__r3;
}

static uint8_t dut_get_pc() {
    return top->rootp->top__DOT__pc;
}

static int check_regs(const uint8_t *dut_regs, const uint8_t *ref_regs) {
    for (int i = 0; i < 4; i++)
        if (dut_regs[i] != ref_regs[i]) return 0;
    return 1;
}

static void difftest_check(int cycle) {
    uint8_t dut_regs[4];
    dut_get_regs(dut_regs);
    uint8_t *ref_regs = ref_get_regs();

    if (check_regs(dut_regs, ref_regs) && dut_get_pc() == ref_get_pc()) return;

    printf("DiffTest failed after %d instructions\n", cycle);
    printf("  pc     dut = 0x%02x, ref = 0x%02x\n", dut_get_pc(), ref_get_pc());
    for (int i = 0; i < 4; i++)
        if (dut_regs[i] != ref_regs[i])
        printf("  GPR[%d] dut = 0x%02x, ref = 0x%02x\n", i, dut_regs[i], ref_regs[i]);
    printf("Simulation stop\n");
    exit(1);
}

int main(int argc, char *argv[]) {
    Verilated::commandArgs(argc, argv);
    top = new Vtop;

    ref_init();
    dut_reset(10);

    int cycle = 0;
    while (1) {
        uint8_t ref_pc_prev = ref_get_pc();
        dut_single_cycle();
        ref_inst_cycle();
        difftest_check(cycle);
        cycle++;

        if (ref_get_pc() == ref_pc_prev) break;  // branch-to-self: program halted
        if (cycle > 10000) { printf("timeout\n"); break; }
    }

    uint8_t *gpr = ref_get_regs();
    printf("HIT GOOD TRAP at pc = 0x%02x after %d instructions\n", ref_get_pc(), cycle);
    printf("final GPR: r0=%d r1=%d r2=%d r3=%d (expect r2 = 55)\n",
            gpr[0], gpr[1], gpr[2], gpr[3]);

    top->final();
    delete top;
    return 0;
}
