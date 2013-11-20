/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

/**
 * @file    drivers/gdisp/SSD1306/gdisp_lld.c
 * @brief   GDISP Graphics Driver subsystem low level driver source for the SSD1306 display.
 */

#include "gfx.h"

#if GFX_USE_GDISP

#define GDISP_DRIVER_VMT			GDISPVMT_SSD1306
#include "../drivers/gdisp/SSD1306/gdisp_lld_config.h"
#include "gdisp/lld/gdisp_lld.h"

#include "../../../boards/addons/gdisp/board_SSD1306_spi.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#ifndef GDISP_SCREEN_HEIGHT
	#define GDISP_SCREEN_HEIGHT		64		// This controller should support 32 (untested) or 64
#endif
#ifndef GDISP_SCREEN_WIDTH
	#define GDISP_SCREEN_WIDTH		128
#endif
#ifndef GDISP_INITIAL_CONTRAST
	#define GDISP_INITIAL_CONTRAST	100
#endif
#ifndef GDISP_INITIAL_BACKLIGHT
	#define GDISP_INITIAL_BACKLIGHT	100
#endif
#ifdef SSD1306_PAGE_PREFIX
	#define SSD1306_PAGE_WIDTH		(GDISP_SCREEN_WIDTH+1)
	#define SSD1306_PAGE_OFFSET		1
#else
	#define SSD1306_PAGE_WIDTH		GDISP_SCREEN_WIDTH
	#define SSD1306_PAGE_OFFSET		0
#endif

#define GDISP_FLG_NEEDFLUSH			(GDISP_FLG_DRIVER<<0)

#include "SSD1306.h"

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

// Some common routines and macros
#define RAM(g)							((uint8_t *)g->priv)
#define write_cmd2(g, cmd1, cmd2)		{ write_cmd(g, cmd1); write_cmd(g, cmd2); }
#define write_cmd3(g, cmd1, cmd2, cmd3)	{ write_cmd(g, cmd1); write_cmd(g, cmd2); write_cmd(g, cmd3); }

// Some common routines and macros
#define delay(us)			gfxSleepMicroseconds(us)
#define delayms(ms)			gfxSleepMilliseconds(ms)

#define xyaddr(x, y)		(SSD1306_PAGE_OFFSET + (x) + ((y)>>3)*SSD1306_PAGE_WIDTH)
#define xybit(y)			(1<<((y)&7))

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * As this controller can't update on a pixel boundary we need to maintain the
 * the entire display surface in memory so that we can do the necessary bit
 * operations. Fortunately it is a small display in monochrome.
 * 64 * 128 / 8 = 1024 bytes.
 */

LLDSPEC bool_t gdisp_lld_init(GDisplay *g) {
	// The private area is the display surface.
	g->priv = gfxAlloc(GDISP_SCREEN_HEIGHT/8 * SSD1306_PAGE_WIDTH);

	// Fill in the prefix command byte on each page line of the display buffer
	// We can do it during initialisation as this byte is never overwritten.
	#ifdef SSD1306_PAGE_PREFIX
		{
			unsigned	i;

			for(i=0; i < GDISP_SCREEN_HEIGHT/8 * SSD1306_PAGE_WIDTH; i+=SSD1306_PAGE_WIDTH)
				RAM(g)[i] = SSD1306_PAGE_PREFIX;
		}
	#endif

	// Initialise the board interface
	init_board(g);

	// Hardware reset
	setpin_reset(g, TRUE);
	gfxSleepMilliseconds(20);
	setpin_reset(g, FALSE);
	gfxSleepMilliseconds(20);

	acquire_bus(g);

	write_cmd(g, SSD1306_DISPLAYOFF);
	write_cmd2(g, SSD1306_SETDISPLAYCLOCKDIV, 0x80);
	write_cmd2(g, SSD1306_SETMULTIPLEX, GDISP_SCREEN_HEIGHT-1);
	write_cmd2(g, SSD1306_SETPRECHARGE, 0x1F);
	write_cmd2(g, SSD1306_SETDISPLAYOFFSET, 0);
	write_cmd(g, SSD1306_SETSTARTLINE | 0);
	write_cmd2(g, SSD1306_ENABLE_CHARGE_PUMP, 0x14);
	write_cmd2(g, SSD1306_MEMORYMODE, 0);
	write_cmd(g, SSD1306_SEGREMAP+1);
	write_cmd(g, SSD1306_COMSCANDEC);
	#if GDISP_SCREEN_HEIGHT == 64
		write_cmd2(g, SSD1306_SETCOMPINS, 0x12);
	#else
		write_cmd2(g, SSD1306_SETCOMPINS, 0x22);
	#endif
	write_cmd2(g, SSD1306_SETCONTRAST, (uint8_t)(GDISP_INITIAL_CONTRAST*256/101));	// Set initial contrast.
	write_cmd2(g, SSD1306_SETVCOMDETECT, 0x10);
	write_cmd(g, SSD1306_DISPLAYON);
	write_cmd(g, SSD1306_NORMALDISPLAY);
	write_cmd3(g, SSD1306_HV_COLUMN_ADDRESS, 0, GDISP_SCREEN_WIDTH-1);
	write_cmd3(g, SSD1306_HV_PAGE_ADDRESS, 0, GDISP_SCREEN_HEIGHT/8-1);

    // Finish Init
    post_init_board(g);

 	// Release the bus
	release_bus(g);

	/* Initialise the GDISP structure */
	g->g.Width = GDISP_SCREEN_WIDTH;
	g->g.Height = GDISP_SCREEN_HEIGHT;
	g->g.Orientation = GDISP_ROTATE_0;
	g->g.Powermode = powerOn;
	g->g.Backlight = GDISP_INITIAL_BACKLIGHT;
	g->g.Contrast = GDISP_INITIAL_CONTRAST;
	return TRUE;
}

