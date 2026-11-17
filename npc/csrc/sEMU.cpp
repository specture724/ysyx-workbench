#include <iostream>
#include <stdint.h>
#include <cassert>
#include <vector>

uint8_t PC = 0;
std::vector<uint8_t> GPR(4, 0x00);
std::vector<uint8_t> ROM(256, 0x00);

void set_rom() {
    ROM[0] = 0b1000'1010;  // li  r0, 10
    ROM[1] = 0b1001'0000;  // li  r1, 0
    ROM[2] = 0b1010'0000;  // li  r2, 0
    ROM[3] = 0b1011'0001;  // li  r3, 1
    ROM[4] = 0b0001'0111;  // add r1, r1, r3
    ROM[5] = 0b0010'1001;  // add r2, r2, r1
    ROM[6] = 0b1101'0001;  // bner0 r1, 4
    ROM[7] = 0b0110'1000;  // io r2, 0
    ROM[8] = 0b1110'0011; // bner0 r3, 8
}

void inst_cycle () {
    uint8_t instr = ROM[PC];
    // std::cout << "PC=" << static_cast<int>(PC) << std::endl;
    switch (instr >> 6)
    {
    case 0b00: { // add
        uint8_t rd = (instr & 0b0011'0000) >> 4;
        uint8_t rs1 = (instr & 0b0000'1100) >> 2;
        uint8_t rs2 = (instr & 0b0000'0011);
        GPR[rd] = GPR[rs1] + GPR[rs2];
        PC++;
        break;
    }
    case 0b01: { // io
        uint8_t rd = (instr & 0b0011'0000) >> 4;
        uint8_t s = (instr & 0b0000'1000) >> 3;
        if (s) std::cout << static_cast<int>(GPR[rd]) << std::endl;
        PC++;
        break;
    }
    case 0b10: { // li
        uint8_t rd = (instr & 0b0011'0000) >> 4;
        uint8_t imm = instr & 0b0000'1111;
        GPR[rd] = imm;
        PC++;
        break;
    }
    case 0b11: { //bner0
        uint8_t addr = (instr & 0b0011'1100) >> 2;
        uint8_t rs2 = instr & 0b0000'0011;
        if (GPR[0] != GPR[rs2]) PC = addr;
        else PC++;
        break;
    }
    
    default:
        break;
    }
}

void init () {
    PC = 0;
    set_rom();
}

int main () {
    init();
    while (1) {
        inst_cycle();        
    }
}