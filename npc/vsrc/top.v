module top(
    input clk,
    input rst,
    input  [7:0] x,
    input  en,
    output reg [2:0]y,
    output haz_out
);

    encoder83 my_encoder(.x(x), .en(en), .y(y), .haz_out(haz_out));
endmodule
