#include <verilated.h>
#include <verilated_vcd_c.h>
#include "VysyxSoCTop.h"

typedef VysyxSoCTop CPU;

#include <stdio.h>   // fopen, fseek, ftell, fread, fclose, fprintf
#include <stdlib.h>  // malloc, free
#include <stdint.h>  // uint8_t
#include <stddef.h>  // size_t
#include <limits.h>  // SIZE_MAX

#include "riscv.cpp"

int read_bin_file(const char* path, uint8_t** out_data, size_t* out_size) {
  if (!out_data || !out_size) return 0;

  *out_data = NULL;
  *out_size = 0;

  FILE* f = fopen(path, "rb");
  if (!f) {
    fprintf(stderr, "Error: Could not open %s\n", path);
    return 0;
  }

  if (fseek(f, 0, SEEK_END) != 0) {
    fprintf(stderr, "Error: Could not seek to end of file.\n");
    fclose(f);
    return 0;
  }

  long len = ftell(f);
  if (len < 0) { // ftell failure
    fprintf(stderr, "Error: Could not determine file size.\n");
    fclose(f);
    return 0;
  }

  if (len == 0) { // empty file is not an error
    fclose(f);
    return 1;
  }

  // Ensure it fits into size_t
  if ((unsigned long long)len > (unsigned long long)SIZE_MAX) {
    fprintf(stderr, "Error: File too large to allocate.\n");
    fclose(f);
    return 0;
  }

  if (fseek(f, 0, SEEK_SET) != 0) {
    fprintf(stderr, "Error: Could not seek to start of file.\n");
    fclose(f);
    return 0;
  }

  size_t size = (size_t)len;
  uint8_t* buf = (uint8_t*)malloc(size);
  if (!buf) {
    fprintf(stderr, "Error: Out of memory.\n");
    fclose(f);
    return 0;
  }

  size_t read = fread(buf, 1, size, f);
  fclose(f);

  if (read != size) {
    fprintf(stderr, "Error: Could not read file.\n");
    free(buf);
    return 0;
  }

  *out_data = buf;
  *out_size = size;
  return 1;
}

extern "C" void flash_init(uint8_t* data, uint32_t size);

struct TestBench {
  VerilatedContext* contextp;
  CPU* cpu;
  VerilatedVcdC* trace;
  uint64_t cycles;

  uint32_t  n_insts;
  uint32_t* insts;
};

void print_all_instructions(TestBench* tb) {
  for (uint32_t i = 0; i < tb->n_insts; i++) {
    printf("[0x%x], (0x%x)\t", 4*i, tb->insts[i]);
    print_instruction(tb->insts[i]);
  }
}

void cycle(TestBench* tb) {
  tb->cpu->eval();
  tb->trace->dump(tb->cycles++);
  tb->cpu->clock ^= 1;

  tb->cpu->eval();
  tb->trace->dump(tb->cycles++);
  tb->cpu->clock ^= 1;
}

int main(int argc, char** argv, char** env) {
  TestBench* tb = new TestBench;
  tb->contextp = new VerilatedContext;
  tb->cpu = new CPU;
  Verilated::traceEverOn(true);
  tb->trace = new VerilatedVcdC;
  tb->cpu->trace(tb->trace, 5);
  tb->trace->open("waveform.vcd");
  uint64_t counter = 0;
  uint8_t* data = NULL;
  size_t   size = 0;
  // uint32_t insts[5] = {
  //   lui(0x80000, REG_SP),     // 0
  //   li(0x1234, REG_T0),         // 4
  //   sw( 0x4, REG_T0, REG_SP), // 8
  //   // li(0x34, REG_T1),         // C
  //   // sw( 0x5, REG_T1, REG_SP), // 10
  //   lw( 0x4, REG_SP, REG_T2), // 14
  //   ebreak()
  // };
  // uint32_t insts[7] = {
  //   lui(0x80000, REG_SP),     // 0
  //   li(0x12, REG_T0),         // 4
  //   sb( 0x4, REG_T0, REG_SP), // 8
  //   li(0x34, REG_T1),         // C
  //   sb( 0x5, REG_T1, REG_SP), // 10
  //   lw( 0x4, REG_SP, REG_T2), // 14
  //   ebreak()
  // };
  // tb->insts = insts;
  // tb->n_insts = 5;
  // print_all_instructions(tb);
  // data = (uint8_t*)insts;
  // size = tb->n_insts*4;
  /*
   0:	555550b7          	lui	x1,0x55555
   4:	00108093          	addi	x1,x1,1 # 0x55555001
   8:	80000137          	lui	x2,0x80000
   c:	00410113          	addi	x2,x2,4 # 0x80000004
  10:	00112023          	sw	x1,0(x2)
  14:	00012183          	lw	x3,0(x2)
  18:	00100073          	ebreak
    */
  // read_bin_file("./bin/code2.bin", &data, &size);
  // read_bin_file("./bin/hello-minirv-ysyxsoc.bin", &data, &size);
  // read_bin_file("./bin/dummy-minirv-ysyxsoc.bin", &data, &size);
  read_bin_file("./bin/dummy-flash-ysyxsoc.bin", &data, &size);
  flash_init(data, (uint32_t)size);

  uint64_t max_sim_time = 0;

  tb->cpu->reset = 1;
  tb->cpu->clock = 0;
  for (uint64_t i = 0; i < 10; i++) {
    cycle(tb);
  }
  tb->cpu->reset = 0;

  printf("start\n");
  for (uint64_t t = 0; !max_sim_time || t < max_sim_time; t++) {
    cycle(tb);
    if (tb->contextp->gotFinish()) break;
  }
  printf("finish\n");

  tb->trace->close();
  int exit_code = EXIT_SUCCESS;
  return exit_code;
}


