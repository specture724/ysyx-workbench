module idu 
    import pkg::*;
(
    input logic [XLEN-1:0] instr,
    output pkg::decode_t   dec_out
);
    logic [6:0] opcode;
    logic [2:0] funct3;
    logic [4:0] rd;
    logic [4:0] rs1;
    logic [4:0] rs2;

    assign opcode = instr[6:0];
    assign funct3 = instr[14:12];
    assign rd     = instr[11:7];
    assign rs1    = instr[19:15];
    assign rs2    = instr[24:20];

    pkg::imm_t       imm_type;
    logic [XLEN-1:0] imm;

    immu u_immu(
        .instr    (instr),
        .imm_type (imm_type),
        .imm      (imm)
    );

    always_comb begin : decode
        dec_out   = '0;
        imm_type  = NONE;

        dec_out.rs1_addr = rs1;
        dec_out.rs2_addr = rs2;
        dec_out.rd_addr  = rd;
        dec_out.imm      = imm;

        case (pkg::opcode_t'(opcode))
            ALU_I: begin
                dec_out.alu_src_a = ALU_SRC_RS1;
                dec_out.alu_src_b = ALU_SRC_IMM;
                dec_out.wb_sel    = WB_ALU;
                case (funct3)
                    3'b000: begin // ADDI
                        dec_out.alu_op = ALU_ADD;
                        dec_out.rf_wen = 1'b1;
                        dec_out.valid  = 1'b1;
                        imm_type       = IMM_I;
                    end 
                    default: ;
                endcase
            end 

            JALR: begin // JALR
                if (funct3 == 3'b000) begin
                    dec_out.jump        = 1'b1;
                    dec_out.alu_src_a   = ALU_SRC_RS1;
                    dec_out.alu_src_b   = ALU_SRC_IMM;
                    dec_out.alu_op      = ALU_ADD;
                    dec_out.wb_sel      = WB_PC;
                    imm_type            = IMM_I;
                    dec_out.rf_wen      = 1'b1;
                    dec_out.valid       = 1'b1;
                end
            end

            E: begin
                if (instr == 32'h00100073) dec_out.ebreak = 1'b1;
            end

            default: ;
        endcase
    end
endmodule
