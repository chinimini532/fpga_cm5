#include <stdio.h>
#include <stdint.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "log_print.h"
#include "fpga.h"
#include "spi.h"

int spi_fd;

int fpga_open()
{
	int fd = spi_open(0);
	if(fd < 0) {
		NGT_LOG(DERR, "SPI device open error\n");
		printf("SPI device open error\n");  /* console log for util/fpga */
		return -1;
	}
	return fd;
}

int fpga_init()
{
	int ret = 0;
	fpga_info_t fpga_info;
	fpga_info.bits  = 8;
	fpga_info.speed = 500000;
	fpga_info.mode  = SPI_MODE_0;  /* REF : KENEL/include/linux/spi/spi.h */

	/* KAP-RP  : SPI_MODE_0 = 0 --> SELECTED
	 * KTMO    : SPI_MODE_1 = SPI_CPHA(0x01 - clock phase)
	 */

	spi_fd = fpga_open();
	if(spi_fd < 0)  return -1;

	ret = spi_init(0, fpga_info.bits, fpga_info.speed, fpga_info.mode, 0);
	if(ret < 0)  printf("SPI device init error\n");  /* console log for util/fpga */

	return ret;
}

void fpga_close()
{
	spi_close(0);
}

/* KAP-RP : 2 bytes operation --> SELECTED
 * KTMO   : 3 bytes operation 
 */
int fpga_read(uint8_t addr, uint8_t *data)
{
	int ret;
	uint8_t tx_buff[2] = {0, };
	uint8_t rx_buff[2] = {0, };
	tx_buff[0] = addr | 0x80;

	// API chagned (2025.02.28)
	// return:  0(normal), -1(FAIL: *data=0)
	*data = 0;
	ret = spi_transceive(0, rx_buff, tx_buff, 2);
	if(ret < 2) {  // 2 BYTES
		NGT_LOG(DERR, "spi_transceive() FAIL: %s\n", strerror(errno));
		return -1;
	}
	else  *data = rx_buff[1];

	return 0;  // SUCCESS
}

int fpga_write(uint8_t addr, uint8_t data)
{
	int ret;
	uint8_t buff[2] = {0,};
	buff[0] = addr & 0x7f;
	buff[1] = data;

	ret = spi_transceive(0, NULL, buff, 2);
	if(ret < 2) {  // 2 BYTES
		NGT_LOG(DERR, "spi_transceive() FAIL: %s\n", strerror(errno));
		return -1;
	}

	return 0;  // SUCCESS
}