#if GDISP_HARDWARE_FLUSH
	LLDSPEC void gdisp_lld_flush(GDisplay *g) {
		unsigned	i;

		// Don't flush if we don't need it.
		if (!(g->flags & GDISP_FLG_NEEDFLUSH))
			return;

		acquire_bus(g);
		write_cmd(g, SSD1306_SETSTARTLINE | 0);

		for(i=0; i < GDISP_SCREEN_HEIGHT/8 * SSD1306_PAGE_WIDTH; i+=SSD1306_PAGE_WIDTH)
			write_data(g, RAM(g)+i, SSD1306_PAGE_WIDTH);
		release_bus(g);
	}
#endif

#if GDISP_HARDWARE_DRAWPIXEL
	LLDSPEC void gdisp_lld_draw_pixel(GDisplay *g) {
		coord_t		x, y;

		switch(g->g.Orientation) {
		case GDISP_ROTATE_0:
			x = g->p.x;
			y = g->p.y;
			break;
		case GDISP_ROTATE_90:
			x = g->p.y;
			y = GDISP_SCREEN_HEIGHT-1 - g->p.x;
			break;
		case GDISP_ROTATE_180:
			x = GDISP_SCREEN_WIDTH-1 - g->p.x;
			y = GDISP_SCREEN_HEIGHT-1 - g->p.y;
			break;
		case GDISP_ROTATE_270:
			x = GDISP_SCREEN_HEIGHT-1 - g->p.y;
			x = g->p.x;
			break;
		}
		if (COLOR2NATIVE(g->p.color) != Black)
			RAM(g)[xyaddr(x, y)] |= xybit(y);
		else
			RAM(g)[xyaddr(x, y)] &= ~xybit(y);
		g->flags |= GDISP_FLG_NEEDFLUSH;
	}
#endif

#if GDISP_HARDWARE_PIXELREAD
	LLDSPEC color_t gdisp_lld_get_pixel_color(GDisplay *g) {
		coord_t		x, y;

		switch(g->g.Orientation) {
		case GDISP_ROTATE_0:
			x = g->p.x;
			y = g->p.y;
			break;
		case GDISP_ROTATE_90:
			x = g->p.y;
			y = GDISP_SCREEN_HEIGHT-1 - g->p.x;
			break;
		case GDISP_ROTATE_180:
			x = GDISP_SCREEN_WIDTH-1 - g->p.x;
			y = GDISP_SCREEN_HEIGHT-1 - g->p.y;
			break;
		case GDISP_ROTATE_270:
			x = GDISP_SCREEN_HEIGHT-1 - g->p.y;
			y = g->p.x;
			break;
		}
		return (RAM(g)[xyaddr(x, y)] & xybit(y)) ? White : Black;
	}
#endif

