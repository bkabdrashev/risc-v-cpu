#include <stdlib.h>
#include <iostream>
#include <verilated.h>
#include <verilated_vcd_c.h>
#include "VminiRV.h"
#include "gm.cpp"

void clock_tick(Trigger* clock) {
  clock->prev = clock->curr;
  clock->curr ^= 1;
}

void reset_dut(VminiRV* dut) {
  dut->reset = 1;
  dut->eval();
  dut->reset = 0;
  dut->eval();
  dut->clk = 0;
}

bool compare_reg(uint64_t sim_time, const char* name, uint32_t dut_v, uint32_t gm_v) {
  if (dut_v != gm_v) {
  std::cout << "Test Failed at time " << sim_time << ". " << name << " mismatch: dut_v = " << dut_v << " vs gm_v = " << gm_v << std::endl;
    return false;
  }
  return true;
}

bool compare(VminiRV* dut, miniRV* gm, uint64_t sim_time) {
  bool result = true;
  result &= compare_reg(sim_time, "PC", dut->pc, gm->pc.v);
  result &= compare_reg(sim_time, "R0", dut->reg0, gm->regs[0].v);
  result &= compare_reg(sim_time, "R1", dut->reg1, gm->regs[1].v);
  result &= compare_reg(sim_time, "R2", dut->reg2, gm->regs[2].v);
  result &= compare_reg(sim_time, "R3", dut->reg3, gm->regs[3].v);
  result &= compare_reg(sim_time, "R4", dut->reg4, gm->regs[4].v);
  result &= compare_reg(sim_time, "R5", dut->reg5, gm->regs[5].v);
  result &= compare_reg(sim_time, "R6", dut->reg6, gm->regs[6].v);
  result &= compare_reg(sim_time, "R7", dut->reg7, gm->regs[7].v);
  return result;
}


int main(int argc, char** argv, char** env) {
  VminiRV *dut = new VminiRV;
  miniRV *gm = new miniRV;
  { // NOTE: set the rom
    const uint32_t n_inst = 28;
    inst_size_t insts[n_inst] = {
      li(10, 1),     // 0
      sw(4, 1, 0),
      lw(4, 0, 2)
    };

    dut->clk = 0;
    dut->rom_wen = 1;
    for (uint32_t i = 0; i < n_inst; i++) {
      dut->eval();
      dut->rom_wdata = insts[i].v;
      dut->rom_addr = i*4;
      dut->clk ^= 1;
      dut->eval();
      dut->clk ^= 1;

      // NOTE: Golden Model loads the same instructions
      gm->rom[i].v = insts[i].v;
    }
  }
  dut->rom_wen = 0;
  reset_dut(dut);

  Trigger clock = {};
  Trigger reset = {};
  for (uint64_t t = 0; t < 20; t++) {
    dut->eval();
    cpu_eval(gm, clock, reset);
    compare(dut, gm, t);
    dut->clk ^= 1;
    clock_tick(&clock);
  }
  reset_dut(dut);

  delete dut;
  delete gm;
  exit(EXIT_SUCCESS);
}


