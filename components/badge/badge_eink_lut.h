/** @file badge_eink_lut.h */
#ifndef BADGE_EINK_LUT_H
#define BADGE_EINK_LUT_H

#include <stdint.h>

/** the needed size of the buffer used in badge_eink_generate_lut() */
#define BADGE_EINK_LUT_MAX_SIZE 70

/** specification of display update instruction */
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

/** filters to use on a badge_eink_lut_entry structure */
enum badge_eink_lut_flags {
	LUT_FLAG_FIRST    = 1, // do not depend on previous image
	LUT_FLAG_PARTIAL  = 2, // do not touch already correct pixels
	LUT_FLAG_WHITE    = 4, // white only
	LUT_FLAG_BLACK    = 8, // black only
};

__BEGIN_DECLS

/**
 * Generate LUT data for specific eink display.
 *
 * @param list screen updata data in 'generic' format.
 * @param flags optional alterations on generated lut data.
 * @param lut output data buffer. should be of size BADGE_EINK_LUT_MAX_SIZE.
 * @return lut length. returns -1 on error.
 */
extern int badge_eink_lut_generate(const struct badge_eink_lut_entry *list, enum badge_eink_lut_flags flags, uint8_t *lut);

/* pre-defined lookup-table display-updates. */

/** full screen update with inverse updates */
extern const struct badge_eink_lut_entry badge_eink_lut_full[];
/** screen update which just sets the black and white */
extern const struct badge_eink_lut_entry badge_eink_lut_normal[];
/** same as badge_eink_lut_normal but with shorter timings */
extern const struct badge_eink_lut_entry badge_eink_lut_faster[];
/** same as badge_eink_lut_faster but with shorter timings */
extern const struct badge_eink_lut_entry badge_eink_lut_fastest[];

__END_DECLS

#endif // BADGE_EINK_LUT_H
