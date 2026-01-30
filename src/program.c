#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <arpa/inet.h>

#include "gpio.h"
#include "log_print.h"

/* [Spartan-3E FPGA XC3S500E] 
 *   Configuration Bits
 *   = 2,270,208 Bits
 *   = 283,776 Bytes
 *   = 0x45480
 *
 *  REF: 
 *  	ACC u-boot/common/cmd_srecord.c
 *  	fpga_bin_converter
 *
 *  H16 format
 *      -------------------------------------------------------------------------
 *  	4 bytes		| 4 bytes	  | 4 bytes 	| 4 bytes
 *  	FPGA		  size			csum		  zero padding + 1byte(board_id)
 *  	46 50 47 41   00 04 54 90	xx xx xx xx   00 00 00 xx
 *      -------------------------------------------------------------------------
 *      size = 0x45480 + 0x10
 *      board_id = 2 (default: 568KB)
 */

#define BYTES_OF_H16         	0x10  // 16 bytes

#define FPGA_XC3S100E_BYTES		0x11BDC
#define FPGA_XC3S250E_BYTES		0x29500
#define FPGA_XC3S500E_BYTES		0x45480  // default
#define FPGA_XC3S1200E_BYTES	0x75394
#define FPGA_XC3S1600E_BYTES	0xB62E4

#define DEV_ID_XC3S100E			0x01C10093
#define DEV_ID_XC3S250E			0x01C1A093
#define DEV_ID_XC3S500E			0x01C22093
#define DEV_ID_XC3S1200E		0x01C2E093
#define DEV_ID_XC3S1600E		0x01C3A093

#define FPGA_MAX_BYTES       	FPGA_XC3S500E_BYTES
#define FILE_SIZE_IN_H16		FPGA_MAX_BYTES + BYTES_OF_H16

#define GPIO_PIN_WAIT   		1000000

/* at gpio.c */
extern unsigned char gpio_read(int g);
extern void gpio_write(int g, int value);
extern int init_fpga_gpio(); 

#include <time.h>
#include <errno.h>
void DelayNanoSec(int delay_nsec)
{
	struct timespec start, now;

	clock_gettime(CLOCK_MONOTONIC, &start);

	while(1) {
		clock_gettime(CLOCK_MONOTONIC, &now);
		if(now.tv_sec > start.tv_sec)  return;
		if(now.tv_nsec > (start.tv_nsec + delay_nsec))  return;
	}

	return;
}

static uint32_t mk_chksum(uint8_t *d, int size)
{
    int i;
	uint32_t csum = 0;

    for(i=0; i<size; i++, d++) {
        csum += *d;
    }

    return csum;
}

