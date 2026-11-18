# Same program as the objdump listing saved as ../csrc/program.o
    .text
    .globl _start
_start:
    addi a0, zero, 20      # 0x00
    jalr ra, 16(zero)      # 0x04 -> fun
    jalr ra, 12(zero)      # 0x08 -> halt
halt:
    jalr zero, 12(zero)    # 0x0c: self-loop, used as "halt"
fun:
    addi a0, a0, 10        # 0x10
    jalr zero, 0(ra)       # 0x14 -> back to 0x08
