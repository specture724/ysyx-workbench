# miniRV 独立运行 / 调试框架

把 `objcopy -O binary` 生成的裸二进制装进 `npc/emu/src/minirvEMU.cpp` 的统一 RAM（代码和数据共用），然后带
trace、断点和结束状态断言地跑起来。不依赖 Verilator，方便先把参考模型本身调对。

## 目录

```
npc/emu/
├── include/
│   ├── minirvEMU.h      # 参考模型对外接口
│   └── loader.h         # 文件 -> RAM
├── src/
│   ├── minirvEMU.cpp    # 参考模型实现（统一的 RAM / 译码 / 执行）
│   ├── loader.cpp       # 读裸二进制并调用 ref_load_ram()
│   └── emu_main.cpp     # CLI、运行循环、trace、寄存器/RAM dump、--expect
├── examples/program.S   # 与 npc/csrc/program.o 里那段程序等价的汇编
├── Makefile
└── README.md
```

模型接口在 `npc/emu/include/minirvEMU.h`（`loader.cpp` / `emu_main.cpp` 都 include 它，不再手写声明）：

```cpp
void     ref_load_ram (const uint8_t *data, std::size_t size, uint32_t base = 0);
const uint8_t *ref_get_ram ();
uint32_t ref_ram_size ();
void     ref_set_pc (uint32_t pc);
```

## 快速开始

```bash
# 1. 构建加载器/调试器 -> npc/build/emu
make -C npc/emu

# 2. 构建自带示例 -> npc/build/program.bin
make -C npc/emu example

# 3. 带 trace 跑，并检查结束状态
npc/build/emu --bin npc/build/program.bin \
    --trace --expect a0=30 --expect ra=0xc --expect pc=0xc
```

输出：

```
loaded npc/build/program.bin: 24 bytes -> RAM [0x00000000, 0x00000018), entry 0x00000000
     1  0x00000000  01400513  addi a0, zero, 20         pc -> 0x00000004  x10(a0) = 0x00000014
     2  0x00000004  010000e7  jalr ra, 16(zero)         pc -> 0x00000010  x1(ra) = 0x00000008
     3  0x00000010  00a50513  addi a0, a0, 10           pc -> 0x00000014  x10(a0) = 0x0000001e
     4  0x00000014  00008067  jalr zero, 0(ra)          pc -> 0x00000008
     5  0x00000008  00c000e7  jalr ra, 12(zero)         pc -> 0x0000000c  x1(ra) = 0x0000000c
     6  0x0000000c  00c00067  jalr zero, 12(zero)       pc -> 0x0000000c

halted (self-loop) after 6 instruction(s)
pc = 0x0000000c
  x1(ra)     = 0x0000000c (12)
  x10(a0)    = 0x0000001e (30)

checks:
  ok    x10(a0)    = 0x0000001e
  ok    x1(ra)     = 0x0000000c
  ok    pc         = 0x0000000c
PASS
```

`PC` 执行后不变（自跳转）被视为停机，这是示例程序里 `halt:` 的约定。

## 工具链

示例用 GCC 系列的 RISC-V 交叉工具链（Arch 包，已安装）：

```bash
sudo pacman -S riscv64-linux-gnu-gcc   # 自带 gcc / g++ / ld / objcopy
```

安装后可用的命令：`riscv64-linux-gnu-gcc`、`riscv64-linux-gnu-g++`、
`riscv64-linux-gnu-ld`、`riscv64-linux-gnu-objcopy`。Makefile 里的前缀可以用
`RISCV_PREFIX=<prefix>` 覆盖。

## 用自己的程序

当前模型的 RAM 基址和 PC 都是 0，所以**链接地址要是 0**：

```bash
riscv64-linux-gnu-gcc -march=rv32i -mabi=ilp32 -c program.S -o program.o
riscv64-linux-gnu-ld -m elf32lriscv --image-base=0 -Ttext=0 -e _start \
    -o program.elf program.o
riscv64-linux-gnu-objcopy -O binary program.elf program.bin
npc/build/emu --bin program.bin --trace
```

