module wbu
  import pkg::*;
(
    input  pkg::decode_t          dec,
    input  logic [XLEN-1:0]       result,
    output logic                  rf_wen,
    output logic [REG_ADDR_W-1:0] rf_waddr,
    output logic [XLEN-1:0]       rf_wdata
);
    assign rf_wen   = dec.valid && dec.rf_wen &&
                      (dec.rd_addr != {REG_ADDR_W{1'b0}});
    assign rf_waddr = dec.rd_addr;
    assign rf_wdata = result;
endmodule
