# Exercises the instructions currently implemented by minirvEMU:
#   ADDI, ADD, LUI, LW, SW, JALR  + the "write to x0 is ignored" rule.
#
# Expected final state:
#   x0  = 0x00000000
#   a0  = 0x00000005
#   a1  = 0xfffffffd   (-3)
#   a2  = 0x00000002
#   a3  = 0x12345000
#   a4  = 0x00000005   (loaded from 0x200)
#   a5  = 0x00000002   (loaded from 0x204)
#   a6  = 0x00000007   (set after the call returns)
#   a7  = 0x0000002a   (42, set inside func)
#   t0  = 0x00000200
#   ra  = 0x0000002c   (return address of the jalr call)
#   pc  = 0x00000034   (self-loop used as halt)
#   RAM[0x200..0x203] = 05 00 00 00
#   RAM[0x204..0x207] = 02 00 00 00
#
# Run it with:
#   npc/build/emu --bin npc/build/isa_test.bin --trace ...

    .text
    .globl _start
_start:
    addi a0, zero, 5          # 0x00: a0 = 5
    addi zero, zero, 5        # 0x04: writing x0 must be ignored
    addi a1, zero, -3         # 0x08: negative immediate, a1 = 0xfffffffd
    add  a2, a0, a1           # 0x0c: a2 = 2
    lui  a3, 0x12345          # 0x10: a3 = 0x12345000

    addi t0, zero, 0x200      # 0x14: t0 = data base
    sw   a0, 0(t0)            # 0x18: RAM[0x200] = 5
    lw   a4, 0(t0)            # 0x1c: a4 = 5
    sw   a2, 4(t0)            # 0x20: RAM[0x204] = 2
    lw   a5, 4(t0)            # 0x24: a5 = 2

    jalr ra, 56(zero)         # 0x28: call func at 0x38, ra = 0x2c
    addi a6, zero, 7          # 0x2c: executed after the call returns
    jalr zero, 52(zero)       # 0x30: jump to halt at 0x34
halt:
    jalr zero, 52(zero)       # 0x34: self-loop, halt
func:
    addi a7, zero, 42         # 0x38: a7 = 42
    jalr zero, 0(ra)          # 0x3c: return to ra (0x2c)
