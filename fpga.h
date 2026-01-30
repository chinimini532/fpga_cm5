#pragma once
#include <stdint.h>

#define MAX_FPGA_REG_ADDR 128

int  fpga_init(void);
int  fpga_read(uint8_t addr, uint8_t *data);
int  fpga_write(uint8_t addr, uint8_t data);
void fpga_close(void);

