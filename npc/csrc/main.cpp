#include "Vtop.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

#include <filesystem>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

namespace fs = std::filesystem;

int main(int argc, char** argv) {
    VerilatedContext* contextp = new VerilatedContext;
    contextp->commandArgs(argc, argv);
    fs::path exe_dir = fs::read_symlink("/proc/self/exe").parent_path();
    fs::path wave_path = exe_dir / "wave.vcd";
    Vtop* top = new Vtop{contextp};

    // Enable tracing
    contextp->traceEverOn(true);

    VerilatedVcdC trace;
    top->trace(&trace, 99);
    trace.open(wave_path.string().c_str());

    for (int i = 0; i < 4; i++) {
        int a = i & 1;
        int b = (i >> 1) & 1;

        top->a = a;
        top->b = b;

        // Evaluate DUT
        top->eval();

        // Dump waveform at current time
        trace.dump(contextp->time());

        printf("a = %d, b = %d, f = %d\n",
               a, b, top->f);

        assert(top->f == (a ^ b));

        // Advance simulation time
        contextp->timeInc(10);
    }

    top->final();
    trace.close();

    delete top;
    delete contextp;

    return 0;
}