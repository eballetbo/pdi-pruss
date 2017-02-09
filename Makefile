PRU_COMPILER_DIR=./vendors/ti-cgt-pru_2.1.4
PRU_C_FLAGS=--silicon_version=3 -O2 -i$(PRU_COMPILER_DIR)/include -i$(PRU_COMPILER_DIR)/lib
PRU_LD_FLAGS=-llibc.a
CROSS_COMPILE=arm-linux-gnueabihf-

PREFIX?=/usr/local

HOST_C_FLAGS += -Wall -g -O2 -mtune=cortex-a8 -march=armv7-a -I$(PREFIX)/include
HOST_LD_FLAGS += $(PREFIX)/lib/libprussdrv.a -lpthread

FIND_ADDRESS_COMMAND=`$(PRU_COMPILER_DIR)/bin/dispru pru.elf | grep _c_int00 | cut -f1 -d\  `

.PHONY: all
all:
	# Compile pru.c into pro.obj
	$(PRU_COMPILER_DIR)/bin/clpru $(PRU_C_FLAGS) -c low_level_pdi.c xmega_pdi_nvm.c pru.c

	# Link pru.obj with libraries and output pru.map and pru.elf
	$(PRU_COMPILER_DIR)/bin/clpru $(PRU_C_FLAGS) -z low_level_pdi.obj xmega_pdi_nvm.obj pru.obj $(PRU_LD_FLAGS) \
		-m pru.map -o pru.elf $(PRU_COMPILER_DIR)/example/AM3359_PRU.cmd

	# Convert pru.elf into text.bin and data.bin
	$(PRU_COMPILER_DIR)/bin/hexpru $(PRU_COMPILER_DIR)/bin.cmd ./pru.elf

	# Find address of start of program and compile host program
	export START_ADDR=0x$(FIND_ADDRESS_COMMAND) && \
	$(CROSS_COMPILE)gcc $(HOST_C_FLAGS) -DSTART_ADDR=`echo $$START_ADDR` -c -o pdi.o pdi.c && \
	$(CROSS_COMPILE)gcc $(HOST_C_FLAGS) -o pdi pdi.o $(HOST_LD_FLAGS)

.PHONY: clean
clean:
	-rm *.obj
	-rm *.map
	-rm *.elf
	-rm *.bin
	-rm *.o
	-rm pdi
