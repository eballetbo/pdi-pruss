/**
 * PRU configuration file.
 *
 * Copyright (C) 2015-2017 Toby Churchill Ltd.
 *
 * Enric Balletbo Serra <enric.balletbo@collabora.com>
 *
 * License
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

/*
 * PDI configuration pins
 */
#define PDI_CLK_PORT		__R30
#define PDI_CLK_PIN		14
#define PDI_INPUT_PORT		__R31
#define PDI_DATA_PIN_I		15	/* GPI */
#define PDI_OUTPUT_PORT		__R30
#define PDI_DATA_PIN_O		15	/* GPO */
#define PDI_TX_PIN_OE		5

/*
 * 200 MHz @ 5ns
 */
#define PDI_CLK_RATE_DIV_2	1000	/* f = 100 kHz */

#endif

