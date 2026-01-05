#include "stdio.h"

#define INITIAL_PC  (0x3000'0000)
#define N_REGS      (16)

#define OPCODE_LUI          (0b0110111)
#define OPCODE_AUIPC        (0b0010111)
#define OPCODE_JAL          (0b1101111)
#define OPCODE_JALR         (0b1100111)
#define OPCODE_BRANCH       (0b1100011)
#define OPCODE_LOAD         (0b0000011)
#define OPCODE_STORE        (0b0100011)
#define OPCODE_CALC_IMM     (0b0010011)
#define OPCODE_CALC_REG     (0b0110011)
#define OPCODE_SYSTEM       (0b1110011)

#define ERROR_NOT_IMPLEMENTED (1010)
#define ERROR_INVALID_RANGE   (2020)

#define FUNCT3_JALR (0b000)

#define FUNCT3_BEQ  (0b000)
#define FUNCT3_BNE  (0b001)
#define FUNCT3_BLT  (0b100)
#define FUNCT3_BGE  (0b101)
#define FUNCT3_BLTU (0b110)
#define FUNCT3_BGEU (0b111)

#define FUNCT3_LB   (0b000)
#define FUNCT3_LH   (0b001)
#define FUNCT3_LW   (0b010)
#define FUNCT3_LHU  (0b101)
#define FUNCT3_LBU  (0b100)
#define FUNCT3_SB   (0b000)
#define FUNCT3_SH   (0b001)
#define FUNCT3_SW   (0b010)

#define FUNCT3_ADD  (0b000)
#define FUNCT3_SLL  (0b001)
#define FUNCT3_SLT  (0b010)
#define FUNCT3_SLTU (0b011)
#define FUNCT3_XOR  (0b100)
#define FUNCT3_SR   (0b101)
#define FUNCT3_OR   (0b110)
#define FUNCT3_AND  (0b111)

#define REG_SP (2)
#define REG_T0 (5)
#define REG_T1 (6)
#define REG_T2 (7)
#define REG_A0 (10)

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
  if (pos >= 32) {
    assert(0);
    return ERROR_INVALID_RANGE;
  }

  uint32_t mask = (1u << pos);
  return (bits & mask) >> pos;
}

uint32_t take_bits_range(uint32_t bits, uint32_t from, uint32_t to) {
  if (to >= 32 || from >= 32 || from > to) {
    assert(0);
    return ERROR_INVALID_RANGE;
  }

  uint32_t mask = ((1u << (to - from + 1)) - 1u) << from;
  return (bits & mask) >> from;
}

uint32_t lui(uint32_t imm, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 12) | (reg_dest << 7) | OPCODE_LUI;
  return inst_u32;
}

uint32_t auipc(uint32_t imm, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 12) | (reg_dest << 7) | OPCODE_AUIPC;
  return inst_u32;
}

uint32_t jal(uint32_t imm, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 12) | (reg_dest << 7) | OPCODE_JAL;
  return inst_u32;
}

uint32_t jalr(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 20) | (reg_src1 << 15) | (FUNCT3_JALR << 12) | (reg_dest << 7) | OPCODE_JALR;
  return inst_u32;
}

uint32_t branch_store(uint32_t imm, uint32_t reg_src2, uint32_t funct3, uint32_t reg_src1, uint32_t reg_dest, uint32_t opcode) {
  uint32_t top_imm = imm >> 5;
  uint32_t bot_imm = 0b11111 & imm;
  uint32_t inst_u32 = (top_imm << 25) | (reg_src2 << 20) | (reg_src1 << 15) | (funct3 << 12) | (bot_imm << 7) | opcode;
  return inst_u32;
}

uint32_t beq(uint32_t imm, uint32_t reg_src2, uint32_t reg_src1, uint32_t reg_dest) {
  return branch_store(imm, reg_src2, FUNCT3_BEQ, reg_src1, reg_dest, OPCODE_BRANCH);
}

uint32_t bne(uint32_t imm, uint32_t reg_src2, uint32_t reg_src1, uint32_t reg_dest) {
  return branch_store(imm, reg_src2, FUNCT3_BNE, reg_src1, reg_dest, OPCODE_BRANCH);
}

