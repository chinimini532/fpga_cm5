#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#define LOG_FILE        "fpga_log"

#include "log_print.h"
#include "fpga.h"  /* from NGT_LIB */

extern int fpga_program(int dbg_level, char *file_name);  /* at program.c */

uint8_t atoh(char *str)
{
	unsigned long value;
	char *stop;

	value = strtoul(str, &stop, 16);
	return (uint8_t)(value & 0xFF);
}

int main(int argc, char *argv[])
{
    int i;
	int err;
    uint8_t addr, data;

    if(argc < 3)	goto HELP;	

	/* LOG -> only at fpga_program() */
	init_ngt_log(LOG_FILE, 0, 0, 0);

    /* Command Parsing */
    if(!strcmp(argv[1], "-w")) {
        addr = atoh(argv[2]);
		data = atoh(argv[3]);
       
		err = fpga_init();  // no LOG
		if(err < 0)  printf("check if you are root\n");
		else {
			fpga_write(addr, data);
			printf(" WRITE : Reg 0x%02X, Data 0x%02X\n", addr, data);
		}
    }
    else if(!strcmp(argv[1], "-r")) {
		err = fpga_init();  // no LOG
		if(err < 0)  printf("check if you are root\n");
		else {
        	if(!strcmp(argv[2], "all")) {	// "all" READ
            	printf(" <FPGA REG values>\n");
				printf(" -------------------------------------------------------\n");
				printf("        00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F \n");
				printf(" -------------------------------------------------------");

            	for(i=0; i<MAX_FPGA_REG_ADDR; i++) {
                	addr = (uint8_t)i;
        			fpga_read(addr, &data);
                	if(i%16 == 0)  printf("\n 0x%02x :", i&0xF0);
                	printf(" %02X", data);
	    		}
				printf("\n -------------------------------------------------------\n");
        	}
        	else {	// only one character READ
            	addr = atoh(argv[2]);
            	fpga_read(addr, &data);
            	printf(" READ : Reg 0x%02X, Data 0x%02X\n", addr&0x7F, data);
        	}
		}
    }
	else if(!strcmp(argv[1], "-p")) {
		ngt_log_set_level(1, (DERR | DINFO));
		err = fpga_init();  // with LOG
		if(err < 0)  printf("check if you are root\n");
		else {	
			if(argc == 4)	i = atoi(argv[3]);
			else			i = 1;				// debug level = 1
			if(i<0 || i>4)	goto HELP;
			fpga_program(i, argv[2]);
		}
	}
    else goto HELP;

	fpga_close();
	ngt_log_init_file_descriptor();
    return 0;

HELP:
    printf(" fpga -<w/r> addr/all [data]\n");
    printf(" fpga -p file [0~4: dbg_level]\n");
    printf(" <ex> fpga -r all\n");
    printf("      fpga -r d\n");
    printf("      fpga -w 8 21\n");
	printf("      fpga -p CRIA_220203.fbin\n");
	
	fpga_close();
	ngt_log_init_file_descriptor();
    return 0;
}



