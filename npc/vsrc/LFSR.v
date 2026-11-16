module LFSR (
    input clk,
    input rst,
    input [7:0] set,
    output reg [7:0] out
);
    reg [7:0] seq;
    reg x8;
    always @(clk) begin
        if (rst) seq <= set;
        else begin
            x8 <= seq[4] ^ seq[3] ^ seq[2] ^ seq[0];
            seq <= {x8, seq[7: 1]};
        end
    end
    assign out = seq;
endmodule