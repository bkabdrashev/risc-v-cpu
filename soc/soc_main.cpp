#include <verilated.h>
#include <verilated_vcd_c.h>
#include "VysyxSoCTop.h"
#include "VysyxSoCTop___024root.h"

typedef VysyxSoCTop SoC;

#include <stdio.h>   // fopen, fseek, ftell, fread, fclose, fprintf
#include <stdlib.h>  // malloc, free
#include <stdint.h>  // uint8_t
#include <stddef.h>  // size_t
#include <limits.h>  // SIZE_MAX

#include "riscv.cpp"
#include "gcpu.cpp"

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

extern void flash_init(uint8_t* data, uint32_t size);

struct Vcpu {
  uint8_t&  ebreak;
  uint32_t& pc;
  uint8_t&  is_done_instruction;
  VlUnpacked<uint32_t, 16>&  regs;
  VlUnpacked<uint16_t, 16777216>& mem;
};

struct TestBenchConfig {
  bool is_trace       = false;
  char* trace_file    = NULL;
  bool is_cycles      = false;
  bool is_bin         = false;
  char* bin_file      = NULL;
  uint64_t max_cycles = 0;

  bool is_diff        = false;
  bool is_random      = false;
  uint64_t max_tests  = 0;
  uint32_t  n_insts   = 0;
};

struct TestBench {
  bool  is_trace;
  char* trace_file;
  bool  is_cycles;
  bool  is_bin;
  char* bin_file;
  uint64_t max_cycles;

  bool is_diff;
  bool is_random;
  uint64_t max_tests;

  VerilatedContext* contextp;
  SoC* soc;
  VerilatedVcdC* trace;
  uint64_t cycles;

  Vcpu* vcpu;
  Gcpu* gcpu;

  size_t    file_size;
  uint32_t  n_insts;
  uint32_t* insts;
};

void print_all_instructions(TestBench* tb) {
  for (uint32_t i = 0; i < tb->n_insts; i++) {
    printf("[0x%x], (0x%x)\t", 4*i, tb->insts[i]);
    print_instruction(tb->insts[i]);
  }
}

void tick(TestBench* tb) {
  tb->soc->eval();
  if (tb->is_trace) tb->trace->dump(tb->cycles);
  if (tb->is_cycles && tb->cycles % 1'000'000 == 0) printf("[INFO] cycles: %lu\n", tb->cycles);
  tb->cycles++;
  tb->soc->clock ^= 1;
}

void cycle(TestBench* tb) {
  tick(tb);
  tick(tb);
}

void v_reset(TestBench* tb) {
  printf("[INFO] reset\n");
  tb->soc->reset = 1;
  tb->soc->clock = 0;
  for (uint64_t i = 0; i < 10; i++) {
    cycle(tb);
  }
  tb->soc->reset = 0;
}


void fetch_exec(TestBench* tb) {
  while (1) {
    cycle(tb);
    if (tb->max_cycles && tb->cycles >= tb->max_cycles) break;
    if (tb->vcpu->ebreak) break;
    if (tb->vcpu->is_done_instruction) break;
  }
}

bool compare_reg(uint64_t sim_time, const char* name, uint32_t v, uint32_t g) {
  if (v != g) {
    printf("[FAILED] Test Failed at time %lu. %s mismatch: v = 0x%x vs g = 0x%x\n", sim_time, name, v, g);
    return false;
  }
  return true;
}

bool compare_mem(uint64_t sim_time, uint32_t address, uint32_t v, uint32_t g) {
  if (v != g) {
    printf("[FAILED] Test Failed at time %lu. 0x%x mismatch: v = %i vs g = %i\n", sim_time, address, v, g);
    return false;
  }
  return true;
}

bool compare(TestBench* tb) {
  bool result = true;
  result &= compare_reg(tb->cycles, "EBREAK", tb->vcpu->ebreak, tb->gcpu->ebreak);
  result &= compare_reg(tb->cycles, "PC",     tb->vcpu->pc,     tb->gcpu->pc);
  for (uint32_t i = 0; i < N_REGS; i++) {
    char digit0 = i%10 + '0';
    char digit1 = i/10 + '0';
    char name[] = {'x', digit1, digit0, '\0'};
    result &= compare_reg(tb->cycles, name, tb->vcpu->regs[i], tb->gcpu->regs[i]);
  }
  // result &= memcmp(tb->gcpu->flash, tb->vflash, FLASH_SIZE) == 0;
  result &= memcmp(tb->gcpu->mem, &tb->vcpu->mem.m_storage[0], MEM_SIZE) == 0;
  if (!result) {
    for (uint32_t i = 0; i < MEM_SIZE; i++) {
      uint32_t v = ((uint8_t*)tb->vcpu->mem.m_storage)[i];
      uint32_t g = tb->gcpu->mem[i];
      result &= compare_mem(tb->cycles, i, v, g);
    }
    // for (uint32_t i = 0; i < FLASH_SIZE; i++) {
    //   uint32_t v = tb->vflash[i];
    //   uint32_t g = tb->gcpu->flash[i];
    //   result &= compare_mem(tb->cycles, i, v, g);
    // }
  }
  return result;
}

