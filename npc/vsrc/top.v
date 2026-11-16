module top(
    input clk,
    input rst,
    input [7:0] set,
    output reg [7:0] out
);
    LFSR my_lfsr(clk, rst, set, out);
endmodule