uint32_t blt(uint32_t imm, uint32_t reg_src2, uint32_t reg_src1, uint32_t reg_dest) {
  return branch_store(imm, reg_src2, FUNCT3_BLT, reg_src1, reg_dest, OPCODE_BRANCH);
}

uint32_t bge(uint32_t imm, uint32_t reg_src2, uint32_t reg_src1, uint32_t reg_dest) {
  return branch_store(imm, reg_src2, FUNCT3_BGE, reg_src1, reg_dest, OPCODE_BRANCH);
}

uint32_t bltu(uint32_t imm, uint32_t reg_src2, uint32_t reg_src1, uint32_t reg_dest) {
  return branch_store(imm, reg_src2, FUNCT3_BLTU, reg_src1, reg_dest, OPCODE_BRANCH);
}

uint32_t bgeu(uint32_t imm, uint32_t reg_src2, uint32_t reg_src1, uint32_t reg_dest) {
  return branch_store(imm, reg_src2, FUNCT3_BGEU, reg_src1, reg_dest, OPCODE_BRANCH);
}

uint32_t lb(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 20) | (reg_src1 << 15) | (FUNCT3_LB << 12) | (reg_dest << 7) | OPCODE_LOAD;
  return inst_u32;
}

uint32_t lh(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 20) | (reg_src1 << 15) | (FUNCT3_LH << 12) | (reg_dest << 7) | OPCODE_LOAD;
  return inst_u32;
}

uint32_t lw(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 20) | (reg_src1 << 15) | (FUNCT3_LW << 12) | (reg_dest << 7) | OPCODE_LOAD;
  return inst_u32;
}

uint32_t lbu(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 20) | (reg_src1 << 15) | (FUNCT3_LBU << 12) | (reg_dest << 7) | OPCODE_LOAD;
  return inst_u32;
}

uint32_t lhu(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 20) | (reg_src1 << 15) | (FUNCT3_LHU << 12) | (reg_dest << 7) | OPCODE_LOAD;
  return inst_u32;
}

uint32_t sb(uint32_t imm, uint32_t reg_src2, uint32_t reg_src1) {
  uint32_t top_imm = imm >> 5;
  uint32_t bot_imm = 0b11111 & imm;
  uint32_t inst_u32 = (top_imm << 25) | (reg_src2 << 20) | (reg_src1 << 15) | (FUNCT3_SB << 12) | (bot_imm << 7) | OPCODE_STORE;
  return inst_u32;
}

uint32_t sh(uint32_t imm, uint32_t reg_src2, uint32_t reg_src1) {
  uint32_t top_imm = imm >> 5;
  uint32_t bot_imm = 0b11111 & imm;
  uint32_t inst_u32 = (top_imm << 25) | (reg_src2 << 20) | (reg_src1 << 15) | (FUNCT3_SH << 12) | (bot_imm << 7) | OPCODE_STORE;
  return inst_u32;
}

uint32_t sw(uint32_t imm, uint32_t reg_src2, uint32_t reg_src1) {
  uint32_t top_imm = imm >> 5;
  uint32_t bot_imm = 0b11111 & imm;
  uint32_t inst_u32 = (top_imm << 25) | (reg_src2 << 20) | (reg_src1 << 15) | (FUNCT3_SW << 12) | (bot_imm << 7) | OPCODE_STORE;
  return inst_u32;
}

uint32_t addi(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 20) | (reg_src1 << 15) | (FUNCT3_ADD << 12) | (reg_dest << 7) | OPCODE_CALC_IMM;
  return inst_u32;
}

uint32_t slti(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 20) | (reg_src1 << 15) | (FUNCT3_SLT << 12) | (reg_dest << 7) | OPCODE_CALC_IMM;
  return inst_u32;
}

uint32_t sltiu(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 20) | (reg_src1 << 15) | (FUNCT3_SLTU << 12) | (reg_dest << 7) | OPCODE_CALC_IMM;
  return inst_u32;
}

uint32_t xori(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 20) | (reg_src1 << 15) | (FUNCT3_XOR << 12) | (reg_dest << 7) | OPCODE_CALC_IMM;
  return inst_u32;
}

