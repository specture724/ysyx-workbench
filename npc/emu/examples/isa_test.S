# Exercises the instructions currently implemented by minirvEMU:
#   ADDI, ADD, LUI, LW, SW, LBU, SB, JALR, EBREAK  + the "write to x0 is ignored" rule.
#
# Expected final state:
#   x0  = 0x00000000
#   a0  = 0x00000005
#   a1  = 0xfffffffd   (-3)
#   a2  = 0x00000002
#   a3  = 0x12345000
#   a4  = 0x00000005   (lw from 0x200)
#   a5  = 0x00000002   (lw from 0x204)
#   a6  = 0x00000007   (set after the call returns)
#   a7  = 0x0000002a   (42, set inside func)
#   s0  = 0x000000ab   (lbu from 0x209)
#   s1  = 0x0000ab05   (lw from 0x208 after the byte store)
#   s2  = 0x000000fd   (lbu must zero-extend: 0xfd, not 0xfffffffd)
#   t0  = 0x00000200
#   t1  = 0x000000ab
#   ra  = 0x00000048   (return address of the jalr call)
#   pc  = 0x00000050   (stays at ebreak)
#   RAM[0x208..0x20b] = 05 ab 00 00
#   RAM[0x20c..0x20f] = fd ff ff ff
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

    sw   a0, 8(t0)            # 0x28: RAM[0x208] = 0x00000005
    addi t1, zero, 0xab       # 0x2c: t1 = 0xab
    sb   t1, 9(t0)            # 0x30: RAM[0x209] = 0xab (only one byte)
    lbu  s0, 9(t0)            # 0x34: s0 = 0x000000ab
    lw   s1, 8(t0)            # 0x38: s1 = 0x0000ab05
    sw   a1, 12(t0)           # 0x3c: RAM[0x20c] = 0xfffffffd
    lbu  s2, 12(t0)           # 0x40: s2 = 0x000000fd

    jalr ra, 84(zero)         # 0x44: call func at 0x54, ra = 0x48
    addi a6, zero, 7          # 0x48: executed after the call returns
    jalr zero, 80(zero)       # 0x4c: jump to the ebreak at 0x50
    ebreak                     # 0x50: halt
func:
    addi a7, zero, 42         # 0x54: a7 = 42
    jalr zero, 0(ra)          # 0x58: return to ra (0x48)
