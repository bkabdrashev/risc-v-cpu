#include <cstdint>
#include <assert.h>

#define ERROR_NOT_IMPLEMENTED (1010)
#define ERROR_INVALID_RANGE   (2020)
#define OPCODE_BITS 7u
#define FUNCT3_BITS 3u
#define FUNCT7_BITS 7u
#define REG_INDEX_BITS 5u
#define ADDR_BITS 32u
#define REG_BITS 32u
#define BYTE 8u
#define INST_BITS 32u

#include "mem_map.h"
#define N_REGS 16u

#define OPCODE_ADDI  (0b0010011)
#define FUNCT3_ADDI  (0b000)
#define FUNCT3_SLLI  (0b001)
#define FUNCT3_SRLI  (0b101)
#define FUNCT7_SRLI  (0b0000000)
#define FUNCT7_SRAI  (0b0100000)

#define OPCODE_JALR  (0b1100111)
#define FUNCT3_JALR  (0b000)
#define OPCODE_ADD   (0b0110011)
#define FUNCT3_ADD   (0b000)
#define OPCODE_LUI   (0b0110111)
#define FUNCT3_LUI   (0b000)
#define OPCODE_LW    (0b0000011)
#define FUNCT3_LW    (0b010)
#define FUNCT3_LBU   (0b100)
#define OPCODE_SW    (0b0100011)
#define FUNCT3_SW    (0b010)
#define FUNCT3_SB    (0b000)

#define REG_A0 (0x10)

#define OPCODE_EBREAK (0b1110011)
#define OPCODE_OUT    (0b0001011)

struct bit {
  uint32_t v : 1;
};
struct bit4 {
  union {
    struct {
      bit a;
      bit b;
      bit c;
      bit d;
    };
    bit bits[4];
  };
};
struct Trigger {
  uint32_t prev : 1;
  uint32_t curr : 1;
};
struct opcode_size_t {
  uint32_t v : OPCODE_BITS;
};
struct funct3_size_t {
  uint32_t v : FUNCT3_BITS;
};
struct funct7_size_t {
  uint32_t v : FUNCT7_BITS;
};
struct addr_size_t {
  uint32_t v : ADDR_BITS;
};
struct reg_index_t {
  uint32_t v : REG_BITS;
};
struct reg_size_t {
  uint32_t v : REG_BITS;
};
struct byte {
  uint8_t v : BYTE;
};

typedef reg_size_t inst_size_t;

bit ZERO_BIT = {};
reg_index_t REG_0 = {};
reg_size_t REG_VALUE_0 = {};

struct miniRV {
  addr_size_t pc = {MEM_START};
  reg_size_t regs[N_REGS];
  byte mem[MEM_SIZE];
  byte vga[VGA_SIZE];

  uint8_t*  uart_ref;
  uint32_t* time_uptime_ref;

  bit ebreak;

  Trigger clock;
  Trigger reset;
};

int32_t sar32(uint32_t u, unsigned shift) {
  assert(shift < 32);

  if (shift == 0) return (u & 0x80000000u) ? -int32_t((~u) + 1u) : int32_t(u);

  uint32_t shifted = u >> shift;      
  if (u & 0x80000000u) {
    shifted |= (~0u << (32 - shift)); 
  }
  return static_cast<int32_t>(shifted);
}

uint32_t take_bit(uint32_t bits, uint32_t pos) {
  if (pos >= REG_BITS) {
    assert(0);
    return ERROR_INVALID_RANGE;
  }

  uint32_t mask = (1u << pos);
  return (bits & mask) >> pos;
}

uint32_t take_bits_range(uint32_t bits, uint32_t from, uint32_t to) {
  if (to >= REG_BITS || from >= REG_BITS || from > to) {
    assert(0);
    return ERROR_INVALID_RANGE;
  }

  uint32_t mask = ((1u << (to - from + 1)) - 1u) << from;
  return (bits & mask) >> from;
}