uint32_t ori(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 20) | (reg_src1 << 15) | (FUNCT3_OR << 12) | (reg_dest << 7) | OPCODE_CALC_IMM;
  return inst_u32;
}

uint32_t andi(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 20) | (reg_src1 << 15) | (FUNCT3_AND << 12) | (reg_dest << 7) | OPCODE_CALC_IMM;
  return inst_u32;
}

uint32_t slli(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (imm << 20) | (reg_src1 << 15) | (FUNCT3_SLL << 12) | (reg_dest << 7) | OPCODE_CALC_IMM;
  return inst_u32;
}

uint32_t srli(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (0b0000000 << 25) | (imm << 20) | (reg_src1 << 15) | (FUNCT3_SR << 12) | (reg_dest << 7) | OPCODE_CALC_IMM;
  return inst_u32;
}

uint32_t srai(uint32_t imm, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (0b0100000 << 25) | (imm << 20) | (reg_src1 << 15) | (FUNCT3_SR << 12) | (reg_dest << 7) | OPCODE_CALC_IMM;
  return inst_u32;
}

uint32_t add(uint32_t reg_src2, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (0b0000000 << 25) | (reg_src2 << 20) | (reg_src1 << 15) | (FUNCT3_ADD << 12) | (reg_dest << 7) | OPCODE_CALC_REG;
  return inst_u32;
}

uint32_t sub(uint32_t reg_src2, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (0b0100000 << 25) | (reg_src2 << 20) | (reg_src1 << 15) | (FUNCT3_ADD << 12) | (reg_dest << 7) | OPCODE_CALC_REG;
  return inst_u32;
}

uint32_t sll(uint32_t reg_src2, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (0b0000000 << 25) | (reg_src2 << 20) | (reg_src1 << 15) | (FUNCT3_SLL << 12) | (reg_dest << 7) | OPCODE_CALC_REG;
  return inst_u32;
}

uint32_t slt(uint32_t reg_src2, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (0b0000000 << 25) | (reg_src2 << 20) | (reg_src1 << 15) | (FUNCT3_SLT << 12) | (reg_dest << 7) | OPCODE_CALC_REG;
  return inst_u32;
}

uint32_t sltu(uint32_t reg_src2, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (0b0000000 << 25) | (reg_src2 << 20) | (reg_src1 << 15) | (FUNCT3_SLTU << 12) | (reg_dest << 7) | OPCODE_CALC_REG;
  return inst_u32;
}

uint32_t vxor(uint32_t reg_src2, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (0b0000000 << 25) | (reg_src2 << 20) | (reg_src1 << 15) | (FUNCT3_XOR << 12) | (reg_dest << 7) | OPCODE_CALC_REG;
  return inst_u32;
}

uint32_t srl(uint32_t reg_src2, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (0b0000000 << 25) | (reg_src2 << 20) | (reg_src1 << 15) | (FUNCT3_SR << 12) | (reg_dest << 7) | OPCODE_CALC_REG;
  return inst_u32;
}

uint32_t sra(uint32_t reg_src2, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (0b0100000 << 25) | (reg_src2 << 20) | (reg_src1 << 15) | (FUNCT3_SR << 12) | (reg_dest << 7) | OPCODE_CALC_REG;
  return inst_u32;
}

uint32_t vor(uint32_t reg_src2, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (0b0000000 << 25) | (reg_src2 << 20) | (reg_src1 << 15) | (FUNCT3_OR << 12) | (reg_dest << 7) | OPCODE_CALC_REG;
  return inst_u32;
}

uint32_t vand(uint32_t reg_src2, uint32_t reg_src1, uint32_t reg_dest) {
  uint32_t inst_u32 = (0b0100000 << 25) | (reg_src2 << 20) | (reg_src1 << 15) | (FUNCT3_AND << 12) | (reg_dest << 7) | OPCODE_CALC_REG;
  return inst_u32;
}

uint32_t ebreak() {
  uint32_t inst_u32 = 1u << 20 | OPCODE_SYSTEM;
  return inst_u32;
}

