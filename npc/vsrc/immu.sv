module immu 
    import pkg::*;
(
    input  logic [XLEN-1:0] instr,
    input  pkg::imm_t       imm_type,
    output logic [XLEN-1:0] imm
);
    always_comb begin : imm_generator
        case (imm_type)
            IMM_I: imm = {{20{instr[31]}}, instr[31:20]};
            IMM_S: imm = {{20{instr[31]}}, instr[31:25], instr[11:7]};
            IMM_U: imm = {instr[31:12], {12{'0}}};
            default: imm = '0;
        endcase
    end
endmodule