#if GDISP_NEED_CONTROL && GDISP_HARDWARE_CONTROL
	LLDSPEC void gdisp_lld_control(GDisplay *g) {
		switch(g->p.x) {
		case GDISP_CONTROL_POWER:
			if (g->g.Powermode == (powermode_t)g->p.ptr)
				return;
			switch((powermode_t)g->p.ptr) {
			case powerOff:
			case powerSleep:
			case powerDeepSleep:
				acquire_bus(g);
				write_cmd(g, SSD1306_DISPLAYOFF);
				release_bus(g);
				break;
			case powerOn:
				acquire_bus(g);
				write_cmd(g, SSD1306_DISPLAYON);
				release_bus(g);
				break;
			default:
				return;
			}
			g->g.Powermode = (powermode_t)g->p.ptr;
			return;

		case GDISP_CONTROL_ORIENTATION:
			if (g->g.Orientation == (orientation_t)g->p.ptr)
				return;
			switch((orientation_t)g->p.ptr) {
			/* Rotation is handled by the drawing routines */
			case GDISP_ROTATE_0:
			case GDISP_ROTATE_180:
				g->g.Height = GDISP_SCREEN_HEIGHT;
				g->g.Width = GDISP_SCREEN_WIDTH;
				break;
			case GDISP_ROTATE_90:
			case GDISP_ROTATE_270:
				g->g.Height = GDISP_SCREEN_WIDTH;
				g->g.Width = GDISP_SCREEN_HEIGHT;
				break;
			default:
				return;
			}
			g->g.Orientation = (orientation_t)g->p.ptr;
			return;

		case GDISP_CONTROL_CONTRAST:
            if ((unsigned)g->p.ptr > 100)
            	g->p.ptr = (void *)100;
			acquire_bus(g);
			write_cmd2(g, SSD1306_SETCONTRAST, (((unsigned)g->p.ptr)<<8)/101);
			release_bus(g);
            g->g.Contrast = (unsigned)g->p.ptr;
			return;

		// Our own special controller code to inverse the display
		// 0 = normal, 1 = inverse
		case GDISP_CONTROL_INVERSE:
			acquire_bus(g);
			write_cmd(g, g->p.ptr ? SSD1306_INVERTDISPLAY : SSD1306_NORMALDISPLAY);
			release_bus(g);
			return;
		}
	}
#endif // GDISP_NEED_CONTROL

#if GDISP_NEED_SCROLL
	/**
	 * @brief   Scroll vertically a section of the screen.
	 * @note    Optional.
	 * @note    If x,y + cx,cy is off the screen, the result is undefined.
	 * @note    If lines is >= cy, it is equivalent to a area fill with bgcolor.
	 *
	 * @param[in] x, y     The start of the area to be scrolled
	 * @param[in] cx, cy   The size of the area to be scrolled
	 * @param[in] lines    The number of lines to scroll (Can be positive or negative)
	 * @param[in] bgcolor  The color to fill the newly exposed area.
	 *
	 * @notapi
	 */
	LLDSPEC void gdisp_lld_vertical_scroll(GDisplay *g) {

		#ifdef GFX_USE_OS_CHIBIOS
			int32_t thdPriority = (int32_t)chThdGetPriority();
			chThdSetPriority(HIGHPRIO);
		#endif

		/* See datasheet table T10-1 for this*/
		uint8_t fHeight = g->p.y1;
		acquire_bus(g);
		write_cmd2(g, SSD1306_SETDISPLAYOFFSET, fHeight-2);
		write_cmd2(g, SSD1306_SETMULTIPLEX, (GDISP_SCREEN_HEIGHT - fHeight+1));

		/* Scrolling animation.*/
		for(int i=0; i<fHeight; i++){
			write_cmd(g, SSD1306_SETSTARTLINE | i);
			gfxSleepMilliseconds(20);
		}
		release_bus(g);

		/* Shift buffer up a font line.*/
		for (int i = 0; i < SSD1306_PAGE_WIDTH*(GDISP_SCREEN_HEIGHT/8-2); i++) {
			if(i % SSD1306_PAGE_WIDTH){
				RAM(g)[i]  = RAM(g)[i+SSD1306_PAGE_WIDTH*(fHeight/8)] >> fHeight % 8;
				RAM(g)[i] |= RAM(g)[i+SSD1306_PAGE_WIDTH*(fHeight/8 + 1)] << (8 - fHeight%8);
			}
		}

		/* Clear second last page.*/
		for(uint8_t i=0; i<2; i++)
			memset( RAM(g) + (GDISP_SCREEN_HEIGHT/8 -i)*SSD1306_PAGE_WIDTH - GDISP_SCREEN_WIDTH, 0, GDISP_SCREEN_WIDTH);

		#ifdef GFX_USE_OS_CHIBIOS
			chThdSetPriority(thdPriority);
		#endif

		/* Update display.*/
		gdisp_lld_flush(g);
	}
#endif // GDISP_NEED_SCROLL

#endif // GFX_USE_GDISP

