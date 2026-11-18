#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>
#include <vector>

#include "loader.h"
#include "minirvEMU.h"

namespace {

constexpr int kNumRegs = 32;
constexpr int kPc      = 32; // pseudo register id used by --expect / trace

const char *const kAbiNames[kNumRegs] = {
    "zero", "ra",  "sp",  "gp",  "tp",  "t0",  "t1",  "t2",
    "s0",   "s1",  "a0",  "a1",  "a2",  "a3",  "a4",  "a5",
    "a6",   "a7",  "s2",  "s3",  "s4",  "s5",  "s6",  "s7",
    "s8",   "s9",  "s10", "s11", "t3",  "t4",  "t5",  "t6",
};

const char *abi_name (int r) { return kAbiNames[r]; }

std::string reg_label (int r) {
    if (r == kPc) return "pc";
    return std::string("x") + std::to_string(r) + "(" + kAbiNames[r] + ")";
}

// ---------------------------------------------------------------- parsing

bool parse_u32 (const std::string &s, uint32_t &out) {
    if (s.empty() || s[0] == '-') return false;
    errno = 0;
    char *end = nullptr;
    const unsigned long long v = std::strtoull(s.c_str(), &end, 0);
    if (errno != 0 || end == s.c_str() || *end != '\0' || v > 0xFFFFFFFFull)
        return false;
    out = static_cast<uint32_t>(v);
    return true;
}

bool parse_u64 (const std::string &s, uint64_t &out) {
    if (s.empty() || s[0] == '-') return false;
    errno = 0;
    char *end = nullptr;
    const unsigned long long v = std::strtoull(s.c_str(), &end, 0);
    if (errno != 0 || end == s.c_str() || *end != '\0') return false;
    out = v;
    return true;
}

// Accepts "pc", "x0".."x31", or an ABI name ("a0", "ra", ...). "fp" is s0.
int parse_reg (const std::string &s) {
    if (s == "pc") return kPc;
    if (s == "fp") return 8;
    if (s.size() > 1 && (s[0] == 'x' || s[0] == 'X')) {
        uint32_t idx = 0;
        if (parse_u32(s.substr(1), idx) && idx < kNumRegs)
            return static_cast<int>(idx);
    }
    for (int i = 0; i < kNumRegs; i++)
        if (s == kAbiNames[i]) return i;
    return -1;
}

// ------------------------------------------------------------- RAM access

bool peek_instr (uint32_t pc, uint32_t &instr) {
    const uint32_t size = ref_ram_size();
    if ((pc & 0x3u) != 0 || pc > size - 4) return false;
    const uint8_t *ram = ref_get_ram();
    instr =  static_cast<uint32_t>(ram[pc])
          | (static_cast<uint32_t>(ram[pc + 1]) << 8)
          | (static_cast<uint32_t>(ram[pc + 2]) << 16)
          | (static_cast<uint32_t>(ram[pc + 3]) << 24);
    return true;
}

void dump_ram (uint32_t start, uint32_t len) {
    const uint8_t *ram = ref_get_ram();
    const uint32_t size = ref_ram_size();
    const uint32_t end = (start >= size) ? size : std::min(start + len, size);

    std::printf("RAM [0x%08x, 0x%08x):\n", start, end);
    if (start >= end) {
        std::printf("  (empty)\n");
        return;
    }
    for (uint32_t addr = start; addr < end; addr += 16) {
        std::printf("  %08x:", addr);
        for (uint32_t i = addr; i < addr + 16 && i < end; i++)
            std::printf(" %02x", ram[i]);
        std::printf("\n");
    }
}

// ------------------------------------------------------------- disassembly

// Mirrors the instruction subset implemented in src/minirvEMU.cpp.
std::string disasm (uint32_t instr) {
    char buf[96];
    const uint32_t opcode = instr & 0x7Fu;
    const uint32_t rd     = (instr >> 7) & 0x1Fu;
    const uint32_t funct3 = (instr >> 12) & 0x7u;
    const uint32_t rs1    = (instr >> 15) & 0x1Fu;
    const uint32_t rs2    = (instr >> 20) & 0x1Fu;
    const int32_t  imm    = static_cast<int32_t>(instr) >> 20;

    if (instr == 0x00100073u) return "ebreak";

    switch (opcode) {
    case 0x13:
        if (funct3 == 0x0)
            std::snprintf(buf, sizeof buf, "addi %s, %s, %d",
                          abi_name(rd), abi_name(rs1), static_cast<int>(imm));
        else
            std::snprintf(buf, sizeof buf, ".word 0x%08x", instr);
        return buf;
    case 0x67:
        if (funct3 == 0x0)
            std::snprintf(buf, sizeof buf, "jalr %s, %d(%s)",
                          abi_name(rd), static_cast<int>(imm), abi_name(rs1));
        else
            std::snprintf(buf, sizeof buf, ".word 0x%08x", instr);
        return buf;
    case 0x37:
        std::snprintf(buf, sizeof buf, "lui %s, 0x%x", abi_name(rd),
                      (instr >> 12) & 0xFFFFFu);
        return buf;
    case 0x33:
        if (funct3 == 0x0)
            std::snprintf(buf, sizeof buf, "add %s, %s, %s", abi_name(rd),
                          abi_name(rs1), abi_name(rs2));
        else
            std::snprintf(buf, sizeof buf, ".word 0x%08x", instr);
        return buf;
    case 0x03:
        if (funct3 == 0x2)
            std::snprintf(buf, sizeof buf, "lw %s, %d(%s)", abi_name(rd),
                          static_cast<int>(imm), abi_name(rs1));
        else if (funct3 == 0x4)
            std::snprintf(buf, sizeof buf, "lbu %s, %d(%s)", abi_name(rd),
                          static_cast<int>(imm), abi_name(rs1));
        else
            std::snprintf(buf, sizeof buf, ".word 0x%08x", instr);
        return buf;
    case 0x23: {
        uint32_t uimm = ((instr >> 25) << 5) | ((instr >> 7) & 0x1Fu);
        if (uimm & 0x800u) uimm |= 0xFFFFF000u;
        const int32_t simm = static_cast<int32_t>(uimm);
        if (funct3 == 0x2)
            std::snprintf(buf, sizeof buf, "sw %s, %d(%s)", abi_name(rs2),
                          static_cast<int>(simm), abi_name(rs1));
        else if (funct3 == 0x0)
            std::snprintf(buf, sizeof buf, "sb %s, %d(%s)", abi_name(rs2),
                          static_cast<int>(simm), abi_name(rs1));
        else
            std::snprintf(buf, sizeof buf, ".word 0x%08x", instr);
        return buf;
    }
    default:
        break;
    }
    std::snprintf(buf, sizeof buf, ".word 0x%08x", instr);
    return buf;
}

// ---------------------------------------------------------------- options

struct Expect {
    int      reg;
    uint32_t value;
};

struct Options {
    std::string          bin;
    uint32_t             base       = 0;
    uint64_t             steps      = UINT64_MAX;
    uint64_t             max_cycles = 1000000;
    bool                 trace      = false;
    uint64_t             trace_from = 1; // 1-based instruction number
    bool                 dump_ram   = false;
    uint32_t             dump_start = 0;
    uint32_t             dump_len   = 64;
    std::vector<uint32_t> breaks;
    std::vector<Expect>  expects;
};

void usage (const char *prog) {
    std::printf(
        "Usage: %s --bin <file> [options]\n"
        "\n"
        "Load a raw binary image (e.g. `objcopy -O binary`) into the reference\n"
        "model's RAM and run it, with optional tracing and end-state checks.\n"
        "\n"
        "Options:\n"
        "  --bin FILE          raw binary to load (required)\n"
        "  --base ADDR         load address and initial pc (default 0)\n"
        "  --steps N           stop after N instructions (default: unlimited)\n"
        "  --max-cycles N      safety cap; reaching it exits with status 4\n"
        "                      (default 1000000)\n"
        "  --trace             print every executed instruction\n"
        "  --trace-from N      first instruction to trace, 1-based (default 1)\n"
        "  --break ADDR        stop before the instruction at ADDR (repeatable)\n"
        "  --dump-ram[=S:L]    dump L bytes (default 64) from address S\n"
        "                      (default 0), e.g. --dump-ram=0x10:0x20\n"
        "  --expect REG=VAL    check final REG (xN / ABI name / pc) equals VAL\n"
        "                      (repeatable)\n"
        "  -h, --help          show this help\n"
        "\n"
        "Exit status:\n"
        "  0  ok (halted, breakpoint hit, or step limit reached)\n"
        "  1  usage or load error\n"
        "  2  trap (illegal instruction or fetch fault)\n"
        "  3  expectation mismatch\n"
        "  4  max-cycles reached without halting\n",
        prog);
}

bool take_value (int &i, int argc, char **argv, const std::string &opt,
                 std::string &out, std::string &err) {
    if (i + 1 >= argc) {
        err = "missing value for " + opt;
        return false;
    }
    out = argv[++i];
    return true;
}

bool parse_args (int argc, char **argv, Options &opt, bool &help,
                 std::string &err) {
    for (int i = 1; i < argc; i++) {
        const std::string a = argv[i];
        std::string v;

        if (a == "-h" || a == "--help") {
            help = true;
            return true;
        }
        if (a == "--bin") {
            if (!take_value(i, argc, argv, a, v, err)) return false;
            opt.bin = v;
        } else if (a == "--base") {
            if (!take_value(i, argc, argv, a, v, err)) return false;
            if (!parse_u32(v, opt.base)) {
                err = "bad --base value: '" + v + "'";
                return false;
            }
        } else if (a == "--steps") {
            if (!take_value(i, argc, argv, a, v, err)) return false;
            if (!parse_u64(v, opt.steps)) {
                err = "bad --steps value: '" + v + "'";
                return false;
            }
        } else if (a == "--max-cycles") {
            if (!take_value(i, argc, argv, a, v, err)) return false;
            if (!parse_u64(v, opt.max_cycles)) {
                err = "bad --max-cycles value: '" + v + "'";
                return false;
            }
        } else if (a == "--trace") {
            opt.trace = true;
        } else if (a == "--trace-from") {
            if (!take_value(i, argc, argv, a, v, err)) return false;
            if (!parse_u64(v, opt.trace_from) || opt.trace_from == 0) {
                err = "bad --trace-from value: '" + v + "' (must be >= 1)";
                return false;
            }
        } else if (a == "--break") {
            uint32_t addr = 0;
            if (!take_value(i, argc, argv, a, v, err)) return false;
            if (!parse_u32(v, addr)) {
                err = "bad --break value: '" + v + "'";
                return false;
            }
            opt.breaks.push_back(addr);
        } else if (a == "--dump-ram") {
            opt.dump_ram = true;
        } else if (a.compare(0, 11, "--dump-ram=") == 0) {
            const std::string spec = a.substr(11);
            const std::size_t colon = spec.find(':');
            if (colon == std::string::npos ||
                !parse_u32(spec.substr(0, colon), opt.dump_start) ||
                !parse_u32(spec.substr(colon + 1), opt.dump_len)) {
                err = "bad --dump-ram value: '" + spec + "' (expected START:LEN)";
                return false;
            }
            opt.dump_ram = true;
        } else if (a == "--expect") {
            if (!take_value(i, argc, argv, a, v, err)) return false;
            const std::size_t eq = v.find('=');
            if (eq == std::string::npos) {
                err = "bad --expect value: '" + v + "' (expected REG=VAL)";
                return false;
            }
            const std::string name = v.substr(0, eq);
            const int reg = parse_reg(name);
            if (reg < 0) {
                err = "unknown register in --expect: '" + name + "'";
                return false;
            }
            Expect e{reg, 0};
            if (!parse_u32(v.substr(eq + 1), e.value)) {
                err = "bad value in --expect: '" + v + "'";
                return false;
            }
            opt.expects.push_back(e);
        } else {
            err = "unknown option: '" + a + "'";
            return false;
        }
    }
    return true;
}

} // namespace

