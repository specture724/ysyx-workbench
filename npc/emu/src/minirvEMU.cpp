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
    // load
    LW, LBU,
    // store 
    SW, SB,
    // special
    EBREAK,
};

Instruction decode (uint32_t instr) {
    if (instr == 0x00100073) return Instruction::EBREAK;

    uint32_t opcode = instr & 0x7F;
    uint32_t funct3 = (instr >> 12) & 0x7;
    uint32_t funct7 = (instr >> 25) & 0x7F;

    switch (opcode) {
        case 0x13: {
            if (funct3 == 0x0) return Instruction::ADDI;
            else throw std::runtime_error("Invalid Instruction.");
        }

        case 0x67: {
            if (funct3 == 0x0) return Instruction::JALR;
            else throw std::runtime_error("Invalid Instruction."); 
        }

        case 0x37: return Instruction::LUI;

        case 0x33: {
            if (funct3 == 0x0 && funct7 == 0x0) return Instruction::ADD;
            else throw std::runtime_error("Invalid Instruction.");
        }

        case 0x03: {
            if (funct3 == 0x2) return Instruction::LW;
            else if (funct3 == 0x4) return Instruction::LBU;
            else throw std::runtime_error("Invalid Instruction.");
        }

        case 0x23: {
            if (funct3 == 0x2) return Instruction::SW;
            else if (funct3 == 0x0) return Instruction::SB;
            else throw std::runtime_error("Invalid Instruction.");
        }

        default: throw std::runtime_error("Instruction not supported.");
    }
}

// PC is a byte address; code and data live in the same RAM.
// An instruction is 4 bytes, little endian.
uint32_t fetch_word (uint32_t addr) {
    if ((addr % 4) != 0 || addr > RAM_SIZE - 4) {
        throw std::runtime_error(
            "Fetch fault at addr=0x" + std::to_string(addr));
    }
    return  static_cast<uint32_t>(RAM[addr])
         | (static_cast<uint32_t>(RAM[addr + 1]) << 8)
         | (static_cast<uint32_t>(RAM[addr + 2]) << 16)
         | (static_cast<uint32_t>(RAM[addr + 3]) << 24);
}

void write_word(uint32_t addr, uint32_t word) {
    if (addr % 4 != 0 || addr > RAM_SIZE - 4) {
        throw std::runtime_error(
            "Write fault at addr=0x" + std::to_string(addr));
    }
    RAM[addr]     = static_cast<uint8_t>(word      );
    RAM[addr + 1] = static_cast<uint8_t>(word >>  8);
    RAM[addr + 2] = static_cast<uint8_t>(word >> 16);
    RAM[addr + 3] = static_cast<uint8_t>(word >> 24);
}


uint8_t ref_inst_cycle () {
    uint32_t instr = fetch_word(PC);

    // decode
    Instruction Instruction_type = decode(instr);

    // execute
    switch (Instruction_type) {
        case Instruction::ADDI: {
            uint8_t rs1 = (instr >> 15) & 0x1F;
            uint8_t rd = (instr >> 7) & 0x1F;
            uint32_t imm = static_cast<int32_t>(instr) >> 20;
            if (rd != 0)
                GPR[rd] = GPR[rs1] + imm;
            PC = PC + 4;
            break;
        }

        case Instruction::JALR: {
            uint8_t rs1 = (instr >> 15) & 0x1F;
            uint8_t rd = (instr >> 7) & 0x1F;
            uint32_t imm = static_cast<int32_t>(instr) >> 20;
            uint32_t target = GPR[rs1] + imm;
            target = target & ~1;
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
            uint8_t rs2 = (instr >> 20) & 0x1F;
            if (rd != 0)
                GPR[rd] = GPR[rs1] + GPR[rs2];
            PC = PC + 4;
            break;
        }

        case Instruction::LW: {
            uint8_t rs1 = (instr >> 15) & 0x1F;
            uint8_t rd = (instr >> 7) & 0x1F;
            uint32_t imm = static_cast<int32_t>(instr) >> 20;
            uint32_t addr = GPR[rs1] + imm;
            if (rd != 0)
                GPR[rd] = fetch_word(addr);
            PC = PC + 4;
            break;
        }

        case Instruction::LBU: {
            uint8_t rs1 = (instr >> 15) & 0x1F;
            uint8_t rd = (instr >> 7) & 0x1F;
            uint32_t imm = static_cast<int32_t>(instr) >> 20;
            uint32_t addr = GPR[rs1] + imm;
            if (rd != 0) {
                if (addr > RAM_SIZE - 1) {
                    throw std::runtime_error(
                        "Fetch fault at addr=0x" + std::to_string(addr));
                }
                GPR[rd] = static_cast<uint32_t>(RAM[addr]);
            }
            PC = PC + 4;
            break;
        }

        case Instruction::SW: {
            uint8_t rs1 = (instr >> 15) & 0x1F;
            uint8_t rs2 = (instr >> 20) & 0x1F;
            uint32_t imm = ((static_cast<int32_t>(instr) >> 20) & (~0x1F) ) + ((instr >> 7) & 0x1F);
            uint32_t addr = GPR[rs1] + imm;
            write_word(addr, GPR[rs2]);
            PC = PC + 4;
            break;
        }

        case Instruction::SB: {
            uint8_t rs1 = (instr >> 15) & 0x1F;
            uint8_t rs2 = (instr >> 20) & 0x1F;
            uint32_t imm = ((static_cast<int32_t>(instr) >> 20) & (~0x1F) ) + ((instr >> 7) & 0x1F);
            uint32_t addr = GPR[rs1] + imm;
            if (addr > RAM_SIZE - 1) {
                throw std::runtime_error(
                    "Write fault at addr=0x" + std::to_string(addr));
            }
            RAM[addr] = static_cast<uint8_t>(GPR[rs2]);
            PC = PC + 4;
            break;
        }

        case Instruction::EBREAK:
            return 1;

        default:
            throw std::runtime_error("Should not reach here.");
    }
    return 0;
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
