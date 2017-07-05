/** @file badge_eink_lut.h */
#ifndef BADGE_EINK_LUT_H
#define BADGE_EINK_LUT_H

#include <stdint.h>

struct badge_eink_lut_entry {
	/** the number of cycles the voltages are held; 0 = end of list */
	uint8_t length;

	/** bitmapped value containing voltages for every (old-bit, new-bit) pair:
	 * - bits 0,1: from 0 to 0
	 * - bits 2,3: from 0 to 1
	 * - bits 4,5: from 1 to 0
	 * - bits 6,7: from 1 to 1
	 *
	 * allowed values:
	 * - 0: VSS
	 * - 1: VSH
	 * - 2: VSL
	 */
	uint8_t voltages;
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
