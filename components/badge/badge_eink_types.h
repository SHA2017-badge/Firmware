/** @file badge_eink_types.h */
#ifndef BADGE_EINK_TYPES_H
#define BADGE_EINK_TYPES_H

#include <sdkconfig.h>

/**
 * Badge eink types.
 */
/* NOTE: This value is used in nvs. Do not change order or remove elements. */
enum badge_eink_dev_t {
	BADGE_EINK_NONE,
	BADGE_EINK_GDEH029A1,
	BADGE_EINK_DEPG0290B1,
};

/**
 * BADGE_EINK_DEFAULT specifies the compile-time default display type
 */
#if defined(CONFIG_SHA_BADGE_EINK_DEF_DEPG0290B1)
 #define BADGE_EINK_DEFAULT BADGE_EINK_DEPG0290B1
#elif defined(CONFIG_SHA_BADGE_EINK_DEF_GDEH029A1)
 #define BADGE_EINK_DEFAULT BADGE_EINK_GDEH029A1
#else
 #define BADGE_EINK_DEFAULT BADGE_EINK_NONE
#endif

#endif // BADGE_EINK_TYPES_H
