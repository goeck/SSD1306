/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

/**
 * @file    drivers/gdisp/SSD1306/board_SSD1306_template.h
 * @brief   GDISP Graphic Driver subsystem board interface for the SSD1306 display.
 *
 * @addtogroup GDISP
 * @{
 */

#ifndef _GDISP_LLD_BOARD_H
#define _GDISP_LLD_BOARD_H

/**
 * @brief   Optional: A byte to prefix on each display page line.
 * @note	If not defined then no byte is prefixed on each page line.
 *
 * @notapi
 */
//#define SSD1306_PAGE_PREFIX		0x40

/**
 * @brief   Initialise the board for the display.
 *
 * @param[in] g			The GDisplay structure
 *
 * @note	Set the g->board member to whatever is appropriate. For multiple
 * 			displays this might be a pointer to the appropriate register set.
 *
 * @notapi
 */
static inline void init_board(GDisplay *g) {
	(void) g;
}

/**
 * @brief   After the initialisation.
 *
 * @param[in] g			The GDisplay structure
 *
 * @notapi
 */
static inline void post_init_board(GDisplay *g) {
	(void) g;
}

/**
 * @brief   Set or clear the lcd reset pin.
 *
 * @param[in] g			The GDisplay structure
 * @param[in] state		TRUE = lcd in reset, FALSE = normal operation
 *
 * @notapi
 */
static inline void setpin_reset(GDisplay *g, bool_t state) {
	(void) g;
	(void) state;
}

/**
 * @brief   Take exclusive control of the bus
 *
 * @param[in] g				The GDisplay structure
 *
 * @notapi
 */
static inline void acquire_bus(GDisplay *g) {
	(void) g;
}

/**
 * @brief   Release exclusive control of the bus
 *
 * @param[in] g				The GDisplay structure
 *
 * @notapi
 */
static inline void release_bus(GDisplay *g) {
	(void) g;
}

/**
 * @brief   Send a command to the controller.
 *
 * @param[in] g				The GDisplay structure
 * @param[in] cmd			The command to send *
 *
 * @notapi
 */
static inline void write_cmd(GDisplay *g, uint8_t cmd) {
	(void) g;
	(void) cmd;
}

/**
 * @brief   Send data to the lcd.
 *
 * @param[in] g				The GDisplay structure
 * @param[in] data			The data to send
 *
 * @notapi
 */
static inline void write_data(GDisplay *g, uint8_t* data, uint16_t length) {
	(void) g;
	(void) data;
	(void) length;
}

#endif /* _GDISP_LLD_BOARD_H */
/** @} */

