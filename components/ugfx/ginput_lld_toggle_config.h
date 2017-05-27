/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#ifndef _GDISP_LLD_TOGGLE_BOARD_H
#define _GDISP_LLD_TOGGLE_BOARD_H

// The below are example values

#define GINPUT_TOGGLE_NUM_PORTS			7			// The total number of toggle inputs
#define GINPUT_TOGGLE_CONFIG_ENTRIES	1			// The total number of GToggleConfig entries

#define GINPUT_TOGGLE_A			0				// Switch 1
#define GINPUT_TOGGLE_B			1				// Switch 2
#define GINPUT_TOGGLE_SELECT			2				// Joystick Up
#define GINPUT_TOGGLE_UP			3				// Joystick Up
#define GINPUT_TOGGLE_DOWN			4				// Joystick Down
#define GINPUT_TOGGLE_LEFT			5				// Joystick Left
#define GINPUT_TOGGLE_RIGHT			6				// Joystick Right

#define GINPUT_TOGGLE_DECLARE_STRUCTURE()											\
	const GToggleConfig GInputToggleConfigTable[GINPUT_TOGGLE_CONFIG_ENTRIES] = {	\
		{}														\
	}

#endif /* _GDISP_LLD_TOGGLE_BOARD_H */