bool is_positive_edge(Trigger trigger) {
  if (!trigger.prev && trigger.curr) {
    return true;
  }
  return false;
}

bool is_negative_edge(Trigger trigger) {
  if (trigger.prev && !trigger.curr) {
    return true;
  }
  return false;
}

reg_size_t alu_eval(opcode_size_t opcode, reg_size_t rdata1, reg_size_t rdata2, reg_size_t imm, funct3_size_t funct3, funct7_size_t funct7) {
  reg_size_t result = {};
  if (opcode.v == OPCODE_ADDI && funct3.v == FUNCT3_ADDI) { // ADDI
    result.v = rdata1.v + imm.v;
  }
  else if (opcode.v == OPCODE_ADDI && funct3.v == FUNCT3_SLLI) {
    result.v = rdata1.v << imm.v;
  }
  else if (opcode.v == OPCODE_ADDI && funct3.v == FUNCT3_SRLI && funct7.v == FUNCT7_SRLI) {
    result.v = rdata1.v >> imm.v;
  }
  else if (opcode.v == OPCODE_ADDI && funct3.v == FUNCT3_SRLI && funct7.v == FUNCT7_SRAI) {
    result.v = sar32(rdata1.v, imm.v);
  }
  else if (opcode.v == OPCODE_ADD) {
    result.v = rdata1.v + rdata2.v;
  }
  else {
    result.v = ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

void gm_mem_reset(miniRV* cpu) {
  memset(cpu->mem, 0, MEM_SIZE);
  memset(cpu->vga, 0, VGA_SIZE);
}

inst_size_t gm_mem_read(miniRV* cpu, addr_size_t addr) {
  inst_size_t result = {0};
  if (addr.v >= VGA_START && addr.v < VGA_END-3) {
    addr.v -= VGA_START;
    result.v = 
      cpu->vga[addr.v+3].v << 24 | cpu->vga[addr.v+2].v << 16 |
      cpu->vga[addr.v+1].v <<  8 | cpu->vga[addr.v+0].v <<  0 ;
  }
  else if (addr.v >= MEM_START && addr.v < MEM_END-3) {
    addr.v -= MEM_START;
    result.v = 
      cpu->mem[addr.v+3].v << 24 | cpu->mem[addr.v+2].v << 16 |
      cpu->mem[addr.v+1].v <<  8 | cpu->mem[addr.v+0].v <<  0 ;
  }
  else if (addr.v == UART_STATUS_ADDR) {
    result.v = cpu->uart_ref[4];
  }
  else if (addr.v == TIME_UPTIME_ADDR) {
    result.v = cpu->time_uptime_ref[0];
  }
  else if (addr.v == TIME_UPTIME_ADDR + 4) {
    result.v = cpu->time_uptime_ref[1];
  }
  else {
    // printf("GM WARNING: mem read memory is not mapped\n");
  }
  return result;
}

void gm_mem_write(miniRV* cpu, bit write_enable, bit4 write_enable_bytes, addr_size_t addr, reg_size_t write_data) {
  if (is_positive_edge(cpu->reset)) {
    memset(cpu->mem, 0, MEM_SIZE);
    memset(cpu->vga, 0, VGA_SIZE);
  }
  // printf("gm mem write: %u, %u %u %u %u, addr: 0x%x, wdata: 0x%x\n", write_enable.v, write_enable_bytes.bits[0].v, write_enable_bytes.bits[1].v, write_enable_bytes.bits[2].v, write_enable_bytes.bits[3].v, addr.v, write_data.v);
  if (write_enable.v) {
    if (addr.v >= VGA_START && addr.v < VGA_END-3) {
      addr.v -= VGA_START;
      if (write_enable_bytes.bits[0].v) {
        cpu->vga[addr.v + 0].v = (write_data.v & (0xff << 0)) >> 0;
      }
      if (write_enable_bytes.bits[1].v) {
        cpu->vga[addr.v + 1].v = (write_data.v & (0xff << 8)) >> 8;
      }
      if (write_enable_bytes.bits[2].v) {
        cpu->vga[addr.v + 2].v = (write_data.v & (0xff << 16)) >> 16;
      }
      if (write_enable_bytes.bits[3].v) {
        cpu->vga[addr.v + 3].v = (write_data.v & (0xff << 24)) >> 24;
      }
    }
    else if (addr.v >= MEM_START && addr.v < MEM_END-3) {
      addr.v -= MEM_START;
      if (write_enable_bytes.bits[0].v) {
        cpu->mem[addr.v + 0].v = (write_data.v >>  0) & 0xff;
      }
      if (write_enable_bytes.bits[1].v) {
        cpu->mem[addr.v + 1].v = (write_data.v >>  8) & 0xff;
      }
      if (write_enable_bytes.bits[2].v) {
        cpu->mem[addr.v + 2].v = (write_data.v >> 16) & 0xff;
      }
      if (write_enable_bytes.bits[3].v) {
        cpu->mem[addr.v + 3].v = (write_data.v >> 24) & 0xff;
      }
    }
    else if (addr.v == UART_DATA_ADDR) {
      putc(write_data.v & 0xff, stderr);
    }
    else {
      // printf("GM WARNING: mem write memory is not mapped\n");
    }
  }
}

void pc_write(miniRV* cpu, addr_size_t in_addr, bit is_jump) {
  if (is_positive_edge(cpu->reset)) {
    cpu->pc.v = MEM_START;
  }
  if (is_jump.v) {
    cpu->pc.v = in_addr.v;
  }
  else {
    cpu->pc.v += 4;
  }
}

struct RF_out {
  reg_size_t rdata1;
  reg_size_t rdata2;
  reg_size_t regs[N_REGS];
};

RF_out rf_read(miniRV* cpu, reg_index_t reg_src1, reg_index_t reg_src2) {
  RF_out out = {};
  if (reg_src1.v < N_REGS) out.rdata1.v = cpu->regs[reg_src1.v].v;
  if (reg_src2.v < N_REGS) out.rdata2.v = cpu->regs[reg_src2.v].v;
  for (int i = 0; i < N_REGS; i++) {
    out.regs[i].v = cpu->regs[i].v;
  }
  return out;
}

void rf_write(miniRV* cpu, bit write_enable, reg_index_t reg_dest, reg_size_t write_data) {
  if (is_positive_edge(cpu->reset)) {
    for (uint32_t i = 0; i < N_REGS; i++) {
      cpu->regs[i].v = 0;
    }
  }
  if (write_enable.v && reg_dest.v != 0 && reg_dest.v < N_REGS) {
    cpu->regs[reg_dest.v] = write_data;
  }
}

struct Dec_out {
  reg_index_t reg_dest;
  reg_index_t reg_src1;
  reg_index_t reg_src2;
  reg_size_t imm;
  opcode_size_t opcode;
  funct3_size_t funct3;
  funct7_size_t funct7;
  bit reg_write_enable;
};

Dec_out dec_eval(inst_size_t inst) {
  Dec_out out = {};
  out.opcode.v = take_bits_range(inst.v, 0, 6);
  out.reg_dest.v = take_bits_range(inst.v, 7, 11);
  out.funct3.v = take_bits_range(inst.v, 12, 14);
  out.funct7.v = take_bits_range(inst.v, 25, 31);
  out.reg_src1.v = take_bits_range(inst.v, 15, 19);
  out.reg_src2.v = take_bits_range(inst.v, 20, 24);
  bit sign = {.v=take_bit(inst.v, 31)};
  if (out.opcode.v == OPCODE_ADDI) {
    // ADDI, SLLI, SRLI, SARLI
    if (sign.v) out.imm.v = (~0u << 12) | take_bits_range(inst.v, 20, 31);
    else        out.imm.v = ( 0u << 12) | take_bits_range(inst.v, 20, 31);
    if (out.funct3.v == FUNCT3_SLLI || out.funct3.v == FUNCT3_SRLI) {
      out.imm.v &= 0b11111;
    }
    out.reg_write_enable.v = 1;
  }
  else if (out.opcode.v == OPCODE_JALR) {
    // JALR
    if (sign.v) out.imm.v = (~0u << 12) | take_bits_range(inst.v, 20, 31);
    else        out.imm.v = ( 0u << 12) | take_bits_range(inst.v, 20, 31);
    out.reg_write_enable.v = 1;
  }
  else if (out.opcode.v == OPCODE_ADD) {
    // ADD
    if (sign.v) out.imm.v = (~0u << 20) | take_bits_range(inst.v, 12, 31);
    else        out.imm.v = ( 0u << 20) | take_bits_range(inst.v, 12, 31);
    out.reg_write_enable.v = 1;
  }
  else if (out.opcode.v == OPCODE_LUI) {
    // LUI
    out.imm.v = take_bits_range(inst.v, 12, 31) << 12;
    out.reg_write_enable.v = 1;
  }
  else if (out.opcode.v == OPCODE_LW) {
    // LW, LBU
    if (sign.v) out.imm.v = (~0u << 12) | take_bits_range(inst.v, 20, 31);
    else        out.imm.v = ( 0u << 12) | take_bits_range(inst.v, 20, 31);
    out.reg_write_enable.v = 1;
  }
  else if (out.opcode.v == OPCODE_SW) {
    // SW, SB
    uint32_t top_imm = take_bits_range(inst.v, 25, 31);
    uint32_t bot_imm = take_bits_range(inst.v, 7, 11);
    top_imm <<= 5;
    if (sign.v) out.imm.v = (~0u << 12) | top_imm | bot_imm;
    else        out.imm.v = ( 0u << 12) | top_imm | bot_imm;
    out.reg_write_enable.v = 0;
  }
  else if (out.opcode.v == OPCODE_EBREAK) {
    // EBREAK
    out.imm.v = take_bits_range(inst.v, 20, 31);
    out.reg_write_enable.v = 0;
  }
  else {
    out.imm.v = 0;
    out.reg_write_enable.v = 0;
  }
  return out;
}

void print_instruction(inst_size_t inst) {
  Dec_out dec = dec_eval(inst);
  switch (dec.opcode.v) {
    case OPCODE_ADDI: {
      if (dec.funct3.v == FUNCT3_ADDI) { // ADDI
        printf("addi imm=%i\t rs1=x%u\t rd=x%u\n", dec.imm.v, dec.reg_src1.v, dec.reg_dest.v);
      }
      else if (dec.funct3.v == FUNCT3_SLLI) { // SLLI
        printf("slli imm=%i\t rs1=x%u\t rd=x%u\n", dec.imm.v, dec.reg_src1.v, dec.reg_dest.v);
      }
      else if (dec.funct3.v == FUNCT3_SRLI) { // SRLI
        if (dec.funct7.v == FUNCT7_SRLI) {
          printf("srli imm=%i\t rs1=x%u\t rd=x%u\n", dec.imm.v, dec.reg_src1.v, dec.reg_dest.v);
        }
        else if (dec.funct7.v == FUNCT7_SRAI) {
          printf("srai imm=%i\t rs1=x%u\t rd=x%u\n", dec.imm.v, dec.reg_src1.v, dec.reg_dest.v);
        }
        else {
          goto not_implemented;
        }
      }
      else {
        goto not_implemented;
      }

    } break;
    case OPCODE_JALR: {
      printf("jalr imm=%i\t rs1=x%u\t rd=x%u\n", dec.imm.v, dec.reg_src1.v, dec.reg_dest.v);
    } break;
    case OPCODE_ADD: { // ADD
      printf("add  rs2=x%u\t rs1=x%u\t rd=x%u\n", dec.reg_src2.v, dec.reg_src1.v, dec.reg_dest.v);
    } break;
    case OPCODE_LUI: { // LUI
      int32_t v = sar32(dec.imm.v, 12);
      printf("lui  imm=%i\t rd=x%u\n", v, dec.reg_dest.v);
    } break;
    case OPCODE_LW: {
      if (dec.funct3.v == FUNCT3_LW) { // LW
        printf("lw   imm=%i\t rs1=x%u\t rd=x%u\n", dec.imm.v, dec.reg_src1.v, dec.reg_dest.v);
      }
      else if (dec.funct3.v == FUNCT3_LBU) { // LBU
        printf("lbu  imm=%i\t rs1=x%u\t rd=x%u\n", dec.imm.v, dec.reg_src1.v, dec.reg_dest.v);
      }
      else {
        printf("not implemented:%u\n", inst);
      }
    } break;
    case OPCODE_SW: {
      if (dec.funct3.v == FUNCT3_SW) { // SW
        printf("sw   imm=%i\t rs2=x%u\t rs1=x%u\n", dec.imm.v, dec.reg_src2.v, dec.reg_src1.v);
      }
      else if (dec.funct3.v == FUNCT3_SB) { // LBU
        printf("sb   imm=%i\t rs2=x%u\t rs1=x%u\n", dec.imm.v, dec.reg_src2.v, dec.reg_src1.v);
      }
      else {
        printf("not implemented:%u\n", inst);
      }
    } break;
    case OPCODE_EBREAK: {
      // EBREAK
      printf("ebreak\n");
    } break;

    case OPCODE_OUT: {
      // OUT
      printf("out rs1=x%u\n", dec.reg_src1.v);
    } break;

    default: { // NOT IMPLEMENTED
      not_implemented:
      printf("GM WARNING: not implemented:0x%x\n", inst);
    } break;
  }
}

struct CPU_out {
  reg_index_t reg_dest;
  reg_size_t  reg_write_data;
  addr_size_t pc_addr;
  addr_size_t ram_addr;
  reg_size_t  ram_write_data;
  bit ebreak;
};

CPU_out cpu_eval(miniRV* cpu) {
  CPU_out out = {};
  inst_size_t inst = gm_mem_read(cpu, cpu->pc);
  Dec_out dec_out = dec_eval(inst);
  RF_out rf_out = rf_read(cpu, dec_out.reg_src1, dec_out.reg_src2);
  reg_size_t alu_out = alu_eval(dec_out.opcode, rf_out.rdata1, rf_out.rdata2, dec_out.imm, dec_out.funct3, dec_out.funct7);

  addr_size_t in_addr = {};
  bit is_jump = {};
  reg_size_t reg_write_data = {};

  reg_size_t ram_write_data = {};
  bit ram_write_enable = {};
  bit4 ram_write_enable_bytes = {};
  addr_size_t ram_addr = {};
  if (dec_out.opcode.v == OPCODE_ADDI) {
    // ADDI
    reg_write_data = alu_out;
  }
  else if (dec_out.opcode.v == OPCODE_JALR) {
    // JALR
    reg_write_data.v= cpu->pc.v + 4;
    in_addr.v = (rf_out.rdata1.v + dec_out.imm.v) & ~3;
    is_jump.v = 1;
  }
  else if (dec_out.opcode.v == OPCODE_ADD) {
    // ADD
    reg_write_data = alu_out;
  }
  else if (dec_out.opcode.v == OPCODE_LUI) {
    // LUI
    reg_write_data = dec_out.imm;
  }
  else if (dec_out.opcode.v == OPCODE_LW && dec_out.funct3.v == FUNCT3_LW) {
    // LW
    ram_addr.v = rf_out.rdata1.v + dec_out.imm.v;
    reg_write_data = gm_mem_read(cpu, ram_addr);
  }
  else if (dec_out.opcode.v == OPCODE_LW && dec_out.funct3.v == FUNCT3_LBU) {
    // LBU
    ram_addr.v = rf_out.rdata1.v + dec_out.imm.v;
    reg_write_data.v = gm_mem_read(cpu, ram_addr).v & 0xff;
  }
  else if (dec_out.opcode.v == OPCODE_SW) {
    // SW
    ram_write_enable.v = 1;
    ram_addr.v = rf_out.rdata1.v + dec_out.imm.v;
    ram_write_data = rf_out.rdata2;
    if (dec_out.funct3.v == FUNCT3_SW) {
      ram_write_enable_bytes = {1, 1, 1, 1};
    }
    else if (dec_out.funct3.v == FUNCT3_SB) {
      ram_write_enable_bytes = {1, 0, 0, 0};
    }
  }
  else if (dec_out.opcode.v == OPCODE_EBREAK && dec_out.imm.v == 1) {
    // EBREAK
    out.ebreak.v = 1;
    cpu->ebreak.v = 1;
  }
  else {
    print_instruction(inst);
  }

  out.reg_dest = dec_out.reg_dest;
  out.reg_write_data = reg_write_data;
  out.pc_addr = cpu->pc;
  out.ram_addr = ram_addr;
  out.ram_write_data = ram_write_data;

  rf_write(cpu, dec_out.reg_write_enable, dec_out.reg_dest, reg_write_data);
  gm_mem_write(cpu, ram_write_enable, ram_write_enable_bytes, ram_addr, ram_write_data);
  pc_write(cpu, in_addr, is_jump);
  return out;
}

void cpu_reset_regs(miniRV* cpu) {
  cpu->pc.v = MEM_START;
  for (uint32_t i = 0; i < N_REGS; i++) {
    cpu->regs[i].v = 0;
  }
}

struct GmVcdTrace {
  miniRV* gm = nullptr;
  uint32_t base = 0;

  static constexpr int kRegs = 16;

  explicit GmVcdTrace(miniRV* g) : gm(g) {}

  // ---- declBus compatibility: Verilator versions differ in declBus signature ----
  template <typename VcdT>
  static auto declBusCompat(VcdT* vcdp, uint32_t code, const char* name, int msb, int lsb, int)
      -> decltype(vcdp->declBus(code, name, false, -1, msb, lsb), void()) {
    // Newer-ish simple declBus(code,name,array,arraynum,msb,lsb)
    vcdp->declBus(code, name, false, -1, msb, lsb);
  }

  template <typename VcdT>
  static auto declBusCompat(VcdT* vcdp, uint32_t code, const char* name, int msb, int lsb, long) -> decltype(vcdp->declBus(code, 0, name, -1,
                                static_cast<VerilatedTraceSigDirection>(0),
                                static_cast<VerilatedTraceSigKind>(0),
                                static_cast<VerilatedTraceSigType>(0),
                                false, 0, msb, lsb), void()) {
    // Newer generic declBus(code,fidx,name,dtypenum,dir,kind,type,isArray,arraynum,msb,lsb)
    vcdp->declBus(code, 0, name, -1,
                  static_cast<VerilatedTraceSigDirection>(0),
                  static_cast<VerilatedTraceSigKind>(0),
                  static_cast<VerilatedTraceSigType>(0),
                  false, 0, msb, lsb);
  }

  static void init_cb(void* userp, VerilatedVcd* vcdp, uint32_t code) {
    auto* self = static_cast<GmVcdTrace*>(userp);
    self->base = code;

    declBusCompat(vcdp, code + 0, "gm.pc", 31, 0, 0);

    for (int i = 0; i < kRegs; i++) {
      char n[32];
      std::snprintf(n, sizeof(n), "gm.x%d", i);
      declBusCompat(vcdp, code + 1 + i, n, 31, 0, 0);
    }
  }

  static void full_cb(void* userp, VerilatedVcd::Buffer* bufp) {
    auto* self = static_cast<GmVcdTrace*>(userp);
    vluint32_t* const oldp = bufp->oldp(self->base);

    bufp->fullIData(oldp + 1, self->gm->pc.v, 32);
    for (int i = 0; i < kRegs; i++) {
      bufp->fullIData(oldp + 2 + i, self->gm->regs[i].v, 32);
    }
  }

  static void chg_cb(void* userp, VerilatedVcd::Buffer* bufp) {
    auto* self = static_cast<GmVcdTrace*>(userp);
    const uint32_t b = self->base;

    // Incremental dump when changed
    bufp->chgIData(bufp->oldp(b + 0), self->gm->pc.v, 32);
    for (int i = 0; i < kRegs; i++) {
      bufp->chgIData(bufp->oldp(b + 1 + i), self->gm->regs[i].v, 32);
    }
  }
};

inst_size_t addi(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 20) | (reg_src1 << 15) | (FUNCT3_ADDI << 12) | (reg_dest << 7) | OPCODE_ADDI;
  inst_size_t result = { inst_u32 };
  return result;
}

inst_size_t slli(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 20) | (reg_src1 << 15) | (FUNCT3_SLLI << 12) | (reg_dest << 7) | OPCODE_ADDI;
  inst_size_t result = { inst_u32 };
  return result;
}

