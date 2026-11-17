module top(
    input clk,
    input rst,
    input  [7:0] in0, in1, in2, in3, in4, in5, in6, in7,
    output [7:0] out0, out1, out2, out3, out4, out5, out6, out7
);

    wire [7:0] pc;
    wire [7:0] instr;
    wire [7:0] gpr_rdata1, gpr_rdata2;
    wire [7:0] alu_result;
    wire [7:0] pc_next;
    wire [7:0] gpr_wdata;
    wire [1:0] gpr_waddr;
    wire       gpr_wen;
    wire       is_add, is_li, is_bner0, is_io;
    wire [7:0] imm;
    wire [7:0] in_arr  [0:7];
    reg  [7:0] out_arr [0:7];
    wire       io_rd;
    wire [2:0] io_dev;
    wire       is_output;

    assign in_arr[0] = in0;
    assign in_arr[1] = in1;
    assign in_arr[2] = in2;
    assign in_arr[3] = in3;
    assign in_arr[4] = in4;
    assign in_arr[5] = in5;
    assign in_arr[6] = in6;
    assign in_arr[7] = in7;

    assign out0 = out_arr[0];
    assign out1 = out_arr[1];
    assign out2 = out_arr[2];
    assign out3 = out_arr[3];
    assign out4 = out_arr[4];
    assign out5 = out_arr[5];
    assign out6 = out_arr[6];
    assign out7 = out_arr[7];

    pc u_pc(.clk(clk), .rst(rst), .pc_next(pc_next), .pc_out(pc));
    rom u_rom(.addr(pc), .data(instr));

    assign is_add = (instr[7:6] == 2'b00);
    assign is_io = (instr[7:6] == 2'b01);
    assign is_li    = (instr[7:6] == 2'b10);
    assign is_bner0 = (instr[7:6] == 2'b11);
    assign imm = {4'b0000, instr[3:0]};
    assign is_output = instr[3]; // 0 for input, 1 for output
    assign io_dev = instr[2:0];

    wire [1:0] raddr1 = is_bner0 ? 2'b00 : 
                        is_add ? instr[3:2] :
                        instr[5:4];
    wire [1:0] raddr2 = instr[1:0];
    assign gpr_waddr = instr[5:4];

    assign gpr_wen = is_add | is_li | (is_io & !is_output);
    assign gpr_wdata = is_li ? imm : 
                       is_add ? alu_result :
                       in_arr[io_dev];

    gpr u_gpr(
        .clk(clk), 
        .rst(rst),
        .wen(gpr_wen),
        .raddr1(raddr1),
        .raddr2(raddr2),
        .waddr(gpr_waddr),
        .wdata(gpr_wdata),
        .rdata1(gpr_rdata1),
        .rdata2(gpr_rdata2)
    );

    assign alu_result = gpr_rdata1 + gpr_rdata2;
    wire cmp_neq = (gpr_rdata1 != gpr_rdata2);

    wire [7:0] jump_addr = {4'b0000, instr[5:2]};
    assign pc_next = is_bner0 & cmp_neq ? jump_addr : (pc + 8'd1);
    integer i;
    always @(posedge clk) begin
        if (rst) begin
            for (i = 0; i < 8; i = i + 1)
                out_arr[i] <= 8'h00;
        end else if (is_io && is_output) begin
            out_arr[io_dev] <= gpr_rdata1;
        end
    end
endmodule