int main (int argc, char **argv) {
    Options opt;
    bool help = false;
    std::string err;

    if (!parse_args(argc, argv, opt, help, err)) {
        std::fprintf(stderr, "error: %s\n\n", err.c_str());
        usage(argv[0]);
        return 1;
    }
    if (help) {
        usage(argv[0]);
        return 0;
    }
    if (opt.bin.empty()) {
        std::fprintf(stderr, "error: --bin is required\n\n");
        usage(argv[0]);
        return 1;
    }

    ref_init();
    ref_set_pc(opt.base);

    std::size_t loaded = 0;
    if (!emu_load_binary(opt.bin, opt.base, loaded, err)) {
        std::fprintf(stderr, "load error: %s\n", err.c_str());
        return 1;
    }

    std::printf("loaded %s: %zu bytes -> RAM [0x%08x, 0x%08x), entry 0x%08x\n",
                opt.bin.c_str(), loaded, opt.base,
                opt.base + static_cast<uint32_t>(loaded), opt.base);
    if (loaded % 4 != 0)
        std::fprintf(stderr, "warning: binary size is not a multiple of 4\n");
    if (opt.base % 4 != 0)
        std::fprintf(stderr, "warning: load address is not 4-byte aligned\n");

    if (opt.dump_ram) dump_ram(opt.dump_start, opt.dump_len);

    std::vector<uint32_t> before(kNumRegs);
    uint64_t executed = 0;
    const char *stop = "halted";
    bool hit_max_cycles = false;
    bool trapped = false;

    for (;;) {
        const uint32_t pc = ref_get_pc();

        if (std::find(opt.breaks.begin(), opt.breaks.end(), pc) !=
            opt.breaks.end()) {
            stop = "breakpoint";
            break;
        }
        if (executed >= opt.steps) {
            stop = "step limit";
            break;
        }
        if (executed >= opt.max_cycles) {
            stop = "max cycles without halt";
            hit_max_cycles = true;
            break;
        }

        uint32_t instr = 0;
        peek_instr(pc, instr);
        std::copy(ref_get_regs(), ref_get_regs() + kNumRegs, before.begin());
        uint8_t ret_code = 0;
        try {
            ret_code = ref_inst_cycle();
        } catch (const std::exception &e) {
            std::fprintf(stderr, "TRAP at pc=0x%08x (instruction %llu): %s\n",
                         pc, static_cast<unsigned long long>(executed + 1),
                         e.what());
            trapped = true;
            stop = "trap";
            break;
        }
        executed++;

        const uint32_t next_pc = ref_get_pc();
        uint32_t *regs = ref_get_regs();

        if (opt.trace && executed >= opt.trace_from) {
            int changed = -1;
            for (int r = 0; r < kNumRegs; r++) {
                if (regs[r] != before[r]) {
                    changed = r;
                    break;
                }
            }
            std::printf("%6llu  0x%08x  %08x  %-24s  pc -> 0x%08x",
                        static_cast<unsigned long long>(executed), pc, instr,
                        disasm(instr).c_str(), next_pc);
            if (changed >= 0)
                std::printf("  %s = 0x%08x", reg_label(changed).c_str(),
                            regs[changed]);
            std::printf("\n");
        }

        if (ret_code == 1) {
            stop = "EBREAK";
            break;
        }

        if (next_pc == pc) {
            stop = "halted (self-loop)";
            break;
        }
    }

    uint32_t *regs = ref_get_regs();
    std::printf("\n%s after %llu instruction(s)\n", stop,
                static_cast<unsigned long long>(executed));
    std::printf("pc = 0x%08x\n", ref_get_pc());

    bool any = false;
    for (int r = 1; r < kNumRegs; r++) {
        if (regs[r] != 0) {
            std::printf("  %-10s = 0x%08x (%u)\n", reg_label(r).c_str(),
                        regs[r], regs[r]);
            any = true;
        }
    }
    if (!any) std::printf("  (all GPRs except x0 are zero)\n");

    int failed = 0;
    if (!opt.expects.empty()) std::printf("\nchecks:\n");
    for (const Expect &e : opt.expects) {
        const uint32_t actual = (e.reg == kPc) ? ref_get_pc() : regs[e.reg];
        if (actual == e.value) {
            std::printf("  ok    %-10s = 0x%08x\n",
                        reg_label(e.reg).c_str(), actual);
        } else {
            std::printf("  FAIL  %-10s = 0x%08x (expected 0x%08x)\n",
                        reg_label(e.reg).c_str(), actual, e.value);
            failed++;
        }
    }

    if (trapped) return 2;
    if (hit_max_cycles) return 4;
    if (failed > 0) return 3;
    if (!opt.expects.empty()) std::printf("PASS\n");
    return 0;
}
