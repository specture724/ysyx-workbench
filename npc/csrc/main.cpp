#include "Vtop.h"
#include "verilated.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char** argv) {
    VerilatedContext* contextp = new VerilatedContext;
    contextp->commandArgs(argc, argv);
    Vtop* top = new Vtop{contextp};
    // while (!contextp->gotFinish()) { top->eval(); }
    int i = 0;
    while (i < 4) {
        int a = i & 1;
        int b = i >> 1 & 1;
        top->a = a;
        top->b = b;
        top->eval();
        printf("a = %d, b = %d, f = %d\n", a, b, top->f);
        assert(top->f == (a ^ b));
        i++;
    }
    delete top;
    delete contextp;
    return 0;
}