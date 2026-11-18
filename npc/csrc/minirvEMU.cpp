#include <iostream>
#include <stdint.h>
#include <vector>
#include <stdexcept>

const u_int32_t GPR_SIZE = 16; // 16 registers for RV32E
const u_int32_t ROM_SIZE = 256; // 16 registers for RV32E

uint32_t PC = 0;
std::vector<uint32_t> GPR(GPR_SIZE, 0x00); // x0 is always 0
std::vector<uint32_t> ROM(ROM_SIZE, 0x00);

enum class Instruction : uint8_t {
    // I-type ALU
    addi,
};

struct Control {
    enum class ImmType : uint8_t { I } imm_type;
    enum class AluOp : uint8_t { ADD } alu_op;
    enum class WbSel : uint8_t { ALU } wb_sel;
    bool reg_we;
    bool alu_a_from_pc;
    bool alu_b_from_imm;
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
            switch (funct3) {
                case 0x0: ctl.alu_op = Control::AluOp::ADD; break;
                default: ctl.illegal = true;
            }
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
        break;
    }

    return output;
}

void ref_inst_cycle () {
    uint32_t instr = ROM[PC];

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
    }
    if (ctl.reg_we && rd != 0) GPR[rd] = wb_data; // rd != 0 means that x0 will always be 0

    uint32_t next_pc = PC + 4;
    PC = next_pc;
}

void ref_init () {
    PC = 0;
}

// difftest interface: expose reference model state
uint32_t *ref_get_regs () { return GPR.data(); }
uint32_t ref_get_pc () { return PC; }

// int main () {
//     init();
//     while (1) {
//         inst_cycle();        
//     }
// }