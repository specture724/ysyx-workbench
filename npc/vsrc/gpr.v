module gpr (
    input clk, 
    input rst,
    input wen,
    input [1:0] raddr1,
    input [1:0] raddr2,
    input [1:0] waddr,
    input [7:0] wdata,
    output [7:0] rdata1,
    output [7:0] rdata2
);
    wire [7:0] r0, r1, r2, r3;

    wire en0 = wen & (waddr == 2'b00);
    wire en1 = wen & (waddr == 2'b01);
    wire en2 = wen & (waddr == 2'b10);
    wire en3 = wen & (waddr == 2'b11);

    reg8 u_r0(.clk(clk), .rst(rst), .en(en0), .d(wdata), .q(r0));
    reg8 u_r1(.clk(clk), .rst(rst), .en(en1), .d(wdata), .q(r1));
    reg8 u_r2(.clk(clk), .rst(rst), .en(en2), .d(wdata), .q(r2));
    reg8 u_r3(.clk(clk), .rst(rst), .en(en3), .d(wdata), .q(r3));

    assign rdata1 = (raddr1 == 2'd0) ? r0 :
                    (raddr1 == 2'd1) ? r1 :
                    (raddr1 == 2'd2) ? r2 : r3;

    assign rdata2 = (raddr2 == 2'd0) ? r0 :
                    (raddr2 == 2'd1) ? r1 :
                    (raddr2 == 2'd2) ? r2 : r3;
endmodule