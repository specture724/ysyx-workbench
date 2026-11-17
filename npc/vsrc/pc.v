module pc (
    input clk,
    input rst,
    input [7:0] pc_next,
    output [7:0] pc_out
);
    reg8 u_pc_reg(
        .clk(clk),
        .rst(rst),
        .en(1'b1),
        .d(pc_next),
        .q(pc_out)
    );
endmodule