module top
    import pkg::*;
(
    input  logic             clk,
    input  logic             rst,
    output logic [XLEN-1:0]  pc,
    input  logic [XLEN-1:0]  inst
);
    logic [XLEN-1:0] pc_reg;
    logic [XLEN-1:0] pc_next;

    always_ff @(posedge clk) begin
        if (rst) pc_reg <= RESET_PC;
        else     pc_reg <= pc_next;
    end

    logic [XLEN-1:0] inst_if;
    logic [XLEN-1:0] imem_addr;

    ifu u_ifu (
        .pc         (pc_reg),
        .imem_rdata (inst),
        .inst       (inst_if),
        .imem_addr  (imem_addr)
    );

    assign pc = imem_addr;

    pkg::decode_t dec;

    idu u_idu (
        .instr   (inst_if),
        .dec_out (dec)
    );

    logic [XLEN-1:0]       rs1_data;
    logic [XLEN-1:0]       rs2_data;
    logic                  rf_wen;
    logic [REG_ADDR_W-1:0] rf_waddr;
    logic [XLEN-1:0]       rf_wdata;

    RegisterFile u_rf (
        .clk    (clk),
        .rst    (rst),
        .wdata  (rf_wdata),
        .waddr  (rf_waddr),
        .wen    (rf_wen),
        .raddr1 (dec.rs1_addr),
        .raddr2 (dec.rs2_addr),
        .rdata1 (rs1_data),
        .rdata2 (rs2_data)
    );

    logic [XLEN-1:0] alu_a;
    logic [XLEN-1:0] alu_b;
    logic [XLEN-1:0] alu_out;

    always_comb begin : alu_src_mux
        unique case (dec.alu_src_a)
            ALU_SRC_RS1: alu_a = rs1_data;
            ALU_SRC_RS2: alu_a = rs2_data;
            ALU_SRC_IMM: alu_a = dec.imm;
            ALU_SRC_PC:  alu_a = pc_reg;
            default:     alu_a = '0;
        endcase
        unique case (dec.alu_src_b)
            ALU_SRC_RS1: alu_b = rs1_data;
            ALU_SRC_RS2: alu_b = rs2_data;
            ALU_SRC_IMM: alu_b = dec.imm;
            ALU_SRC_PC:  alu_b = pc_reg;
            default:     alu_b = '0;
        endcase
    end

    exu u_exu (
        .a      (alu_a),
        .b      (alu_b),
        .alu_op (dec.alu_op),
        .out    (alu_out)
    );

    logic [XLEN-1:0] jalr_target;
    assign jalr_target = {alu_out[XLEN-1:1], 1'b0};

    always_comb begin : Update_PC
        if (dec.jump)
            pc_next = jalr_target;
        else
            pc_next = pc_reg + 32'd4;
    end

    logic [XLEN-1:0] wb_data;

    always_comb begin : wb_mux
        unique case (dec.wb_sel)
            WB_ALU:     wb_data = alu_out;
            WB_PC:      wb_data = pc_reg + 32'd4;
            default:    wb_data = '0;
        endcase
    end

    wbu u_wbu (
        .dec      (dec),
        .result   (wb_data),
        .rf_wen   (rf_wen),
        .rf_waddr (rf_waddr),
        .rf_wdata (rf_wdata)
    );

endmodule
