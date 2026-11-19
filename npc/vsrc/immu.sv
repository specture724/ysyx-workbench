module immu 
    import pkg::*;
(
    input  logic [XLEN-1:0] instr,
    input  pkg::imm_t       imm_type,
    output logic [XLEN-1:0] imm
);
    always_comb begin : imm_generator
        case (imm_type)
            IMM_I: begin
                // I 型立即数需要符号扩展
                imm = {{20{instr[31]}}, instr[31:20]};
            end 
            default: imm = '0;
        endcase
    end
endmodule
