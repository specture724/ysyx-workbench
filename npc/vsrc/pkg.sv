package pkg;
    parameter int XLEN       = 32;
    parameter int REG_NUM    = 32;
    parameter int REG_ADDR_W = $clog2(REG_NUM);
    parameter logic [XLEN-1:0] RESET_PC = 32'h0000_0000;

    typedef enum logic [3:0] { ALU_NONE, ALU_ADD } alu_op_t;
    typedef enum logic [2:0] { ALU_SRC_NONE, ALU_SRC_RS1, ALU_SRC_RS2, ALU_SRC_IMM, ALU_SRC_PC } alu_src_t;
    typedef enum logic [2:0] { WB_NONE, WB_ALU, WB_PC, WB_MEM, WB_IMM } wb_sel_t;
    typedef enum logic [6:0] {
        OP_NONE = 7'h00,
        ALU_I   = 7'h13,
        JALR    = 7'h67,
        E       = 7'h73,
        LOAD    = 7'h03,
        STORE   = 7'h23,
        LUI     = 7'h37
    } opcode_t;
    typedef enum logic [2:0] { IMM_NONE, IMM_I, IMM_S, IMM_U } imm_t;
    typedef enum logic [1:0] { MEM_NONE, MEM_B, MEM_W } mem_size_t;

    typedef struct packed {
        logic                   valid;
        logic [REG_ADDR_W-1:0]  rs1_addr;
        logic [REG_ADDR_W-1:0]  rs2_addr;
        logic [XLEN-1:0]        imm;
        alu_op_t                alu_op;
        alu_src_t               alu_src_a;
        alu_src_t               alu_src_b;
        logic                   rf_wen;
        logic [REG_ADDR_W-1:0]  rd_addr;
        wb_sel_t                wb_sel;
        logic                   jump;
        logic                   ebreak;
        logic                   mem_re;
        logic                   mem_we;
        mem_size_t              mem_size;
        logic                   is_signed;
    } decode_t;

endpackage