// NOTE: pseudo instructions
uint32_t li(uint32_t imm, uint32_t reg_dest) {
  return addi(imm, 0, reg_dest);
}


struct InstInfo {
  uint8_t reg_dest;
  uint8_t reg_src1;
  uint8_t reg_src2;
  uint32_t imm;
  uint8_t opcode;
  uint8_t funct3;
  uint8_t funct7;
  uint32_t i_imm;
  uint32_t u_imm;
  uint32_t s_imm;
  uint32_t j_imm;
  uint32_t b_imm;
  uint8_t reg_write_enable;
};

InstInfo inst_info(uint32_t inst) {
  InstInfo out = {};
  out.opcode = take_bits_range(inst, 0, 6);
  out.reg_dest = take_bits_range(inst, 7, 11);
  out.funct3 = take_bits_range(inst, 12, 14);
  out.funct7 = take_bits_range(inst, 25, 31);
  out.reg_src1 = take_bits_range(inst, 15, 19);
  out.reg_src2 = take_bits_range(inst, 20, 24);
  uint8_t sign = take_bit(inst, 31);

  if (sign) out.i_imm = (~0u << 12) | take_bits_range(inst, 20, 31);
  else      out.i_imm = ( 0u << 12) | take_bits_range(inst, 20, 31);

  out.u_imm = sar32(take_bits_range(inst, 12, 31), 12);

  if (sign) out.s_imm = (~0u << 12) | take_bits_range(inst, 25, 31) << 5 | take_bits_range(inst, 7, 11);
  else      out.s_imm = ( 0u << 12) | take_bits_range(inst, 25, 31) << 5 | take_bits_range(inst, 7, 11);

  if (sign) out.j_imm = (~0u << 20) | take_bits_range(inst, 12, 19) << 12 | take_bit(inst, 20) << 11 | take_bits_range(inst, 21, 30) << 1 | 0b0;
  else      out.j_imm = ( 0u << 20) | take_bits_range(inst, 12, 19) << 12 | take_bit(inst, 20) << 11 | take_bits_range(inst, 21, 30) << 1 | 0b0;

  if (sign) out.b_imm = (~0u << 12) | take_bit(inst, 7) << 11 | take_bits_range(inst, 25, 30) << 5 | take_bits_range(inst, 8, 11) << 1 | 0b0;
  else      out.b_imm = ( 0u << 12) | take_bit(inst, 7) << 11 | take_bits_range(inst, 25, 30) << 5 | take_bits_range(inst, 8, 11) << 1 | 0b0;

  return out;
}

