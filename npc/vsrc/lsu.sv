module lsu
    import pkg::*;
(
    input  pkg::decode_t    dec,
    input  logic [XLEN-1:0] raddr,
    input  logic [XLEN-1:0] mem_data,
    input  logic [XLEN-1:0] store_data,
    output logic [XLEN-1:0] wdata,
    output logic [XLEN-1:0] rdata,
    output logic [7:0]      wmask
);
    // silent misalign
    // read
    logic [7:0]  byte_sel;
    always_comb begin
        byte_sel = mem_data[8*raddr[1:0] +: 8];
        unique case (dec.mem_size)
            MEM_B:  rdata = dec.is_signed ? {{XLEN-8{byte_sel[7]}},  byte_sel}
                                          : {{XLEN-8{1'b0}},        byte_sel};
            MEM_W:  rdata = mem_data;
            default: rdata = '0;
        endcase
    end

    // write
    always_comb begin
        unique case (dec.mem_size)
            MEM_B: begin
                wmask = 8'h01 << raddr[1:0];
                wdata = {4{store_data[7:0]}};
            end
            MEM_W: begin
                wmask = 8'h0f;
                wdata = store_data;
            end
            default: begin
                wmask = 8'h00;
                wdata = '0;
            end
        endcase
    end
endmodule
