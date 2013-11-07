/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

/**
 * @file    drivers/gdisp/SSD1306/board_SSD1306_spi.h
 * @brief   GDISP Graphic Driver subsystem board interface for the SSD1306 display.
 */

#ifndef _GDISP_LLD_BOARD_H
#define _GDISP_LLD_BOARD_H

// The command byte to put on the front of each page line
#define SSD1306_PAGE_PREFIX		0x40			 		// Co = 0, D/C = 1

// For a multiple display configuration we would put all this in a structure and then
//	set g->board to that structure.
#define SSD1306_RESET_PORT		GPIOB
#define SSD1306_RESET_PIN		5
#define SSD1306_CS_PORT			GPIOA
#define SSD1306_CS_PIN			4
#define SSD1306_SCK_PORT		GPIOA
#define SSD1306_SCK_PIN			5
#define SSD1306_MISO_PORT		GPIOA
#define SSD1306_MISO_PIN		6
#define SSD1306_MOSI_PORT		GPIOA
#define SSD1306_MOSI_PIN		7

#define SET_RST					palSetPad(SSD1306_RESET_PORT, SSD1306_RESET_PIN);
#define CLR_RST					palClearPad(SSD1306_RESET_PORT, SSD1306_RESET_PIN);

/*
 * SPI1 configuration structure.
 * Speed 42MHz, CPHA=0, CPOL=0, 8bits frames, MSb transmitted first.
 * The slave select line is the pin 4 on the port GPIOA.
 */
static const SPIConfig spi1config = {
	NULL,
	/* HW dependent part.*/
	SSD1306_CS_PORT,
	SSD1306_CS_PIN,
	SPI_CR1_BR_0,
};

#if GFX_USE_OS_CHIBIOS
	static int32_t thdPriority = 0;
#endif

static inline void init_board(GDisplay *g) {
	unsigned	i;

	// As we are not using multiple displays we set g->board to NULL as we don't use it.
	g->board = 0;


	switch(g->controllerdisplay) {
	case 0:											// Set up for Display 0
		// RESET pin.
		palSetPadMode(SSD1306_RESET_PORT, SSD1306_RESET_PIN, PAL_MODE_OUTPUT_PUSHPULL);

		palSetPadMode(SSD1306_MISO_PORT, SSD1306_MISO_PIN, 	PAL_MODE_OUTPUT_PUSHPULL|
															PAL_STM32_OSPEED_HIGHEST);
		palSetPadMode(SSD1306_MOSI_PORT, SSD1306_MOSI_PIN, 	PAL_MODE_ALTERNATE(0)|
															PAL_STM32_OSPEED_HIGHEST);
		palSetPadMode(SSD1306_SCK_PORT,  SSD1306_SCK_PIN,  	PAL_MODE_ALTERNATE(0)|
															PAL_STM32_OSPEED_HIGHEST);
		palSetPad(SSD1306_CS_PORT, SSD1306_CS_PIN);
		palSetPadMode(SSD1306_CS_PORT,   SSD1306_CS_PIN,   	PAL_MODE_OUTPUT_PUSHPULL|
															PAL_STM32_OSPEED_HIGHEST);
		break;
	}
}

static inline void post_init_board(GDisplay *g) {
	(void) g;
}

static inline void setpin_reset(GDisplay *g, bool_t state) {
	(void) g;
	if(state)
		CLR_RST
	else
		SET_RST
}

static inline void acquire_bus(GDisplay *g) {
	(void) g;
	#if GFX_USE_OS_CHIBIOS
		thdPriority = (int32_t)chThdGetPriority();
		chThdSetPriority(HIGHPRIO);
	#endif
	spiAcquireBus(&SPID1);
	spiStart(&SPID1, &spi1config);
	spiSelect(&SPID1);
}

static inline void release_bus(GDisplay *g) {
	(void) g;
	spiUnselect(&SPID1);
	spiReleaseBus(&SPID1);
	#if GFX_USE_OS_CHIBIOS
		chThdSetPriority(thdPriority);
	#endif
}

static inline void write_cmd(GDisplay *g, uint8_t cmd) {
	uint8_t command[2];
	(void)	g;

	palClearPad(SSD1306_MISO_PORT, SSD1306_MISO_PIN);
	spiSend(&SPID1, 1, &cmd);
}

static inline void write_data(GDisplay *g, uint8_t* data, uint16_t length) {
	(void) g;

	palSetPad(SSD1306_MISO_PORT, SSD1306_MISO_PIN);
	spiSend(&SPID1, length-1, data+1);
}


#endif /* _GDISP_LLD_BOARD_H */

