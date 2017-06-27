/** @file badge_eink_lut.h */
#ifndef BADGE_EINK_LUT_H
#define BADGE_EINK_LUT_H

#include <stdint.h>

struct badge_eink_lut_entry {
	uint8_t length;    // 0 = end of list
	uint8_t voltages;  // bitmap: 11 10 01 00 (from .. to ..)
	                   // values: 0=VSS, 1=VSH, 2=VSL
};

enum badge_eink_lut_flags {
	LUT_FLAG_FIRST    = 1, // do not depend on previous image
	LUT_FLAG_PARTIAL  = 2, // do not touch already correct pixels
	LUT_FLAG_WHITE    = 4, // white only
	LUT_FLAG_BLACK    = 8, // black only
};

extern uint8_t * badge_eink_lut_generate(const struct badge_eink_lut_entry *list, enum badge_eink_lut_flags flags);

/* pre-defined lookup-table display-updates. */
extern const struct badge_eink_lut_entry badge_eink_lut_full[];
extern const struct badge_eink_lut_entry badge_eink_lut_normal[];
extern const struct badge_eink_lut_entry badge_eink_lut_faster[];
extern const struct badge_eink_lut_entry badge_eink_lut_fastest[];

#endif // BADGE_EINK_LUT_H
