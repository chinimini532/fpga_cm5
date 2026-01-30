#ifndef	__NGT_SPI_H__
#define	__NGT_SPI_H__

typedef struct{
	int fd;
	unsigned int bits;
	unsigned int speed;
	unsigned int mode;
	unsigned short delay;
} spi_info_t;

#ifdef __cplusplus
extern "C" {
#endif
int spi_open(int id);
int spi_close(int id);
int spi_init(int id,
		unsigned int spi_bits,
		unsigned int spi_speed,
		unsigned int spi_mode,
		unsigned short spi_delay);
int spi_transceive(int id, unsigned char *rxBuffer, unsigned char *txBuffer, int len);
#ifdef __cplusplus
}
#endif

#endif
