#ifndef __RASP_GPIO_H__
#define __RASP_GPIO_H__


#define RASP_INPUT		0
#define RASP_OUTPUT		1
#define RASP_ALT		2

// pin-number           BCM     Description
#define BCM_GPIO_2     2      // INIT_B
#define BCM_GPIO_3     3      // DONE
#define BCM_GPIO_4     4      // PROGRAM_B
#define BCM_GPIO_5     5      // DIN
#define BCM_GPIO_6     6      // CCLK

#define FPGA_CCLK       BCM_GPIO_6
#define FPGA_INIT       BCM_GPIO_2
#define FPGA_DONE       BCM_GPIO_3
#define FPGA_DIN        BCM_GPIO_5
#define FPGA_PROGRAM    BCM_GPIO_4


#endif	// __RASP_GPIO_H__
