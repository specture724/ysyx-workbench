module full_adder (
    input a,
    input b,
    input cin,
    output reg s,
    output reg cout
);
    reg [1:0] sum;
    always @(*) begin
        sum = a + b + cin;
        s = sum[0];
        cout = sum[1];
    end
endmodule