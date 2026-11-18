#include <iostream>
#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <vector>
#include <stdexcept>

constexpr uint32_t GPR_SIZE = 32;   // 32 registers for RV32I
constexpr uint32_t ROM_SIZE = 4096; // 4 KiB instruction memory, byte addressed

uint32_t PC = 0;                            // byte address
std::vector<uint32_t> GPR(GPR_SIZE, 0x00);  // x0 is always 0
std::vector<uint8_t> ROM(ROM_SIZE, 0x00);   // byte addressable, little endian

enum class Instruction : uint8_t {
    // I-type ALU
    addi,
    // I-type 
    jalr,
};

struct Control {
    enum class ImmType : uint8_t { I } imm_type;
    enum class AluOp : uint8_t { ADD } alu_op;
    enum class WbSel : uint8_t { ALU, PC4 } wb_sel;
    bool reg_we;
    bool alu_a_from_pc;
    bool alu_b_from_imm;
    bool jump; 
    bool illegal;
};


Control decode (uint32_t instr) {
    Control ctl{};
    ctl.illegal = false;
    uint32_t opcode = instr & 0x7F;
    uint32_t funct3 = (instr >> 12) & 0x7;

    switch (opcode) {
        case 0x13: {
            ctl.imm_type = Control::ImmType::I;
            ctl.wb_sel = Control::WbSel::ALU;
            ctl.reg_we = true;
            ctl.alu_a_from_pc = false;
            ctl.alu_b_from_imm = true;
            ctl.jump = false;
            switch (funct3) {
                case 0x0: ctl.alu_op = Control::AluOp::ADD; break;
                default: ctl.illegal = true;
            }
            break;
        }
        case 0x67: {
            if (funct3 != 0x0) { ctl.illegal = true; break; }
            ctl.imm_type = Control::ImmType::I;
            ctl.wb_sel = Control::WbSel::PC4;
            ctl.reg_we = true;
            ctl.alu_a_from_pc = false;
            ctl.alu_b_from_imm = true;
            ctl.jump = true;
            ctl.alu_op = Control::AluOp::ADD;
            break;
        }
        default: ctl.illegal = true;
    }

    return ctl;
}


int32_t imm_gen(uint32_t instr, Control::ImmType t) {
    switch (t) {
        case Control::ImmType::I:
            return static_cast<int32_t>(instr) >> 20;
        default: return 0;
    }
}

uint32_t alu(Control::AluOp alu_op, uint32_t a, uint32_t b) {
    uint32_t output;
    switch (alu_op) {
    case Control::AluOp::ADD:
        output = a + b;
        break;
    
    default:
        throw std::logic_error("unknown alu op");
    }

    return output;
}

// PC is a byte address; an instruction is 4 bytes, little endian.
uint32_t fetch (uint32_t pc) {
    if ((pc & 0x3u) != 0 || pc > ROM_SIZE - 4) {
        throw std::runtime_error(
            "Fetch fault at pc=0x" + std::to_string(pc));
    }
    return  static_cast<uint32_t>(ROM[pc])
         | (static_cast<uint32_t>(ROM[pc + 1]) << 8)
         | (static_cast<uint32_t>(ROM[pc + 2]) << 16)
         | (static_cast<uint32_t>(ROM[pc + 3]) << 24);
}

void ref_inst_cycle () {
    uint32_t instr = fetch(PC);

    // decode
    Control ctl = decode(instr);
    if (ctl.illegal) {
        std::string msg = "Invalid instruction " + std::to_string(instr);
        throw std::runtime_error(msg);
    }

    uint32_t rd  = (instr >> 7)  & 0x1F;
    uint32_t rs1 = (instr >> 15) & 0x1F;
    uint32_t rs2 = (instr >> 20) & 0x1F;

    uint32_t a = ctl.alu_a_from_pc ? PC : GPR[rs1];
    int32_t  imm = imm_gen(instr, ctl.imm_type);
    uint32_t b = ctl.alu_b_from_imm ? static_cast<uint32_t>(imm) : GPR[rs2];

    uint32_t alu_out = alu(ctl.alu_op, a, b);

    u_int32_t wb_data = 0;
    switch (ctl.wb_sel) {
        case Control::WbSel::ALU: wb_data = alu_out; break;
        case Control::WbSel::PC4: wb_data = PC + 4; break;
        default: throw std::logic_error("unknown write back source");
    }
    if (ctl.reg_we && rd != 0) GPR[rd] = wb_data; // rd != 0 means that x0 will always be 0

    uint32_t next_pc = PC + 4;
    if (ctl.jump) 
        next_pc  = alu_out & ~1u;
    PC = next_pc;
}

void ref_init () {
    PC = 0;
    GPR.assign(GPR_SIZE, 0);
}

void ref_set_pc (uint32_t pc) { PC = pc; }

// loader / debug interface
void ref_load_rom (const uint8_t *data, std::size_t size, uint32_t base = 0) {
    if (size == 0) return;
    if (base > ROM_SIZE || size > static_cast<std::size_t>(ROM_SIZE - base)) {
        throw std::runtime_error(
            "ROM overflow: " + std::to_string(size) + " bytes at 0x" +
            std::to_string(base) + ", ROM size is " + std::to_string(ROM_SIZE));
    }
    std::copy(data, data + size, ROM.begin() + base);
}

const uint8_t *ref_get_rom () { return ROM.data(); }
uint32_t ref_rom_size () { return ROM_SIZE; }

// difftest interface: expose reference model state
uint32_t *ref_get_regs () { return GPR.data(); }
uint32_t ref_get_pc () { return PC; }

// int main () {
//     init();
//     while (1) {
//         inst_cycle();        
//     }
// }