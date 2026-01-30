#include <stdint.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <memory.h>
#include <sys/select.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>

#include <linux/spi/spidev.h>

#include "spi.h"
#include "log_print.h"

spi_info_t spi_info[2];

int spi_open(int id)
{
	if ( id == 0 )
	{
		spi_info[id].fd = open("/dev/spidev0.0", O_RDWR | O_CLOEXEC);
	}
	else
	{
		spi_info[id].fd = open("/dev/spidev0.1", O_RDWR);
	}
	NGT_LOG(DINFO, "open spi[%d] = %d\n", id, spi_info[id].fd);

	return spi_info[id].fd;
}

// 0: success, -1: error
int spi_close(int id)
{
	NGT_LOG(DINFO, "close spi[%d] = %d\n", id, spi_info[id].fd);
	return close(spi_info[id].fd);
}

int spi_init(int id,
		unsigned int spi_bits,
		unsigned int spi_speed,
		unsigned int spi_mode,
		unsigned short spi_delay)
{
	int ret = 0;

	// spi mode
//	ret = ioctl(spi_info[id].fd, SPI_IOC_WR_MODE, &spi_mode);
	ret = ioctl(spi_info[id].fd, SPI_IOC_WR_MODE32, &spi_mode);
	if (ret == -1)  {
		NGT_LOG(DERR, "can't set spi mode\n"); 
		goto __spi_init_done;
	}

//	ret = ioctl(spi_info[id].fd, SPI_IOC_RD_MODE, &spi_mode);
	ret = ioctl(spi_info[id].fd, SPI_IOC_RD_MODE32, &spi_mode);
	if (ret == -1) {
		NGT_LOG(DERR, "can't get spi mode\n");
		goto __spi_init_done;
	}

	// bits per word
	ret = ioctl(spi_info[id].fd, SPI_IOC_WR_BITS_PER_WORD, &spi_bits);
	if (ret == -1) {
		NGT_LOG(DERR, "can't set bits per word\n");
		goto __spi_init_done;
	}

	ret = ioctl(spi_info[id].fd, SPI_IOC_RD_BITS_PER_WORD, &spi_bits);
	if (ret == -1) {
		NGT_LOG(DERR, "can't get bits per word\n");
		goto __spi_init_done;
	}

	// max speed hz
	ret = ioctl(spi_info[id].fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed);
	if (ret == -1) {
		NGT_LOG(DERR, "can't set max speed hz\n");
		goto __spi_init_done;
	}

	ret = ioctl(spi_info[id].fd, SPI_IOC_RD_MAX_SPEED_HZ, &spi_speed);
	if (ret == -1) {
		NGT_LOG(DERR, "can't get max speed hz\n");
		goto __spi_init_done;
	}

	spi_info[id].bits = spi_bits;
	spi_info[id].speed = spi_speed;
	spi_info[id].mode = spi_mode;
	spi_info[id].delay = spi_delay;

__spi_init_done:
	return ret;
}

int spi_transceive(int id, unsigned char *rxBuffer, unsigned char *txBuffer, int len)
{
	int ret;

	struct spi_ioc_transfer xfer;
	memset(&xfer, 0, sizeof(xfer));
	//xfer.tx_buf = (unsigned long)txBuffer;
	//xfer.rx_buf = (unsigned long)rxBuffer;
	xfer.tx_buf = (uint64_t)(uintptr_t)txBuffer;
	xfer.rx_buf = (uint64_t)(uintptr_t)rxBuffer;

	xfer.len = (uint32_t)len;
	xfer.cs_change = 0;
	xfer.delay_usecs = 0;
	xfer.speed_hz = spi_info[id].speed;
	xfer.bits_per_word = spi_info[id].bits;

	ret = ioctl(spi_info[id].fd, SPI_IOC_MESSAGE(1), &xfer);

	return ret;
}