inst_size_t srli(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (FUNCT7_SRLI << 25) | (imm << 20) | (reg_src1 << 15) | (FUNCT3_SRLI << 12) | (reg_dest << 7) | OPCODE_ADDI;
  inst_size_t result = { inst_u32 };
  return result;
}

inst_size_t srai(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (FUNCT7_SRAI << 25) | (imm << 20) | (reg_src1 << 15) | (FUNCT3_SRLI << 12) | (reg_dest << 7) | OPCODE_ADDI;
  inst_size_t result = { inst_u32 };
  return result;
}

inst_size_t li(uint32_t imm, uint32_t reg_dest) {
  return addi(imm, 0, reg_dest);
}

inst_size_t jalr(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 20) | (reg_src1 << 15) | (FUNCT3_JALR << 12) | (reg_dest << 7) | OPCODE_JALR;
  inst_size_t result = { inst_u32 };
  return result;
}

inst_size_t add(uint32_t reg_src2, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (0b0000000 << 25) | (reg_src2 << 20) | (reg_src1 << 15) | (FUNCT3_ADD << 12) | (reg_dest << 7) | OPCODE_ADD;
  inst_size_t result = { inst_u32 };
  return result;
}

inst_size_t lui(uint32_t imm, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 12) | (reg_dest << 7) | OPCODE_LUI;
  inst_size_t result = { inst_u32 };
  return result;
}

