#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>

#include <prussdrv.h>
#include <pruss_intc_mapping.h>

#include "beaglepru.h"

#define PRU_NUM 0

#ifndef START_ADDR
#error "START_ADDR must be defined"
#endif

int finish = 0;

void init_pru_program() {
	tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;
	prussdrv_init();
	prussdrv_open(PRU_EVTOUT_0);
	prussdrv_pruintc_init(&pruss_intc_initdata);

	prussdrv_load_datafile(PRU_NUM, "./data.bin");
	prussdrv_exec_program_at(PRU_NUM, "./text.bin", START_ADDR);
}

void signal_handler(int signal) {
	finish = 1;
}

int main(int argc, const char *argv[]) {
	int i;
	uint8_t dev_id[3];

	/* Listen to SIGINT signals (program termination) */
	signal(SIGINT, signal_handler);

	/* Load and run binary into pru0 */
	init_pru_program();

	/* Get pointer to shared ram */
	void* p;
	prussdrv_map_prumem(PRUSS0_SHARED_DATARAM, &p);
	unsigned int* shared_ram = (unsigned int*)p;

	for (i = 0; i < 3 ; i++) {
		shared_ram[1] = i;
		shared_ram[0] = CMD_READ_SIGNATURE;

		printf("Wait for interrupt from PRU\n");
		prussdrv_pru_wait_event(PRU_EVTOUT_0);
		prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);

		printf("Got interrupt from PRU\n");
		dev_id[i] = shared_ram[2];
	}

	printf("Device signature = 0x%02x%02x%02x\n", dev_id[0], dev_id[1],
	       dev_id[2]);

	printf("Disabling PRU.\n");
	prussdrv_pru_disable(PRU_NUM);
	prussdrv_exit();

	return 0;
}
