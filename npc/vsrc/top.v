module top(
    input clk,
    input rst,
    input  [7:0] x,
    input  en,
    output reg [2:0]y,
    output reg [7:0] o_seg0,
    output haz_out
);

    encoder83 my_encoder(.x(x), .en(en), .y(y), .haz_out(haz_out));
    seg my_seg(.clk(clk),
        .rst(rst),
        .in(y),
        .o_seg0(o_seg0)
    );
endmodule
