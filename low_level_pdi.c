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

#include "low_level_pdi.h"

/* PRU registers */
volatile register unsigned int __R30;
volatile register unsigned int __R31;

/**
 * \brief Set the PDI DATA tx pin low.
 */
#define pdi_data_tx_low() do { \
       PDI_OUTPUT_PORT &= ~(1 << PDI_DATA_PIN_O); \
} while (0)

/**
 * \brief Set the PDI DATA tx pin high.
 */
#define pdi_data_tx_high() do { \
       PDI_OUTPUT_PORT |= (1 << PDI_DATA_PIN_O); \
} while (0)

/**
 * \brief Set the PDI DATA tx pin enabled.
 */
#define pdi_data_tx_enable() do { \
        PDI_OUTPUT_PORT |= (1 << PDI_TX_PIN_OE); \
} while (0)

/**
 * \brief Set the PDI DATA tx pin in tri-state mode.
 */
#define pdi_data_tx_disable() do { \
        PDI_OUTPUT_PORT &= ~(1 << PDI_TX_PIN_OE); \
} while (0)

/**
 * \brief Read PDI data bit
 */
static inline bool pdi_data_rx_bit(void)
{
	return (PDI_INPUT_PORT & (1 << PDI_DATA_PIN_I));
}

/**
 * \brief Write PDI data bit.
 */
static inline void pdi_write_bit(bool bit)
{
	pdi_clk_low();

	if (bit)
		pdi_data_tx_high();
	else
		pdi_data_tx_low();

	/* wait the 1st half of our clock cycle (50us) */
	__delay_cycles(PDI_CLK_RATE_DIV_2);

	pdi_clk_high();

	/* wait the 2nd half of our clock cycle (50us) */
	__delay_cycles(PDI_CLK_RATE_DIV_2);
}

/**
 * \brief Read PDI data bit.
 *
 * Returns data bit value.
 */
static inline bool pdi_read_bit(void)
{
	bool bit;

	pdi_clk_low();
	/* wait the 1st half of our clock cycle (50us) */
	__delay_cycles(PDI_CLK_RATE_DIV_2);

	pdi_clk_high();

	/* read back data */
	bit = pdi_data_rx_bit();
	
	/* wait the 2nd half of our clock cycle (50us) */
	__delay_cycles(PDI_CLK_RATE_DIV_2);

	return bit;
}

/**
 * \brief Write a single tx frame.
 *
 * \param data Byte to be sent.
 */
static inline void pdi_write_frame(uint8_t data)
{
	uint8_t bit;

	/* start bit */
	pdi_write_bit(0);

	/* write data bits */
	for (bit = 0; bit < 8; bit++)
		pdi_write_bit(data & (1 << bit));

	/* parity bit (even) */
	bit = 0;
	while (data) {
		bit = !bit;
		data = data & (data - 1);
	}
	pdi_write_bit(bit);

	/* stop bits (2) */
	pdi_write_bit(1);
	pdi_write_bit(1);
}

/**
 * \brief Write a BREAK byte to PDI
 */
static void pdi_write_break(void)
{
	int i;

	for (i = 0; i < 8; i++) {
		pdi_clk_low();

		/* wait the 1st half of our clock cycle (50us) */
		__delay_cycles(PDI_CLK_RATE_DIV_2);

		pdi_clk_high();

		/* wait the 2nd half of our clock cycle (50us) */
		__delay_cycles(PDI_CLK_RATE_DIV_2);
	}
}

/**
 * \brief Read a byte from PDI.
 *
 * \param value Pointer to buffer memory where data to be stored.
 * \param retries the retry count.
 *
 * \retval STATUS_OK read successfully.
 * \retval ERR_TIMEOUT read fail.
 *
 */
enum status_code pdi_get_byte(uint8_t *value, uint32_t retries)
{
	bool	bit;
	uint8_t i, parity = 0;

	pdi_data_tx_disable();

	/* Wait for start bit */
	while (retries) {
		if (!pdi_read_bit())
			break;
		retries--;
	}
	if (retries == 0)
		goto err_timeout;

	/* Initialize value (clean) */
	*value = 0;

	/* LSB first */
	for (i = 0; i < 8; i++) {
		bit = pdi_read_bit();
		if (bit) {
			*value |= bit << i;
			parity++;
		}
	}