bool test_instructions(TestBench* tb) {
  tb->cycles = 0;
  v_reset(tb);
  g_reset(tb->gcpu);

  flash_init((uint8_t*)tb->insts, tb->file_size);
  if (tb->is_diff) {
    g_flash_init(tb->gcpu, (uint8_t*)tb->insts, tb->file_size);
  }
  bool is_test_success = true;
  while (1) {
    fetch_exec(tb);
    if (tb->vcpu->ebreak) {
      printf("[INFO] vcpu ebreak\n");
      if (tb->vcpu->regs[10] != 0) {
        printf("[FAILED] test is not successful: vcpu returned %u\n", tb->vcpu->regs[10]);
        is_test_success=false;
      }
    }

    if (tb->is_diff) {
      uint32_t pc = tb->gcpu->pc;
      uint32_t inst = g_mem_read(tb->gcpu, tb->gcpu->pc);
      uint8_t ebreak = cpu_eval(tb->gcpu);
      if (ebreak) {
        printf("[INFO] gcpu ebreak\n");
        if (tb->gcpu->regs[10] != 0) {
          printf("[FAILED] test is not successful: gcpu returned %u\n", tb->gcpu->regs[10]);
          is_test_success=false;
        }
      }
      is_test_success &= compare(tb);
      if (!is_test_success) {
        printf("[%x] pc=%x inst: [0x%x] ", tb->cycles, pc, inst);
        print_instruction(inst);
        break;
      }
      if (tb->vcpu->ebreak && tb->gcpu->ebreak) break;
    }
    else if (tb->vcpu->ebreak) {
      break;
    }

    if (tb->max_cycles && tb->cycles >= tb->max_cycles) {
      printf("[FAILED] test is not successful: timeout %u/%u\n", tb->cycles, tb->max_cycles);
      is_test_success=false;
      break;
    }
  }
  printf("[INFO] finished:%u cycles\n", tb->cycles);
  return is_test_success;
}

bool test_bin(TestBench* tb) {
  uint8_t* data = NULL; size_t size = 0;
  printf("[INFO] read file %s\n", tb->bin_file);
  int ok = read_bin_file(tb->bin_file, &data, &size);
  if (!ok) return false;

  tb->file_size = size;
  tb->n_insts = size/4;
  tb->insts = (uint32_t*)data;

  return test_instructions(tb);
}

bool test_random(TestBench* tb) {
  bool is_tests_success = true;
  printf("[TODO] random tests are not implemented\n");
  return is_tests_success;
}

static void usage(const char* prog) {
  fprintf(stderr,
    "Usage:\n"
    "  %s [trace <path>] [cycles] [max <number>] bin <path>\n",
    prog 
  );
}

static int streq(const char* a, const char* b) {
  return a && b && strcmp(a, b) == 0;
}

TestBench new_testbench(TestBenchConfig config) {
  TestBench tb = {
    .is_trace   = config.is_trace,
    .trace_file = config.trace_file,
    .is_cycles  = config.is_cycles,
    .is_bin     = config.is_bin,
    .bin_file   = config.bin_file,
    .max_cycles = config.max_cycles,
    .is_diff    = config.is_diff,
    .is_random  = config.is_random,
    .max_tests  = config.max_tests,
    .n_insts    = config.n_insts,
  };

  tb.soc = new SoC;
  tb.vcpu = new Vcpu {
    .ebreak              = tb.soc->rootp->ysyxSoCTop__DOT__dut__DOT__asic__DOT__cpu__DOT__u_cpu__DOT__ebreak,
    .pc                  = tb.soc->rootp->ysyxSoCTop__DOT__dut__DOT__asic__DOT__cpu__DOT__u_cpu__DOT__pc,
    .is_done_instruction = tb.soc->rootp->ysyxSoCTop__DOT__dut__DOT__asic__DOT__cpu__DOT__u_cpu__DOT__is_done_instruction,
    .regs                = tb.soc->rootp->ysyxSoCTop__DOT__dut__DOT__asic__DOT__cpu__DOT__u_cpu__DOT__u_rf__DOT__regs,
    .mem                 = tb.soc->rootp->ysyxSoCTop__DOT__dut__DOT__sdram__DOT__mem_ext__DOT__Memory,
    // .uart                =
  };

  tb.gcpu = new Gcpu;

  tb.contextp = new VerilatedContext;

  if (tb.is_trace) {
    Verilated::traceEverOn(true);
    tb.trace = new VerilatedVcdC;
    tb.soc->trace(tb.trace, 5);
    tb.trace->open(tb.trace_file);
  }
  return tb;
}

