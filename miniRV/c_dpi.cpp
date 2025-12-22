#include "svdpi.h"
#define MEM_SIZE (128 * 1024 * 1024)
static char memory[MEM_SIZE];

extern "C" uint32_t mem_read(uint32_t address) {
  uint32_t result = 
    memory[address + 3].v << 24 | memory[address + 2].v << 16 |
    memory[address + 1].v <<  8 | memory[address + 0].v <<  0 ;
  return result;
}

extern "C" void mem_write(uint32_t address, uint32_t write_data, char wstrb) {
  uint8_t byte0 = (write_data >>  0) & 0xff;
  uint8_t byte1 = (write_data >>  8) & 0xff;
  uint8_t byte2 = (write_data >> 16) & 0xff;
  uint8_t byte3 = (write_data >> 24) & 0xff;
  if (wstrb[0]) memory[address + 0] = byte0;
  if (wstrb[1]) memory[address + 1] = byte1;
  if (wstrb[2]) memory[address + 2] = byte2;
  if (wstrb[3]) memory[address + 3] = byte3;
}

extern "C" void mem_reset() {
  memset(memory, 0, MEM_SIZE);
}
