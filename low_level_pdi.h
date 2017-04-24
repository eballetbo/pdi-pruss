/**
 * Low level PDI driver.
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

#ifndef MEGA_PDI_H_INCLUDED
#define MEGA_PDI_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>

#include "config.h"
#include "status_codes.h"

/**
 * \brief Set the PDI CLK pin low
 */
#define pdi_clk_low() do { \
	PDI_CLK_PORT &= ~(1 << PDI_CLK_PIN); \
} while (0)

/**
 * \brief Set the PDI CLK pin high
 */
#define pdi_clk_high() do { \
	PDI_CLK_PORT |= (1 << PDI_CLK_PIN); \
} while (0)


void pdi_init(void);
void pdi_deinit(void);
enum status_code pdi_write(const uint8_t *data, uint16_t length);
enum status_code pdi_get_byte( uint8_t *ret, uint32_t retries);
uint16_t pdi_read(uint8_t *data, uint16_t length, uint32_t retries);

#endif
