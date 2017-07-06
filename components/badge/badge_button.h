/** @file badge_button.h */
#ifndef BADGE_BUTTON_H
#define BADGE_BUTTON_H

/** badge button constants */
enum badge_button_t {
	BADGE_BUTTON_UP     =  1,
	BADGE_BUTTON_DOWN   =  2,
	BADGE_BUTTON_LEFT   =  3,
	BADGE_BUTTON_RIGHT  =  4,

	BADGE_BUTTON_A      =  6,
	BADGE_BUTTON_B      =  7,
	BADGE_BUTTON_SELECT =  8,
	BADGE_BUTTON_START  =  9,
	BADGE_BUTTON_FLASH  = 10,

	// Number of buttons on the badge
	BADGE_BUTTONS       = 10,
};

#endif // BADGE_BUTTON_H