void delete_testbench(TestBench tb) {
  if (tb.n_insts) {
    free(tb.insts);
  }
  if (tb.is_trace) {
    tb.trace->close();
    delete tb.trace;
  }
  delete tb.vcpu;
  delete tb.gcpu;
  delete tb.soc;
  delete tb.contextp;
}

int main(int argc, char** argv, char** env) {
  int exit_code = EXIT_SUCCESS;

  if (argc < 2) {
    usage(argv[0]);
    exit_code = EXIT_FAILURE;
    goto exit_label;
  }
  else {
    TestBenchConfig config = {};
    int curr_arg = 1;
    while (curr_arg < argc) {
      char* mode = argv[curr_arg++];
      if (streq(mode, "trace")) {
        if (config.is_trace) {
          fprintf(stderr, "[ERROR]: second trace\n");
          usage(argv[0]);
          exit_code = EXIT_FAILURE;
          goto exit_label;
        }
        if (curr_arg >= argc) {
          fprintf(stderr, "[ERROR]: 'trace' requires a <path>\n");
          usage(argv[0]);
          exit_code = EXIT_FAILURE;
          goto exit_label;
        }
        config.trace_file = argv[curr_arg++];
        config.is_trace = true;
      }
      else if (streq(mode, "diff")) {
        config.is_diff = true;
      }
      else if (streq(mode, "cycles")) {
        config.is_cycles = true;
      }
      else if (streq(mode, "max")) {
        if (config.max_cycles) {
          fprintf(stderr, "[ERROR]: second max cycles\n");
          usage(argv[0]);
          exit_code = EXIT_FAILURE;
          goto exit_label;
        }
        if (curr_arg >= argc) {
          fprintf(stderr, "[ERROR]: 'max' requires a <number>\n");
          usage(argv[0]);
          exit_code = EXIT_FAILURE;
          goto exit_label;
        }
        config.max_cycles = atoi(argv[curr_arg++]);
      }
      else if (streq(mode, "random")) {
        if (config.is_random) {
          fprintf(stderr, "[ERROR]: second random is not supported\n");
          usage(argv[0]);
          exit_code = EXIT_FAILURE;
          goto exit_label;
        }
        if (curr_arg+1 >= argc) {
          fprintf(stderr, "[ERROR]: 'random' requires a <number> <number>\n");
          usage(argv[0]);
          exit_code = EXIT_FAILURE;
          goto exit_label;
        }
        config.is_random = true;
        config.max_tests = atoi(argv[curr_arg++]);
        config.n_insts   = atoi(argv[curr_arg++]);
      }
      else if (streq(mode, "bin")) {
        if (config.is_bin) {
          fprintf(stderr, "[ERROR]: second bin is not supported\n");
          usage(argv[0]);
          exit_code = EXIT_FAILURE;
          goto exit_label;
        }
        if (curr_arg >= argc) {
          fprintf(stderr, "[ERROR]: 'bin' requires a <path>\n");
          usage(argv[0]);
          exit_code = EXIT_FAILURE;
          goto exit_label;
        }
        config.is_bin = true;
        config.bin_file = argv[curr_arg++];
      }
      else {
        fprintf(stderr, "[ERROR]: unknown mode '%s'\n", mode);
        usage(argv[0]);
        exit_code = EXIT_FAILURE;
      }
    }
    TestBench tb = new_testbench(config);
    tb.gcpu->tb = &tb;


    if (tb.is_bin && tb.is_random) {
      printf("[WARNING] bin test and random test together are not supported: doing only bin test\n");
    }
    if (tb.is_bin) {
      bool result = test_bin(&tb);
      if (!result) exit_code = EXIT_FAILURE;
    }
    else if (tb.is_random) {
      bool result = test_random(&tb);
      if (!result) exit_code = EXIT_FAILURE;
    }
    delete_testbench(tb);
  }
  
exit_label:
  return exit_code;
}


