#ifndef BADGE_EINK_LUT_H
#define BADGE_EINK_LUT_H

#include <stdint.h>

/* pre-defined lookup-table display-updates. size depends on model. */
extern const uint8_t badge_eink_lut_full[];     // full, includes inverting
extern const uint8_t badge_eink_lut_normal[];   // full, no inversion
extern const uint8_t badge_eink_lut_faster[];   // full, no inversion, needs 2 updates for full update
extern const uint8_t badge_eink_lut_fastest[];  // full, no inversion, needs 4 updates for full update

#endif // BADGE_EINK_LUT_H
