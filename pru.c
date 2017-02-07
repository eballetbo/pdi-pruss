/*
 * PDI programmer
 *
 * Copyright (c) 2016 Collabora Ltd.
 * Author: Enric Balletbo Serra <enric.balletbo@collabora.cocom>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "xmega_pdi_nvm.h"
#include "low_level_pdi.h"
#include "beaglepru.h"

#include "atxmega16d4_nvm_regs.h"

/* Macro for accessing a hardware register (32 bit) */
#define HWREG(x) (*((volatile unsigned int *)(x)))

/*
 * AM335x PRU-ICSS Reference Guide (Rev. A), p. 272
 *
 * The PRU-ICSS CFG block contains the registers for control and status of
 * power management, memory parity, and enhanced PRU GP ports functions.
 */
#define SYSCFG	0x26004
#define GPCFG0	0x26008

volatile register uint32_t __R31;

#define BUFSIZE	256
#define half(x)	((x)/2)

int main(int argc, const char *argv[]) {
	int i;
	uint8_t page_buffer[BUFSIZE], dev_id[3];
	unsigned int finish = 0;

	/*
	 * Shared ram is at address 0x10000.
	 * See table 4.7 Local Memory Map in page 204 of manual
	 */
	volatile uint32_t* shared_ram = (uint32_t *)0x10000;

	/*
	 * Enable OCP so we can access the whole memory map for the
	 * device from the PRU. Clear bit 4 of SYSCFG register
	 */
	HWREG(SYSCFG) &= 0xFFFFFFEF;

	/*
	 * GPI Mode 0, GPO Mode 0 
	 */
	HWREG(GPCFG0) = 0;

	while (!finish) {
		/*
		 * Wait until an interrupt request to this PRU happens.
		 * If bit 30 of register 31 is set. That means someone sent 
		 * an interrupt request to this PRU.
		 */
		if (shared_ram[0] == 0)
			continue;

		switch (shared_ram[0]) {
		case CMD_READ_SIGNATURE:
			if (shared_ram[1] == 0) {
				/* Initialize the PDI interface */
				xnvm_init();
				/* Read device ID */
				xnvm_read_memory(XNVM_DATA_BASE + NVM_MCU_CONTROL, dev_id, 3);
				shared_ram[2] = dev_id[0];
			} else if (shared_ram[1] == 1) {
				shared_ram[2] = dev_id[1];
			} else if (shared_ram[1] == 2) {
				shared_ram[2] = dev_id[2];
			}
			break;
		case CMD_CHIP_ERASE:
			xnvm_chip_erase();
			break;
		case CMD_READ_FLASH:
			memset(page_buffer, 0, BUFSIZE);

			/* Initialize the PDI interface */
			xnvm_init();
			xnvm_read_memory(XNVM_FLASH_BASE + shared_ram[1],
					 page_buffer, half(BUFSIZE));

			/* Initialize the PDI interface */
			xnvm_init();
			xnvm_read_memory(XNVM_FLASH_BASE + shared_ram[1] + half(BUFSIZE),
					 &page_buffer[half(BUFSIZE)], half(BUFSIZE));

			/* */
			pdi_deinit();

			for (i = 0; i < BUFSIZE; i++)
				shared_ram[i + 5] = page_buffer[i];
			break;
		case CMD_PROGRAM_FLASH:
			for (i = 0; i < BUFSIZE; i++)
				page_buffer[i] = (uint8_t)shared_ram[i + 5];

			/* Initialize the PDI interface */
			xnvm_init();
			xnvm_erase_program_flash_page(0x0000 + shared_ram[1], page_buffer, BUFSIZE);
			break;
		default:
			break;
		}

		shared_ram[0] = 0;
		shared_ram[1] = 0;

		/*
		 * Writing to register 31 sends interrupt requests to the
		 * ARM system. See section 4.4.1.2.2 Event Interface
		 * Mapping (R31): PRU System Events in page 209 of manual.
		 * Not sure why vector output is 4 in this case but this is
		 * what they do in.
		 */
		__R31 = 35;
	}

	/*
	 * stop pru processing
	 */
	__halt(); 

	return 0;
}

