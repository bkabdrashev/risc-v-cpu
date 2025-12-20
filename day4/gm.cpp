#include <cstdint>
#include <assert.h>
#include <iostream>

#define ERROR_NOT_IMPLEMENTED (0xEFEFEFEF)
#define ERROR_INVALID_RANGE   (0xEFEFEFEF)
#define OPCODE_BITS 7u
#define REG_INDEX_BITS 5u
#define ADDR_BITS 32u
#define REG_BITS 32u
#define BYTE 8u
#define INST_BITS 32u
#define ROM_SIZE (1u << 16)
#define N_REGS 16u

struct bit {
  uint32_t v : 1;
};
struct Trigger {
  uint32_t prev : 1;
  uint32_t curr : 1;
};
struct opcode_size_t {
  uint32_t v : OPCODE_BITS;
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
struct byte_t {
  uint8_t v : BYTE;
};
struct inst_size_t {
  uint32_t v : REG_BITS;
};

bit ZERO_BIT = {};
reg_index_t REG_0 = {};
reg_size_t REG_VALUE_0 = {};



struct miniRV {
  addr_size_t pc;
  reg_size_t regs[N_REGS];
  reg_size_t rom[ROM_SIZE];
};

uint32_t take_bit(uint32_t bits, uint32_t pos) {
  if (pos > 7) {
    assert(0);
    return ERROR_INVALID_RANGE;
  }

  uint32_t mask = (1u << pos);
  return (bits & mask) >> pos;
}

uint32_t take_bits_range(uint32_t bits, uint32_t from, uint32_t to) {
  if (to > REG_BITS-1 || from > REG_BITS-1 || from > to) {
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

reg_size_t alu_eval(opcode_size_t opcode, reg_size_t rdata1, reg_size_t rdata2, reg_size_t imm) {
  reg_size_t result = {};
  if (opcode.v == 0b0010011) {
    // ADDI
    result.v = rdata1.v + imm.v;
  }
  else if (opcode.v == 0b0110011) {
    result.v = rdata1.v + rdata2.v;
  }
  else {
    result.v = ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

inst_size_t rom_eval(miniRV* cpu, addr_size_t addr) {
  inst_size_t result = {};
  result.v = cpu->rom[addr.v / 4].v;
  return result;
}

addr_size_t pc_eval(miniRV* cpu, Trigger clock, Trigger reset, addr_size_t in_addr, bit is_jump) {
  int a = is_jump.v;
  if (is_positive_edge(reset)) {
    cpu->pc.v = 0;
  }
  else if (is_positive_edge(clock)) {
    if (is_jump.v) {
      cpu->pc.v = in_addr.v;
    }
    else {
      cpu->pc.v+=4;
    }
  }
  return cpu->pc;
}

struct RF_out {
  reg_size_t rdata1;
  reg_size_t rdata2;
  reg_size_t regs[N_REGS];
};

RF_out rf_read(miniRV* cpu, reg_index_t reg_src1, reg_index_t reg_src2) {
  RF_out out = {};
  out.rdata1.v = cpu->regs[reg_src1.v].v;
  out.rdata2.v = cpu->regs[reg_src2.v].v;
  for (int i = 0; i < N_REGS; i++) {
    out.regs[i].v = cpu->regs[i].v;
  }
  return out;
}

void rf_write(miniRV* cpu, Trigger clock, Trigger reset, bit write_enable, reg_index_t reg_dest, reg_size_t write_data) {
  if (is_positive_edge(reset)) {
    for (uint32_t i = 0; i < N_REGS; i++) {
      cpu->regs[i].v = 0;
    }
  }
  else if (is_positive_edge(clock)) {
    if (write_enable.v) {
      cpu->regs[reg_dest.v] = write_data;
    }
  }
}

struct Dec_out {
  reg_index_t reg_dest;
  reg_index_t reg_src1;
  reg_index_t reg_src2;
  reg_size_t imm;
  opcode_size_t opcode;
  bit write_enable;
};

Dec_out dec_eval(inst_size_t inst) {
  Dec_out out = {};
  out.opcode.v = take_bits_range(inst.v, 0, 6);
  out.reg_dest.v = take_bits_range(inst.v, 7, 11);
  out.reg_src1.v = take_bits_range(inst.v, 15, 19);
  out.reg_src2.v = take_bits_range(inst.v, 20, 24);
  bit sign = take_bit(inst.v, 31, 31);
  if (out.opcode.v == 0b0010011) {
    // ADDI
    if (sign.v) out.imm.v = take_bits_range(inst.v, 20, 31);
    else        out.imm.v = -1 | take_bits_range(inst.v, 20, 31);
    out.write_enable.v = 1;
  }
  else if (out.opcode.v == 0b1100111) {
    // JALR
    if (sign.v) out.imm.v = take_bits_range(inst.v, 20, 31);
    else        out.imm.v = -1 | take_bits_range(inst.v, 20, 31);
    out.write_enable.v = 1;
  }
  else if (out.opcode.v == 0b0110011) {
    // ADD
    if (sign.v) out.imm.v = take_bits_range(inst.v, 12, 31);
    else        out.imm.v = -1 | take_bits_range(inst.v, 12, 31);
    out.write_enable.v = 1;
  }
  else if (out.opcode.v == 0b0110111) {
    // LUI
    out.imm.v = take_bits_range(inst.v, 12, 31) << 12;
    out.write_enable.v = 1;
  }
  else {
    out.imm.v = 0;
    out.write_enable.v = 0;
  }
  return out;
}

void cpu_eval(miniRV* cpu, Trigger clock, Trigger reset) {
  inst_size_t inst = rom_eval(cpu, cpu->pc);
  Dec_out dec_out = dec_eval(inst);
  RF_out rf_out = rf_read(cpu, dec_out.reg_src1, dec_out.reg_src2);
  reg_size_t alu_out = alu_eval(dec_out.opcode, rf_out.rdata1, rf_out.rdata2, dec_out.imm);

  addr_size_t in_addr = {};
  bit is_jump = {};
  reg_size_t write_data = {};
  if (dec_out.opcode.v == 0b0010011) {
    // ADDI
    write_data = alu_out;
    in_addr.v = 0;
    is_jump.v = 0;
  }
  else if (dec_out.opcode.v == 0b1100111) {
    // JALR
    in_addr.v = (rf_out.rdata1.v + dec_out.imm.v) & ~1;
    write_data.v= cpu->pc.v + 4;
    is_jump.v = 1;
  }
  else if (dec_out.opcode.v == 0b0110011) {
    // ADD
    write_data.v = alu_out.v;
    in_addr.v = 0;
    is_jump.v = 0;
  }
  else if (dec_out.opcode.v == 0b0110111) {
    // LUI
    write_data.v = dec_out.imm.v;
    in_addr.v = 0;
    is_jump.v = 0;
  }
  else {
    in_addr.v = 0;
    write_data.v = 0;
    is_jump.v = 0;
  }

  rf_write(cpu, clock, reset, dec_out.write_enable, dec_out.reg_dest, alu_out);
  pc_eval(cpu, clock, reset, in_addr, is_jump);
}

inst_size_t addi(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 20) | (reg_src1 << 15) | (0b000 << 12) | (reg_dest << 7) | 0b0010011;
  inst_size_t result = { inst_u32 };
  return result;
}

inst_size_t li(uint32_t imm, uint32_t reg_dest) {
  return addi(imm, 0, reg_dest);
}

inst_size_t jalr(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 20) | (reg_src1 << 15) | (0b000 << 12) | (reg_dest << 7) | 0b1100111;
  inst_size_t result = { inst_u32 };
  return result;
}

inst_size_t add(uint32_t reg_src2, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (0b0000000 << 25) | (reg_src2 << 20) | (reg_src1 << 15) | (0b000 << 12) | (reg_dest << 7) | 0b0110011;
  inst_size_t result = { inst_u32 };
  return result;
}

inst_size_t lui(uint32_t imm, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 12) | (reg_dest << 7) | 0b0110111;
  inst_size_t result = { inst_u32 };
  return result;
}