C/C++ 程序同样用这套 gcc/g++（需要 `-nostdlib` 或自己带 `-T` 链接脚本和启动
代码；当前模型还没有 load/store，所以只适合纯计算的小程序）。

注意事项：

- **不要直接把未链接的 `.o` 喂给 `objcopy -O binary`**：重定位（尤其是
  `auipc`/全局地址/`call`）还没解析，生成的二进制地址是错的。先链接再 objcopy。
- `csrc/program.o` 目前是 objdump 的文本，不是 ELF；需要用真正的工具链重新生成。
- RAM 只有 4 KiB（`RAM_SIZE`，在 `src/minirvEMU.cpp` 里改），代码和数据共用这一块。
- `--base` 同时决定装载地址和初始 PC。程序如果不是按 `--base` 的地址链接的，
  跳到高地址后取不到正确指令，会报 fetch fault。
- 加载的是扁平二进制（.text/.data 按 LMA 排布），没有 ELF 段解析；当前模型也
  没有 load/store，所以小程序的 `.text` 就是全部内容。

## 选项

| 选项 | 说明 |
| --- | --- |
| `--bin FILE` | 要加载的裸二进制（必填） |
| `--base ADDR` | 装载地址和初始 PC，默认 0 |
| `--steps N` | 只执行 N 条就停（退出码 0） |
| `--max-cycles N` | 安全上限，撞上说明没停机（退出码 4），默认 1000000 |
| `--trace` | 打印每条执行的指令、下一条 PC、写回结果 |
| `--trace-from N` | 从第 N 条（1-based）开始打印，配合 `--trace` |
| `--break ADDR` | 执行到 ADDR 之前停下，可重复给多个断点 |
| `--dump-ram[=S:L]` | 打印 RAM 从 S 开始 L 字节，默认 `0:64` |
| `--expect REG=VAL` | 检查结束时的寄存器，REG 支持 `x0`..`x31`、ABI 名（`a0`/`ra`/`sp`…）和 `pc`，VAL 支持十进制/0x 十六进制，可重复 |
| `-h, --help` | 帮助 |

## 退出码

| 码 | 含义 |
| --- | --- |
| 0 | 正常停机 / 命中 breakpoint / 到达 `--steps` |
| 1 | 参数错误、文件打不开、镜像超过 RAM |
| 2 | trap：非法指令或取指越界/非对齐 |
| 3 | `--expect` 不匹配 |
| 4 | 达到 `--max-cycles` 仍未停机 |

适合直接接进脚本：

```bash
npc/build/emu --bin test.bin --expect a0=30 --expect pc=0xc >/dev/null && echo PASS
```

## 常见调试流程

```bash
# 只看前 10 条，别让死循环刷屏
npc/build/emu --bin program.bin --trace --steps 10

# 停在某条指令前，看寄存器现场
npc/build/emu --bin program.bin --break 0x10

# 怀疑装载错了，先看 RAM 字节
npc/build/emu --bin program.bin --dump-ram=0:64 --steps 0

# 只从第 5 条开始 trace
npc/build/emu --bin program.bin --trace --trace-from 5
```

## 扩展

- **加指令**：在 `src/minirvEMU.cpp` 的 `decode()` / `imm_gen()` / `alu()` 里加控制信号，
  同时更新 `src/emu_main.cpp` 的 `disasm()` 让 trace 可读。
- **加内存/load-store**：`src/loader.cpp` 已经只依赖 `ref_load_ram()` 这个字节接口，
  以后换成按段加载或加 data 段不用改 CLI。
- **接 difftest**：在 `main.cpp` 里 `#include "minirvEMU.h"`（把 `npc/emu/include`
  加进 include path），就能直接 `ref_load_ram(...)` 装载同一份镜像，用
  `ref_set_pc(entry)` 设入口；trace/断言逻辑留在本目录，不污染模型。