static int check_fpga_data(int fsize, uint8_t *buff)
{
    uint32_t r_size, r_csum, c_csum;

    if((buff[0] != 'F') || (buff[1] != 'P') || (buff[2] != 'G') || (buff[3] != 'A')) {
        printf("CheckData ERR: not \"FPGA\"\n");
        NGT_LOG(DERR, "CheckData ERR: not \"FPGA\"\n");
        return -1;
    }

    // size check
    r_size = htonl(*(uint32_t *)&buff[4]);
    if(r_size != (fsize - BYTES_OF_H16)) {
        printf("CheckData ERR: SIZE(read=0x%X, file=0x%X)\n", r_size, fsize);
        NGT_LOG(DERR, "CheckData ERR: SIZE(read=0x%X, file=0x%X)\n", r_size, fsize);
        return -1;
    }

    // checksum check
    r_csum = htonl(*(uint32_t *)&buff[8]);
	c_csum = mk_chksum(&buff[16], r_size);
    if(r_csum != c_csum) {
        printf("CheckData ERR: CSUM(read=0x%x, cal=0x%x)\n", r_csum, c_csum);
        NGT_LOG(DERR, "CheckData ERR: CSUM(read=0x%x, cal=0x%x)\n", r_csum, c_csum);
        return -1;
    }

#if 1  // DEBUG INFO - SyncWord, DeviceID
	{
		int i, j;
		uint8_t data, mask, swap_data = 0;
		uint32_t sync_word, dev_id;
		char str_devid[20] = {0,};

		sync_word = 0;
		for(i=0; i<4; i++) {
			data = *(uint8_t *)(buff + BYTES_OF_H16 + 0x04 + i);  // SyncWord : 4~7
			swap_data = 0;
	        for(j=0, mask=0x01; j<8; j++, mask <<= 1) {
    	        if(mask & data) swap_data = (swap_data << 1) | 0x01;
        	    else            swap_data = (swap_data << 1) & 0xFE;
        	}
			sync_word |= (swap_data << (3-i)*8);
		}

		dev_id = 0;
		for(i=0; i<4; i++) {
			data = *(uint8_t *)(buff + BYTES_OF_H16 + 0x24 + i);  // DeviceID : 0x24~0x27
			swap_data = 0;
	        for(j=0, mask=0x01; j<8; j++, mask <<= 1) {
    	        if(mask & data) swap_data = (swap_data << 1) | 0x01;
        	    else            swap_data = (swap_data << 1) & 0xFE;
        	}
			dev_id |= (swap_data << (3-i)*8);
		}

		switch(dev_id) {
			case DEV_ID_XC3S100E  :  sprintf(str_devid, "XC3S100E");   break;
			case DEV_ID_XC3S250E  :  sprintf(str_devid, "XC3S250E");   break;
			case DEV_ID_XC3S500E  :  sprintf(str_devid, "XC3S500E");   break;
			case DEV_ID_XC3S1200E :  sprintf(str_devid, "XC3S1200E");  break;
			case DEV_ID_XC3S1600E :  sprintf(str_devid, "XC3S1600E");  break;
			default :  sprintf(str_devid, "UNKNOWN");  break;
		}
		printf("SyncWord: %X, DeviceIDx: %X -> %s\n", sync_word, dev_id, str_devid);
		NGT_LOG(DINFO, "SyncWord: %X, DeviceIDx: %X -> %s\n", sync_word, dev_id, str_devid);
	}
#endif

    return fsize;  // return file_size
}

