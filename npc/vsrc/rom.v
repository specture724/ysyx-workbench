module rom (
    input [7:0] addr,
    output reg [7:0] data
);
    always @(*) begin
        case (addr)
            8'd0: data = 8'b1000_1010; // li  r0, 10  (opcode=10, rd=00, imm=1010)
            8'd1: data = 8'b1001_0000; // li  r1, 0
            8'd2: data = 8'b1010_0000; // li  r2, 0
            8'd3: data = 8'b1011_0001; // li  r3, 1
            8'd4: data = 8'b0001_0111; // add r1, r1, r3  (00, rd=01, rs1=01, rs2=11)
            8'd5: data = 8'b0010_1001; // add r2, r2, r1  (00, rd=10, rs1=10, rs2=01)
            8'd6: data = 8'b1101_0001; // bner0 r1, 4    (11, addr=0100, rs2=01)
            8'd7: data = 8'b1101_1111; // bner0 r3, 7    (11, addr=0111, rs2=11)
            // 8'd8: data = 8'b0110_1000; // io r2, 0
            default: data = 8'b0000_0000;
        endcase
    end
endmodule