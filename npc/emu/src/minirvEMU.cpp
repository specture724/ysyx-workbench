#include "minirvEMU.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

constexpr uint32_t GPR_SIZE = 32;   // 32 registers for RV32I
constexpr uint32_t RAM_SIZE = 4096; // 4 KiB unified memory (code + data), byte addressed

uint32_t PC = 0;                            // byte address
std::vector<uint32_t> GPR(GPR_SIZE, 0x00);  // x0 is always 0
std::vector<uint8_t> RAM(RAM_SIZE, 0x00);   // byte addressable, little endian

enum class Instruction : uint8_t {
    // I-type ALU
    ADDI,
    // I-type 
    JALR,
    // R-type ALU
    ADD,
    // U-type
    LUI,
};

Instruction decode (uint32_t instr) {
    uint32_t opcode = instr & 0x7F;
    uint32_t funct3 = (instr >> 12) & 0x7;
    uint32_t funct7 = (instr >> 25) & 0x7F;

    switch (opcode) {
        case 0x13: {
            if (funct3 == 0x0) return Instruction::ADD;
            else throw std::runtime_error("Invalid Instruction.");
        }

        case 0x67: {
            if (funct3 != 0x0) { throw std::runtime_error("Invalid Instruction."); }
            return Instruction::JALR;
        }

        case 0x37: return Instruction::LUI;

        case 0x33: {
            if (funct3 == 0x0 && funct7 == 0x0) return Instruction::ADD;
            else throw std::runtime_error("Invalid Instruction.");
        }

        default: throw std::runtime_error("Instruction not supported.");
    }
}

// PC is a byte address; code and data live in the same RAM.
// An instruction is 4 bytes, little endian.
uint32_t fetch (uint32_t pc) {
    if ((pc & 0x3u) != 0 || pc > RAM_SIZE - 4) {
        throw std::runtime_error(
            "Fetch fault at pc=0x" + std::to_string(pc));
    }
    return  static_cast<uint32_t>(RAM[pc])
         | (static_cast<uint32_t>(RAM[pc + 1]) << 8)
         | (static_cast<uint32_t>(RAM[pc + 2]) << 16)
         | (static_cast<uint32_t>(RAM[pc + 3]) << 24);
}

void ref_inst_cycle () {
    uint32_t instr = fetch(PC);

    // decode
    Instruction Instruction_type = decode(instr);

    // execute
    switch (Instruction_type) {
        case Instruction::ADDI: {
            uint8_t rs1 = (instr >> 15) & 0x1F;
            uint8_t rd = (instr >> 7) & 0x1F;
            uint32_t imm = instr >> 20;
            if (rd != 0)
                GPR[rd] = GPR[rs1] + imm;
            PC = PC + 4;
            break;
        }

        case Instruction::JALR: {
            uint8_t rs1 = (instr >> 15) & 0x1F;
            uint8_t rd = (instr >> 7) & 0x1F;
            uint32_t imm = instr >> 20;
            uint32_t target = GPR[rs1] + imm;
            if (rd != 0)
                GPR[rd] = PC + 4;
            PC = target;
            break;
        }

        case Instruction::LUI: {
            uint8_t rd = (instr >> 7) & 0x1F;
            uint32_t imm = instr & 0xFFFFF000;
            if (rd != 0)
                GPR[rd] = imm;
            PC = PC + 4;
            break;
        }

        case Instruction::ADD: {
            uint8_t rs1 = (instr >> 15) & 0x1F;
            uint8_t rd = (instr >> 7) & 0x1F;
            uint32_t rs2 = (instr >> 20) & 0x1F;
            if (rd != 0)
                GPR[rd] = GPR[rs1] + GPR[rs2];
            PC = PC + 4;
            break;
        }
        
        default:
            throw std::runtime_error("Should not reach here.");
    }

}

void ref_init () {
    PC = 0;
    GPR.assign(GPR_SIZE, 0);
}

void ref_set_pc (uint32_t pc) { PC = pc; }

// loader / debug interface
void ref_load_ram (const uint8_t *data, std::size_t size, uint32_t base) {
    if (size == 0) return;
    if (base > RAM_SIZE || size > static_cast<std::size_t>(RAM_SIZE - base)) {
        throw std::runtime_error(
            "RAM overflow: " + std::to_string(size) + " bytes at 0x" +
            std::to_string(base) + ", RAM size is " + std::to_string(RAM_SIZE));
    }
    std::copy(data, data + size, RAM.begin() + base);
}

const uint8_t *ref_get_ram () { return RAM.data(); }
uint32_t ref_ram_size () { return RAM_SIZE; }

// difftest interface: expose reference model state
uint32_t *ref_get_regs () { return GPR.data(); }
uint32_t ref_get_pc () { return PC; }