int fpga_program(int dbg_level, char *file_name)
{
	int file_size, rd_len;
    uint8_t *buff;
	FILE *fp;

	int i, j;
	uint8_t *addr;
	uint8_t data, mask;

    /* FILE check */
    // file open
    fp = fopen(file_name, "r+");
    if(fp == NULL) {
        printf(" !! %s open ERROR !!\n", file_name);
		NGT_LOG(DERR, " !! %s open ERROR !!\n", file_name);
		goto ERR_0;
    }

    // memory allocation
    fseek(fp, 0L, SEEK_END);
    file_size = ftell(fp);
	printf("FileSize: 0x%x -> ", file_size);
	switch((file_size - BYTES_OF_H16)) {
		case FPGA_XC3S100E_BYTES  :  printf("XC3S100E\n");  break;
		case FPGA_XC3S250E_BYTES  :  printf("XC3S250E\n");  break;
		case FPGA_XC3S500E_BYTES  :  printf("XC3S500E\n");  break;
		case FPGA_XC3S1200E_BYTES :  printf("XC3S1200E\n"); break;
		case FPGA_XC3S1600E_BYTES :  printf("XC3S1600E\n"); break;
		default :  printf("UNKNOWN\n");  break;
	}
    buff = malloc(file_size);
    if(buff == NULL) {
        printf(" !! memory allocation ERROR !!\n");
		NGT_LOG(DERR, " !! memory allocation ERROR !!\n");
    	goto ERR_1;
    }

    // read file
    fseek(fp, 0L, SEEK_SET);
    rd_len = fread(buff, 1, file_size, fp);
    if(rd_len != file_size) {
        printf(" !! file read ERROR !! (rd_len = 0x%x, file_size = 0x%x)\n", 
				rd_len, file_size);
        NGT_LOG(DERR, " !! file read ERROR !! (rd_len = 0x%x, file_size = 0x%x)\n",
				rd_len, file_size);
    	goto ERR_2;
	}

	NGT_LOG(DINFO, "FPGA(%s, fsize=0x%x) program\n", file_name, file_size);
	
	// check ("FPGA" + size + checksum)
	file_size = check_fpga_data(file_size, buff);
	if(file_size < 0)  goto ERR_2;

	/* program FPGA */
	printf("FPGA program "); 
	fflush((FILE *)stdout);
	
	addr = buff + BYTES_OF_H16;

	init_fpga_gpio();

	gpio_write(FPGA_PROGRAM, 0);
	DelayNanoSec(100);
	
	gpio_write(FPGA_CCLK, 1);
	gpio_write(FPGA_PROGRAM, 1);

	i = 100;
	while(gpio_read(FPGA_INIT) == 0) {
		usleep(GPIO_PIN_WAIT);
		if(--i < 0) {
			printf("FAIL\n!! INIT is Low - None FPGA Error !!\n");
			NGT_LOG(DERR, "FAIL !! --> INIT is Low (None FPGA Error)\n");
			goto SKIP_FPGA;  // INIT_LOW -> SKIP_FPGA -> ERR_2
		}
	}

	for(i=0; i<file_size; i++) {
		data = *addr++;
		
		mask = 0x01;
		for(j=0; j<8; j++, mask <<= 1) {
			if(mask & data)  gpio_write(FPGA_DIN, 1);
			else             gpio_write(FPGA_DIN, 0);

			gpio_write(FPGA_CCLK, 0);
			DelayNanoSec(50);

			gpio_write(FPGA_CCLK, 1);
			DelayNanoSec(50);
		}

		/* Check FPGA_INIT after DeviceID */
		if((i == 0x60) && (gpio_read(FPGA_INIT) == 0)) {
			printf("FAIL\n!! INIT is Low - DeviceID Error !!\n");
			NGT_LOG(DERR, "FAIL !! --> INIT is Low (DeviceID Error)\n");
			goto SKIP_FPGA;  // INIT_LOW -> SKIP_FPGA -> ERR_2
		}

		/* if needs -> Check FPGA_INIT after CMD(CRC) */
		if((i == (FPGA_MAX_BYTES-0x20)) && (gpio_read(FPGA_INIT) == 0)) {
			printf("FAIL\n!! INIT is Low - Maybe CRC Error !!\n");
			NGT_LOG(DERR, "FAIL !! --> INIT is Low (Maybe CRC Error)\n");
			goto SKIP_FPGA;  // INIT_LOW -> SKIP_FPGA -> ERR_2
		}

		/* progress -> about 0.5 sec */
		if(i!=0 && i%0x4000 == 0) {
			printf(".");  fflush((FILE *)stdout);
		}
	}

	usleep(GPIO_PIN_WAIT);

	if(gpio_read(FPGA_DONE)) {
		printf(" SUCCESS\n");
		NGT_LOG(DINFO, "SUCCESS\n");
    	free(buff);
		fclose(fp);
		return 0;  // SUCCESS return
	}

	// FAIL -> SKIP_FPGA -> ERR_2
	printf(" FAIL\n!! DONE is Low !!\n"); 
	NGT_LOG(DERR, "FAIL !! --> DONE is Low\n");

SKIP_FPGA :
	gpio_write(FPGA_DIN, 0);
	usleep(GPIO_PIN_WAIT);
	gpio_write(FPGA_DIN, 1);

ERR_2 :  // free memory -> file close -> return
    free(buff);
    fclose(fp);
	return -1;

ERR_1 :  // file close -> return
    fclose(fp);
	return -1;

ERR_0 :  // return
	return -1;
}
