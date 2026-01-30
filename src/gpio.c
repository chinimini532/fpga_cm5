/* 
 * Raspberry GPIO interface TEST
 * 
 *  ORG : http://elinux.org/RPi_GPIO_Code_Samples
 *  - Direct register access by C 
 *
 *  CHG : 2016.03.23 by KKU
 *  - Raspberry PI-2
 *  - Read/Write Test
 *  - g is pin-number(BCM) : g=5 -> GPIO.21
 *  - only ROOT can open /dev/mem ... (==> chmod 4755 : SetUID)
 *    // -rwsr-xr-x 1 root root 30456 Jul  8  2015 /usr/bin/gpio
 *
 *  [2022.02.04]
 *  - Raspberry PI-4 (CM4) 
 *  - BCM2711_PERI_BASE is 0xFE000000
 *  - BCM pin-number g 를 사용하고
 *  - wPi pin-number 는 참조로 GPIO.xx 로 주석처리
 */

//
//  How to access GPIO registers from C-code on the Raspberry-Pi
//  Example program
//  15-January-2012
//  Dom and Gert
//  Revised: 15-Feb-2013
 
 
// Access from ARM Running Linux

//#define BCM2708_PERI_BASE		0x20000000	// Raspberry PI-1
//#define BCM2708_PERI_BASE		0x3F000000 	// Raspberry PI-2,3
//#define BCM2711_PERI_BASE		0xFE000000  // Raspberry PI-4 (CM4)
//#define GPIO_BASE        	 	(BCM2711_PERI_BASE + 0x200000) /* GPIO controller */
 
#include <stdio.h>
#include <stdlib.h>
#include <gpiod.h>
#include "gpio.h"

#define GPIOCHIP_PATH "/dev/gpiochip0"

static struct gpiod_chip *chip = NULL;
static struct gpiod_line_request *reqs[60] = {0};
static int line_mode[60] = {0};    /*0=uninitialized, RASP_INPUT or RASP_OUTPUT*/

static int ensure_line(int bcm, int mode)
{
    if (bcm < 0 || bcm >= 60) { fprintf(stderr, "bad GPIO %d\n", bcm); return -1; }
    if (!chip) { fprintf(stderr, "gpiochip not open\n"); return -1; }

    if (reqs[bcm] && line_mode[bcm] == mode) return 0;

    /* release previous request if mode changed */
    if (reqs[bcm]) {
        gpiod_line_request_release(reqs[bcm]);
        reqs[bcm] = NULL;
        line_mode[bcm] = 0;
    }

    /* configure one-line request */
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    if (!settings) { fprintf(stderr, "line_settings_new failed\n"); return -1; }
    gpiod_line_settings_set_direction(
        settings,
        (mode == RASP_OUTPUT) ? GPIOD_LINE_DIRECTION_OUTPUT : GPIOD_LINE_DIRECTION_INPUT
    );
    if (mode == RASP_OUTPUT)
        gpiod_line_settings_set_output_value(settings, 0);

    struct gpiod_line_config *lcfg = gpiod_line_config_new();
    if (!lcfg) { gpiod_line_settings_free(settings); fprintf(stderr, "line_config_new failed\n"); return -1; }
    unsigned int offset = (unsigned int)bcm;
    if (gpiod_line_config_add_line_settings(lcfg, &offset, 1, settings) < 0) {
        gpiod_line_settings_free(settings);
        gpiod_line_config_free(lcfg);
        fprintf(stderr, "add_line_settings(%d) failed\n", bcm);
        return -1;
    }

    struct gpiod_request_config *rcfg = gpiod_request_config_new();
    if (!rcfg) {
        gpiod_line_settings_free(settings);
        gpiod_line_config_free(lcfg);
        fprintf(stderr, "request_config_new failed\n");
        return -1;
    }
    gpiod_request_config_set_consumer(rcfg, "fpga_loader");

    /* make the request */
    struct gpiod_line_request *req = gpiod_chip_request_lines(chip, rcfg, lcfg);

    gpiod_request_config_free(rcfg);
    gpiod_line_config_free(lcfg);
    gpiod_line_settings_free(settings);

    if (!req) { fprintf(stderr, "chip_request_lines(%d) failed\n", bcm); return -1; }

    reqs[bcm] = req;
    line_mode[bcm] = mode;
    return 0;
}

int  setup_io();
unsigned char gpio_read(int g);
void gpio_write(int g, int value);
void gpio_setmode(int g, int mode);

int init_fpga_gpio()
{
  // Set up gpi pointer for direct register access
  if(setup_io() < 0) return -1;

  // Set up FPGA PINS mode
  gpio_setmode(FPGA_INIT,    RASP_INPUT);
  gpio_setmode(FPGA_DONE,    RASP_INPUT);

  gpio_setmode(FPGA_CCLK,    RASP_OUTPUT);
  gpio_setmode(FPGA_DIN,     RASP_OUTPUT);
  gpio_setmode(FPGA_PROGRAM, RASP_OUTPUT);

  return 0;
}

//
// Set up a memory regions to access GPIO
//
int setup_io()
{
	chip = gpiod_chip_open(GPIOCHIP_PATH);
	if (!chip){
		fprintf(stderr, "gpiod_chip_open(%s) failed\n", GPIOCHIP_PATH);
		return -1;
	}
	return 0;
}

unsigned char gpio_read(int g)
{
    if (ensure_line(g, RASP_INPUT) < 0) return 0;
    int v = gpiod_line_request_get_value(reqs[g], (unsigned int)g);
    if (v < 0) { fprintf(stderr, "get_value(%d) failed\n", g); return 0; }
    return (unsigned char)(v & 1);
}

void gpio_write(int g, int value)
{
	if (ensure_line(g, RASP_OUTPUT) < 0) return;
        if (gpiod_line_request_set_value(reqs[g], (unsigned int)g, value ? 1 : 0) < 0) { 
		fprintf(stderr, "set_value(%d) failed\n", g);
    }
}

/* <MODE>
 * 0: INPUT
 * 1: OUTPUT
 * 2: ALT		--> later ...
 */
void gpio_setmode(int g, int mode)
{
	if (ensure_line(g, mode)){
		fprintf(stderr, "gpio_setmode(%d,%d) failed\n", g, mode);
	}
}