	/* parity bit */
	bit = pdi_read_bit();
	if (bit != (parity % 2))
		goto err_timeout;

	/* stop bits */
	bit = pdi_read_bit();
	if (bit != 1)
		goto err_timeout;

	bit = pdi_read_bit();
	if (bit != 1)
		goto err_timeout;

	pdi_data_tx_enable();

	return STATUS_OK;

err_timeout:
	pdi_data_tx_enable();

	return ERR_TIMEOUT;
}

/**
 * \brief Write bulk bytes with PDI.
 *
 * \param data Pointer to memory where data to be sent is stored.
 * \parm length  Number of bytes to be sent.
 *
 * Returns 0 if the transmission was successful, otherwise returns an error
 * number.
 */
enum status_code pdi_write(const uint8_t *data, uint16_t length)
{
	uint16_t i;

	/* send two breaks before any write */
	for (i = 0; i < 2; i++)
		pdi_write_break();

	pdi_data_tx_enable();

	for (i = 0; i < length; i++)
		pdi_write_frame(data[i]);

	return STATUS_OK;
}

/**
 * \brief Read bulk bytes from PDI.
 *
 * \param data Pointer to memory where data to be stored.
 * \param length Number of bytes to be read.
 * \param retries the retry count.
 *
 * \retval non-zero the length of data.
 * \retval zero read fail.
 */
uint16_t pdi_read(uint8_t *data, uint16_t length, uint32_t retries)
{
	uint32_t count;
	uint16_t bytes_read = 0;
	uint8_t i, value = 0;

	for (i = 0; i < length; i++) {
		count = retries;
		while (count != 0) {
			if (pdi_get_byte(&value, retries) == STATUS_OK) {
				*(data + i) = value;
				bytes_read++;
				break;
			}
			--count;
		}
		/* Read fail error */
		if (count == 0) {
			return 0;
		}
	}

	return bytes_read;
}

/**
 * \brief Enable PDI programming mode (doc8282)
 *
 * The PDI Physical must be enabled before it can be used. This is done by
 * first forcing the PDI_DATA line high for a period longer than the
 * equivalent external reset minimum pulse width (refer to device data sheet
 * for reset characteristics).
 *
 * The first PDI_CLK cycle must start no later than 100uS after the RESET
 * functionality of the Reset pin was disabled. If this does not occur in time
 * the RESET functionality of the Reset pin is automatically enabled again and
 * the enabling procedure must start over again. 
 *
 * After this sequence, the PDI is enabled and ready to receive instructions.
 */
void pdi_init(void)
{
	uint8_t i;

	pdi_data_tx_enable();

	/* Make PDI DATA low and PDI CLK high as idle states. */
	pdi_clk_high();
	pdi_data_tx_low();

	/* Loop for 10ms to time out the PDI */
	for (i = 0; i < 100; i++) {
		__delay_cycles(20000);	/* 100us */
	}

	/* Now set PDI DATA high, wait 1us, this then start a 100us window */
	pdi_data_tx_high();

	/* Now wait some usec to be in the middle of a 100us window */
	__delay_cycles(1000);	/* 5us */

	/* let run PDI_CLK for at least 16 cycles (must be faster than 10 KHz) */
	for (i = 0; i < 32; i++) {
		pdi_clk_low();
		__delay_cycles(PDI_CLK_RATE_DIV_2);
		pdi_clk_high();
		__delay_cycles(PDI_CLK_RATE_DIV_2);
	}

	/*
	 * At this point the PDI Hardware Interface in the xmega chip should be
	 * enabled.
	 */
}

/**
 * \brief This function disables the PDI port.
 *
 * 3.5 Exit the PDI programming 
 * If there is no activity on the PDI_CLK line for approximately 100μs, the PDI 
 * automatically disabled. Then set the PDI_CLK to High and set the PDI_DATA to
 * Low. 
 */
void pdi_deinit( void )
{
	pdi_write_break();
	pdi_clk_high();
	__delay_cycles(60000);	/* 300us */
	// pdi_data_tx_disable();
	pdi_data_tx_low();
	pdi_clk_low();
	__delay_cycles(60000);	/* 300us */
	pdi_clk_high();

/*
	pdi_disable_clk();
	pdi_disable_rx();
	pdi_disable_tx();
	pdi_data_tx_input();
	pdi_data_tx_low();
	udelay(300);
	pdi_reset_high();
*/
}

