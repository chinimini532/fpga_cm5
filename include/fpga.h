#ifndef __NGT_FPGA_H__
#define __NGT_FPGA_H__

#include <stdint.h>

#define MAX_FPGA_REG_ADDR  	0x80

/* common */

#define FPGA_REG_00			0x00 
#define FPGA_REG_01			0x01 
#define FPGA_REG_02			0x02 
#define FPGA_REG_03			0x03 
#define FPGA_REG_04			0x04 
#define FPGA_REG_05			0x05
#define FPGA_REG_06			0x06
#define FPGA_REG_07			0x07
#define FPGA_REG_08			0x08
#define FPGA_REG_09			0x09
#define FPGA_REG_0A			0x0A
#define FPGA_REG_0B			0x0B
#define FPGA_REG_0C			0x0C
#define FPGA_REG_0D			0x0D
#define FPGA_REG_0E			0x0E
#define FPGA_REG_0F			0x0F

#define FPGA_REG_10			0x10
#define FPGA_REG_11			0x11 
#define FPGA_REG_12			0x12 
#define FPGA_REG_13			0x13 
#define FPGA_REG_14			0x14 
#define FPGA_REG_15			0x15
#define FPGA_REG_16			0x16
#define FPGA_REG_17			0x17
#define FPGA_REG_18			0x18
#define FPGA_REG_19			0x19
#define FPGA_REG_1A			0x1A
#define FPGA_REG_1B			0x1B
#define FPGA_REG_1C			0x1C
#define FPGA_REG_1D			0x1D 
#define FPGA_REG_1E			0x1E 
#define FPGA_REG_1F			0x1F 

typedef struct fpga_info {
	int         fd;
	uint32_t    mode;
	uint32_t    speed;
	uint8_t     bits;
} fpga_info_t;

#ifdef __cplusplus
extern "C" {
#endif
int fpga_init(); 
void fpga_close(); 
int fpga_read(uint8_t addr, uint8_t *data);
int fpga_write(uint8_t addr, uint8_t data);
#ifdef __cplusplus
}
#endif

#endif  /* __NGT_FPGA_H__ */
