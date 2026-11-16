
module alu (
    input  wire [31:0] a,
    input  wire [31:0] b,
    input  wire [3:0]  alu_op,
    output reg  [31:0] result,
    output wire        zero
);

    localparam ALU_ADD  = 4'b0000;
    localparam ALU_SUB  = 4'b0001;
    localparam ALU_AND  = 4'b0010;
    localparam ALU_OR   = 4'b0011;
    localparam ALU_XOR  = 4'b0100;
    localparam ALU_SLL  = 4'b0101;
    localparam ALU_SRL  = 4'b0110;
    localparam ALU_SRA  = 4'b0111;
    localparam ALU_SLT  = 4'b1000;
    localparam ALU_SLTU = 4'b1001;

    wire sub_en = (alu_op == ALU_SUB)  ||
                  (alu_op == ALU_SLT)  ||
                  (alu_op == ALU_SLTU);

    // sub_en = 0: a + b
    // sub_en = 1: a + ~b + 1 = a - b
    wire [31:0] b_mux = b ^ {32{sub_en}};

    wire [32:0] sum_ext =
        {1'b0, a} + {1'b0, b_mux} + sub_en;

    wire [31:0] sum = sum_ext[31:0];
    wire carry_out = sum_ext[32];

    wire overflow =
        (a[31] ^ b[31]) & (a[31] ^ sum[31]);

    wire signed_lt = sum[31] ^ overflow;

    wire unsigned_lt = ~carry_out;

    always @(*) begin
        case (alu_op)
            ALU_ADD:  result = sum;
            ALU_SUB:  result = sum;

            ALU_AND:  result = a & b;
            ALU_OR:   result = a | b;
            ALU_XOR:  result = a ^ b;

            ALU_SLL:  result = a << b[4:0];
            ALU_SRL:  result = a >> b[4:0];
            ALU_SRA:  result = $signed(a) >>> b[4:0];

            ALU_SLT:  result = {31'b0, signed_lt};
            ALU_SLTU: result = {31'b0, unsigned_lt};

            default:  result = 32'b0;
        endcase
    end

    assign zero = (result == 32'b0);

endmodule
