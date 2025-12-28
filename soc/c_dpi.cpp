#include "svdpi.h"
#include "stdio.h"
#include "assert.h"

#define FLASH_SIZE (16*1024*1024)

uint8_t flash_mem[FLASH_SIZE];

extern "C" void flash_read(int32_t addr, int32_t* data) {
 *data = 
      flash_mem[addr + 3] << 24 | flash_mem[addr + 2] << 16 |
      flash_mem[addr + 1] <<  8 | flash_mem[addr + 0] <<  0 ;
;
}

extern "C" void flash_init(uint8_t* data, uint32_t size) {
  for (uint32_t i = 0; i < size; i++) {
    flash_mem[i] = data[i];
  }
  printf("[INFO] flash written: %u bytes\n", size);
}