void print_instruction(uint32_t inst) {
  InstInfo info = inst_info(inst);
  switch (info.opcode) {
    case OPCODE_LUI: {
      printf("lui   imm=%5i rd=%2u\n", info.u_imm, info.reg_dest);
    } break;
    case OPCODE_AUIPC: {
      printf("auipc imm=%5i rd=%2u\n", info.u_imm, info.reg_dest);
    } break;
    case OPCODE_JAL: {
      printf("jal   imm=%5i rd=%2u\n", info.j_imm, info.reg_src1, info.reg_dest);
    } break;
    case OPCODE_JALR: {
      printf("jalr  imm=%5i rs1=%2u rd=%2u\n", info.i_imm, info.reg_src1, info.reg_dest);
    } break;
    case OPCODE_BRANCH: {
      switch (info.funct3) {
        case FUNCT3_BEQ:   printf("beq ");  break;
        case FUNCT3_BNE:   printf("bne ");  break;
        case FUNCT3_BLT:   printf("blt ");  break;
        case FUNCT3_BGE:   printf("bge ");  break;
        case FUNCT3_BLTU:  printf("bltu");  break;
        case FUNCT3_BGEU:  printf("bgeu");  break;
      }
      printf("  imm=%5i rs2=%2u rs1=%2u\n", info.b_imm, info.reg_src2, info.reg_src1);
    } break;
    case OPCODE_LOAD: {
      switch (info.funct3) {
        case FUNCT3_LB:   printf("lb ");  break;
        case FUNCT3_LH:   printf("lh ");  break;
        case FUNCT3_LW:   printf("lw ");  break;
        case FUNCT3_LBU:  printf("lbu");  break;
        case FUNCT3_LHU:  printf("lhu");  break;
      }
      printf("   imm=%5i rs1=%2u rd=%2u\n", info.i_imm, info.reg_src1, info.reg_dest);
    } break;
    case OPCODE_STORE: {
      switch (info.funct3) {
        case FUNCT3_SB:   printf("sb");  break;
        case FUNCT3_SH:   printf("sh");  break;
        case FUNCT3_SW:   printf("sw");  break;
      }
      printf("    imm=%5i rs2=%2u rs1=%2u\n", info.s_imm, info.reg_src2, info.reg_src1);
    } break;
    case OPCODE_CALC_IMM: {
      switch (info.funct3) {
        case FUNCT3_ADD:   printf("addi "); break;
        case FUNCT3_SLT:   printf("slti "); break;
        case FUNCT3_SLTU:  printf("sltui"); break;
        case FUNCT3_XOR:   printf("xori "); break;
        case FUNCT3_OR:    printf("ori  "); break;
        case FUNCT3_AND:   printf("andi "); break;
        case FUNCT3_SLL:   printf("slli "); break;
        case FUNCT3_SR:    {
          if (info.funct7 == 0)
                          printf("srli ");
          else            printf("srai ");
        } break;
      }
      printf(" imm=%5i rs1=%2u rd=%2u\n", info.i_imm, info.reg_src1, info.reg_dest);
    } break;
    case OPCODE_CALC_REG: {
      switch (info.funct3) {
        case FUNCT3_ADD:   {
          if (info.funct7 == 0)  printf("add");
          else printf("sub");
        } break;
        case FUNCT3_SLT:   printf("slt ");  break;
        case FUNCT3_SLTU:  printf("sltu"); break;
        case FUNCT3_XOR:   printf("xor ");  break;
        case FUNCT3_OR:    printf("or  ");   break;
        case FUNCT3_AND:   printf("and ");  break;
        case FUNCT3_SLL:   printf("sll ");  break;
        case FUNCT3_SR:    {
          if (info.funct7 == 0)
                          printf("srl ");
          else            printf("sra ");
        } break;
      }
      printf("  rs2=%u\t rs1=%u\t rd=%u\n", info.reg_src2, info.reg_src1, info.reg_dest);
    } break;
    case OPCODE_SYSTEM: {
      printf("ebreak\n");
    } break;

    default: { // NOT IMPLEMENTED
      not_implemented:
      printf("GM WARNING: not implemented:0x%x\n", inst);
    } break;
  }
}

uint32_t random_range(std::mt19937* gen, uint32_t ge, uint32_t lt) {
  std::uniform_int_distribution<uint32_t> dist(0, (1U << lt) - 1);

  return dist(*gen) % lt + ge;
}

uint32_t random_bits(std::mt19937* gen, uint32_t n) {
  std::uniform_int_distribution<uint32_t> dist(0, (1U << n) - 1);
  return dist(*gen);
}

