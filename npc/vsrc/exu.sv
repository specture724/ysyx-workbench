module exu
    import pkg::*;
(
    input   logic [XLEN-1:0] a,
    input   logic [XLEN-1:0] b,
    input   pkg::alu_op_t    alu_op,
    output  logic [XLEN-1:0] out
);
    always_comb begin : ALU
        case (alu_op)
            ALU_ADD: out = a + b;
            default: out = '0;
        endcase        
    end
endmodule
