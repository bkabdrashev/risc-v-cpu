#include <stdlib.h>
#include "golden_model.cpp"
#include <iostream>
#include <verilated.h>
#include <verilated_vcd_c.h>
#include "Vmain.h"

void clock_tick(Trigger* clock) {
  clock->prev = clock->curr;
  clock->curr ^= 1;
}

void compare(Vmain* dut, sCPU* gm, uint64_t sim_time) {
  if (dut->pc != gm->pc.v) {
    int dut_v = dut->pc;
    int gm_v = gm->pc.v;
    std::cout << "Test Failed at time " << sim_time << ". PC mismatch: dut->pc = " << dut_v << " vs gm->pc = " << gm_v << std::endl;
  }
  for (int i = 0; i < 4; i++) {
    if (dut->regs[i] != gm->regs[i].v) {
      int dut_v = dut->regs[i];
      int gm_v = gm->regs[i].v;
      std::cout << "Test Failed at time " << sim_time << ". regs["<<i<<"] mismatch: dut->regs["<<i<<"] = " << dut_v << " vs gm->regs["<<i<<"] = " << gm_v << std::endl;
    }
  }
}

int main(int argc, char** argv, char** env) {
  // NOTE: sCPU:
  inst_size_t insts[16] = {
    li(0, 10),
    li(1, 0),
    li(2, 0),
    li(3, 1),
    add(1, 1, 3),
    add(2, 2, 1),
    bner0(1, 4),
    bner0(3, 7),
  };
  sCPU cpu = {};
  for (int i = 0; i < 16; i++) {
    cpu.rom[i].v = insts[i].v;
  }
  Trigger clock = {};
  Trigger reset = {};

  // NOTE: Vmain
  Vmain* dut = new Vmain;

  const uint64_t max_sim_time = 100;
  uint64_t sim_time = 0;
  while (sim_time < max_sim_time) {
    cpu_eval(&cpu, clock, reset);
    dut->clk ^= 1;
    dut->eval();
    clock_tick(&clock);
    compare(dut, &cpu, sim_time);
    sim_time++;
  }
  if (cpu.pc.v != 7) {
    int got = cpu.pc.v;
    std::cout << "Test Failed:" << " PC expected 7 but got " << got << std::endl;
  }
  if (cpu.regs[0].v != 10) {
    int got = cpu.regs[0].v;
    std::cout << "Test Failed:" << " Reg0 expected 10 but got " << got << std::endl;
  }
  if (cpu.regs[1].v != 10) {
    int got = cpu.regs[1].v;
    std::cout << "Test Failed:" << " Reg1 expected 10 but got " << got << std::endl;
  }
  if (cpu.regs[2].v != 55) {
    int got = cpu.regs[2].v;
    std::cout << "Test Failed:" << " Reg2 expected 55 but got " << got << std::endl;
  }
  if (cpu.regs[3].v != 1) {
    int got = cpu.regs[3].v;
    std::cout << "Test Failed:" << " Reg[3] expected 1 but got " << got << std::endl;
  }

  exit(EXIT_SUCCESS);
}