uint32_t random_instruction(std::mt19937* gen) {
  uint32_t inst_id = random_range(gen, 0, 38);
  uint32_t inst = {};
  switch (inst_id) {
    case 0: { // LUI
      uint32_t imm = random_bits(gen, 20);
      uint32_t rd  = random_bits(gen, 4);
      inst = lui(imm, rd);
    } break;
    case 1: { // AUIPC
      uint32_t imm = random_bits(gen, 20);
      uint32_t rd  = random_bits(gen, 4);
      inst = auipc(imm, rd);
    } break;
    case 2: { // JAL
      uint32_t imm = random_bits(gen, 20);
      uint32_t rd  = random_bits(gen, 4);
      inst = jal(imm, rd);
    } break;
    case 3: { // JALR
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = jalr(imm, rs1, rd);
    } break;
    case 4: { // BEQ
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = beq(imm, rs2, rs1, rd);
    } break;
    case 5: { // bne
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = bne(imm, rs2, rs1, rd);
    } break;
    case 6: { // blt
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = blt(imm, rs2, rs1, rd);
    } break;
    case 7: { // bge
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = bge(imm, rs2, rs1, rd);
    } break;
    case 8: { // bltu
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = bltu(imm, rs2, rs1, rd);
    } break;
    case 9: { // bgeu
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = bgeu(imm, rs2, rs1, rd);
    } break;
    case 10: { // LB
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = lb(imm, rs1, rd);
    } break;
    case 11: { // lh
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = lh(imm, rs1, rd);
    } break;
    case 12: { // LW
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = lw(imm, rs1, rd);
    } break;
    case 13: { // LBU
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = lbu(imm, rs1, rd);
    } break;
    case 14: { // LHU
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = lhu(imm, rs1, rd);
    } break;
    case 15: { // SB
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1  = random_bits(gen, 4);
      inst = sb(imm, rs2, rs1);
    } break;

    case 16: { // SH
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1  = random_bits(gen, 4);
      inst = sh(imm, rs2, rs1);
    } break;
    case 17: { // SW
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1  = random_bits(gen, 4);
      inst = sw(imm, rs2, rs1);
    } break;
    case 18:{ // ADDI
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = addi(imm, rs1, rd);
    } break;
    case 19: { // sltI
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = slti(imm, rs1, rd);
    } break;
    case 20: { // sltiu
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = sltiu(imm, rs1, rd);
    } break;
    case 21: { // xori
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = xori(imm, rs1, rd);
    } break;
    case 22: { // ori
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = ori(imm, rs1, rd);
    } break;
    case 23: { // andi
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = andi(imm, rs1, rd);
    } break;
    case 24: {
      uint32_t imm = random_bits(gen, 5);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = slli(imm, rs1, rd);
    } break;
    case 25: {
      uint32_t imm = random_bits(gen, 5);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = srli(imm, rs1, rd);
    } break;
    case 26: {
      uint32_t imm = random_bits(gen, 5);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = srai(imm, rs1, rd);
    } break;
    case 27: { // ADD
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = add(rs2, rs1, rd);
    } break;
    case 28: { // sub
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = sub(rs2, rs1, rd);
    } break;
    case 29: { // sll
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = sll(rs2, rs1, rd);
    } break;
    case 30: { // slt
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = slt(rs2, rs1, rd);
    } break;
    case 31: { // sltu
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = sltu(rs2, rs1, rd);
    } break;
    case 32: { // xor
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = vxor(rs2, rs1, rd);
    } break;
    case 33: { // srl
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = srl(rs2, rs1, rd);
    } break;
    case 34: { // sra
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = sra(rs2, rs1, rd);
    } break;
    case 35: {
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = vor(rs2, rs1, rd);
    } break;
    case 36: {
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = vand(rs2, rs1, rd);
    } break;
    case 37: { // EBREAK
      inst = ebreak();
    } break;
  }
  return inst;
}

