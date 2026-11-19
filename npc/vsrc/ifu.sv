module ifu 
    import pkg::*;
(
    input logic [XLEN-1:0]  pc,
    input  logic [XLEN-1:0] imem_rdata,
    output logic [XLEN-1:0] inst,
    output logic [XLEN-1:0] imem_addr
);
    assign imem_addr = pc;
    assign inst = imem_rdata;
endmodule
