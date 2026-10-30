module encoder83(
    input  [7:0] x,
    input  en,
    output reg [2:0]y,
    output haz_out
);
  integer i;
  always @(*) begin
    if (en) begin
        y = 0;
        for(i = 0; i <= 7; i = i + 1)
            if(x[i] == 1)  y = i[2:0];
        if (y == 3'b000) haz_out = 0;
        else haz_out = 1;
    end
    else begin
        y = 0;
        haz_out = 0;
    end
  end
endmodule