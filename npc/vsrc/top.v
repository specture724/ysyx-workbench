module top(
    input clk,
    input rst,
    output [15:0] ledr
);

led my_led(
    .clk(clk),
    .rst(rst),
    .ledr(ledr)
);

endmodule
