import "DPI-C" function int pmem_read(input int addr);
import "DPI-C" function void pmem_write(
  input int waddr, input int wdata, input byte wmask
);

module pmem 
    import pkg::*;
(
    input logic             clk,
    input logic [7:0]       wmask,
    input logic             ren,
    input logic             wen,
    input logic [XLEN-1:0]  raddr,
    input logic [XLEN-1:0]  waddr,
    input logic [XLEN-1:0]  wdata,
    output logic [XLEN-1:0] rdata
);
    always_comb begin
        if (ren)
            rdata = pmem_read(raddr);
        else
            rdata = '0;
    end

    always_ff @(posedge clk) begin
        if (wen)
            pmem_write(waddr, wdata, wmask);
    end
endmodule