inst_size_t lw(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 20) | (reg_src1 << 15) | (FUNCT3_LW << 12) | (reg_dest << 7) | OPCODE_LW;
  inst_size_t result = { inst_u32 };
  return result;
}

inst_size_t lbu(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 20) | (reg_src1 << 15) | (FUNCT3_LBU << 12) | (reg_dest << 7) | OPCODE_LW;
  inst_size_t result = { inst_u32 };
  return result;
}

inst_size_t sw(uint32_t imm, uint32_t reg_src2, uint32_t reg_src1) {
  uint32_t top_imm = (imm << 5) >> 5;
  uint32_t bot_imm = 0b11111 & imm;
  uint32_t inst_u32 = (top_imm << 25) | (reg_src2 << 20) | (reg_src1 << 15) | (FUNCT3_SW << 12) | (bot_imm << 7) | OPCODE_SW;
  inst_size_t result = { inst_u32 };
  return result;
}

inst_size_t sb(uint32_t imm, uint32_t reg_src2, uint32_t reg_src1) {
  uint32_t top_imm = imm << 5;
  uint32_t bot_imm = 0b11111 & imm;
  uint32_t inst_u32 = (top_imm << 25) | (reg_src2 << 20) | (reg_src1 << 15) | (FUNCT3_SB << 12) | (bot_imm << 7) | OPCODE_SW;
  inst_size_t result = { inst_u32 };
  return result;
}

inst_size_t ebreak() {
  uint32_t inst_u32 = 1u << 20 | OPCODE_EBREAK;
  inst_size_t result = { inst_u32 };
  return result;
}