uint32_t random_instruction_no_jump(std::mt19937* gen) {
  uint32_t inst_id = random_range(gen, 0, 29);
  uint32_t inst = {};
  switch (inst_id) {
    case 0: { // LUI
      uint32_t imm = random_bits(gen, 20);
      uint32_t rd  = random_bits(gen, 4);
      inst = lui(imm, rd);
    } break;
    case 1: { // AUIPC
      uint32_t imm = random_bits(gen, 20);
      uint32_t rd  = random_bits(gen, 4);
      inst = auipc(imm, rd);
    } break;
    case 2: { // LB
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = lb(imm, rs1, rd);
    } break;
    case 3: { // lh
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = lh(imm, rs1, rd);
    } break;
    case 4: { // LW
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = lw(imm, rs1, rd);
    } break;
    case 5: { // LBU
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = lbu(imm, rs1, rd);
    } break;
    case 6: { // LHU
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = lhu(imm, rs1, rd);
    } break;
    case 7: { // SB
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1  = random_bits(gen, 4);
      inst = sb(imm, rs2, rs1);
    } break;
    case 8: { // SH
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1  = random_bits(gen, 4);
      inst = sh(imm, rs2, rs1);
    } break;
    case 9: { // SW
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1  = random_bits(gen, 4);
      inst = sw(imm, rs2, rs1);
    } break;
    case 10:{ // ADDI
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = addi(imm, rs1, rd);
    } break;
    case 11: { // sltI
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = slti(imm, rs1, rd);
    } break;
    case 12: { // sltiu
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = sltiu(imm, rs1, rd);
    } break;
    case 13: { // xori
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = xori(imm, rs1, rd);
    } break;
    case 14: { // ori
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = ori(imm, rs1, rd);
    } break;
    case 15: { // andi
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = andi(imm, rs1, rd);
    } break;
    case 16: {
      uint32_t imm = random_bits(gen, 5);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = slli(imm, rs1, rd);
    } break;
    case 17: {
      uint32_t imm = random_bits(gen, 5);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = srli(imm, rs1, rd);
    } break;
    case 18: {
      uint32_t imm = random_bits(gen, 5);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = srai(imm, rs1, rd);
    } break;
    case 19: { // ADD
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = add(rs2, rs1, rd);
    } break;
    case 20: { // sub
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = sub(rs2, rs1, rd);
    } break;
    case 21: { // sll
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = sll(rs2, rs1, rd);
    } break;
    case 22: { // slt
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = slt(rs2, rs1, rd);
    } break;
    case 23: { // sltu
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = sltu(rs2, rs1, rd);
    } break;
    case 24: { // xor
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = vxor(rs2, rs1, rd);
    } break;
    case 25: { // srl
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = srl(rs2, rs1, rd);
    } break;
    case 26: { // sra
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = sra(rs2, rs1, rd);
    } break;
    case 27: {
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = vor(rs2, rs1, rd);
    } break;
    case 28: {
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = vand(rs2, rs1, rd);
    } break;
  }
  return inst;
}

uint32_t random_instruction_no_mem_no_jump(std::mt19937* gen) {
  uint32_t inst_id = random_range(gen, 0, 21);
  uint32_t inst = {};
  switch (inst_id) {
    case 0: { // LUI
      uint32_t imm = random_bits(gen, 20);
      uint32_t rd  = random_bits(gen, 4);
      inst = lui(imm, rd);
    } break;
    case 1: { // AUIPC
      uint32_t imm = random_bits(gen, 20);
      uint32_t rd  = random_bits(gen, 4);
      inst = auipc(imm, rd);
    } break;
    case 2:{ // ADDI
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = addi(imm, rs1, rd);
    } break;
    case 3: { // sltI
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = slti(imm, rs1, rd);
    } break;
    case 4: { // sltiu
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = sltiu(imm, rs1, rd);
    } break;
    case 5: { // xori
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = xori(imm, rs1, rd);
    } break;
    case 6: { // ori
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = ori(imm, rs1, rd);
    } break;
    case 7: { // andi
      uint32_t imm = random_bits(gen, 12);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = andi(imm, rs1, rd);
    } break;
    case 8: {
      uint32_t imm = random_bits(gen, 5);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = slli(imm, rs1, rd);
    } break;
    case 9: {
      uint32_t imm = random_bits(gen, 5);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = srli(imm, rs1, rd);
    } break;
    case 10: {
      uint32_t imm = random_bits(gen, 5);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = srai(imm, rs1, rd);
    } break;
    case 11: { // ADD
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = add(rs2, rs1, rd);
    } break;
    case 12: { // sub
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = sub(rs2, rs1, rd);
    } break;
    case 13: { // sll
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = sll(rs2, rs1, rd);
    } break;
    case 14: { // slt
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = slt(rs2, rs1, rd);
    } break;
    case 15: { // sltu
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = sltu(rs2, rs1, rd);
    } break;
    case 16: { // xor
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = vxor(rs2, rs1, rd);
    } break;
    case 17: { // srl
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = srl(rs2, rs1, rd);
    } break;
    case 18: { // sra
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = sra(rs2, rs1, rd);
    } break;
    case 19: {
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = vor(rs2, rs1, rd);
    } break;
    case 20: {
      uint32_t rs2 = random_bits(gen, 4);
      uint32_t rs1 = random_bits(gen, 4);
      uint32_t rd  = random_bits(gen, 4);
      inst = vand(rs2, rs1, rd);
    } break;
  }
  return inst;
}

