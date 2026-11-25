module RegisterFile
    import pkg::*;
(
  input  logic                  clk,
  input  logic                  rst,
  input  logic [XLEN-1:0]       wdata,
  input  logic [REG_ADDR_W-1:0] waddr,
  input  logic                  wen,
  input  logic [REG_ADDR_W-1:0] raddr1,
  input  logic [REG_ADDR_W-1:0] raddr2,
  output logic [XLEN-1:0]       rdata1,
  output logic [XLEN-1:0]       rdata2
);
    logic [XLEN-1:0] rf [REG_NUM-1:0];

    always_ff @(posedge clk) begin
      if (rst) begin
        for (int i = 0; i < REG_NUM; i++)
          rf[i] <= '0;
      end else if (wen && (waddr != {REG_ADDR_W{1'b0}})) begin
        rf[waddr] <= wdata;
      end
    end

    // x0 恒为 0；两个读端口组合读
    assign rdata1 = (raddr1 == {REG_ADDR_W{1'b0}}) ? {XLEN{1'b0}} : rf[raddr1];
    assign rdata2 = (raddr2 == {REG_ADDR_W{1'b0}}) ? {XLEN{1'b0}} : rf[raddr2];
endmodule
