module seg(
  input clk,
  input rst,
  input [2:0] in,
  output reg [7:0] o_seg0
);

parameter [7:0] NUMS [7:0] = {
    8'b11000000,  // 7
    8'b11111001,  // 6
    8'b10100100,  // 5
    8'b10110000,  // 4
    8'b10011001,  // 3
    8'b10010010,  // 2
    8'b10000010,  // 1
    8'b11111000   // 0
};

always @(posedge clk) begin
  if(rst) begin o_seg0 <= 0; end
  else begin
    o_seg0 <= NUMS[7-in];
  end
end

endmodule
