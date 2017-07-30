#include <sdkconfig.h>

#ifdef CONFIG_SHA_BADGE_EINK_LUT_DEBUG
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#endif // CONFIG_SHA_BADGE_EINK_LUT_DEBUG

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <esp_log.h>

#include "badge_eink_dev.h"
#include "badge_eink_lut.h"

static const char *TAG = "badge_eink_lut";

// full, includes inverting
const struct badge_eink_lut_entry badge_eink_lut_full[] = {
	{ .length = 23, .voltages = 0x02, },
	{ .length =  4, .voltages = 0x01, },
	{ .length = 11, .voltages = 0x11, },
	{ .length =  4, .voltages = 0x12, },
	{ .length =  6, .voltages = 0x22, },
	{ .length =  5, .voltages = 0x66, },
	{ .length =  4, .voltages = 0x69, },
	{ .length =  5, .voltages = 0x59, },
	{ .length =  1, .voltages = 0x58, },
	{ .length = 14, .voltages = 0x99, },
	{ .length =  1, .voltages = 0x88, },
	{ .length = 0 }
};

// full, no inversion
const struct badge_eink_lut_entry badge_eink_lut_normal[] = {
	{ .length =  3, .voltages = 0x10, },
	{ .length =  5, .voltages = 0x18, },
	{ .length =  1, .voltages = 0x08, },
	{ .length =  8, .voltages = 0x18, },
	{ .length =  2, .voltages = 0x08, },
	{ .length = 0 }
};

// full, no inversion, needs 2 updates for full update
const struct badge_eink_lut_entry badge_eink_lut_faster[] = {
	{ .length =  1, .voltages = 0x10, },
	{ .length =  8, .voltages = 0x18, },
	{ .length =  1, .voltages = 0x08, },
	{ .length = 0 }
};

// full, no inversion, needs 4 updates for full update
const struct badge_eink_lut_entry badge_eink_lut_fastest[] = {
	{ .length =  1, .voltages = 0x10, },
	{ .length =  5, .voltages = 0x18, },
	{ .length =  1, .voltages = 0x08, },
	{ .length = 0 }
};

static uint8_t
badge_eink_lut_conv(uint8_t voltages, enum badge_eink_lut_flags flags)
{
	if (flags & LUT_FLAG_FIRST)
	{
		voltages |= voltages >> 4;
		voltages &= 15;
		if ((voltages & 3) == 3) // reserved
			voltages ^= 2; // set to '1': VSH (black)
		if ((voltages & 12) == 12) // reserved
			voltages ^= 4; // set to '2': VSL (white)
		voltages |= voltages << 4;
	}

	if (flags & LUT_FLAG_PARTIAL)
		voltages &= 0x3c; // only keep 0->1 and 1->0

	if (flags & LUT_FLAG_WHITE)
		voltages &= 0xcc; // only keep 0->1 and 1->1

	if (flags & LUT_FLAG_BLACK)
		voltages &= 0x33; // only keep 0->0 and 1->0

	return voltages;
}


// GDEH029A1
int
badge_eink_lut_generate_gdeh029a1(const struct badge_eink_lut_entry *list, enum badge_eink_lut_flags flags, uint8_t *lut)
{
	ESP_LOGD(TAG, "flags = %d.", flags);

	memset(lut, 0, 30);

	int pos = 0;
	while (list->length != 0)
	{
		uint8_t voltages = badge_eink_lut_conv(list->voltages, flags);
		int len = list->length;
		while (len > 0)
		{
			int plen = len > 15 ? 15 : len;
			if (pos == 20)
			{
				ESP_LOGE(TAG, "lut overflow.");
				return -1; // full
			}
			lut[pos] = voltages;
			if ((pos & 1) == 0)
				lut[20+(pos >> 1)] = plen;
			else
				lut[20+(pos >> 1)] |= plen << 4;
			len -= plen;
			pos++;
		}

		list = &list[1];
	}

	// the GDEH029A01 needs an empty update cycle at the end.
	if (pos == 20)
	{
		ESP_LOGE(TAG, "lut overflow.");
		return -1; // full
	}
	if ((pos & 1) == 0)
		lut[20+(pos >> 1)] = 1;
	else
		lut[20+(pos >> 1)] |= 1 << 4;

#ifdef CONFIG_SHA_BADGE_EINK_LUT_DEBUG
	{
		ESP_LOGD(TAG, "dump.");
		char line[10*3 + 1];
		char *lptr = line;
		int i;
		for (i=0; i<30; i++)
		{
			sprintf(lptr, " %02x", lut[i]);
			lptr = &lptr[3];
			if ((i % 10) == 9)
			{
				ESP_LOGD(TAG, "%s", line);
				lptr = line;
			}
		}
	}
#endif // CONFIG_SHA_BADGE_EINK_LUT_DEBUG

	return 30;
}

// DEPG0290B01
int
badge_eink_lut_generate_depg0290b1(const struct badge_eink_lut_entry *list, enum badge_eink_lut_flags flags, uint8_t *lut)
{
	ESP_LOGD(TAG, "flags = %d.", flags);

	memset(lut, 0, 70);

	int pos = 0;
	int spos = 0;
	while (list->length != 0)
	{
		int len = list->length;
		if (pos == 7)
		{
			ESP_LOGE(TAG, "lut overflow.");
			return -1; // full
		}
		uint8_t voltages = badge_eink_lut_conv(list->voltages, flags);

		lut[0*7 + pos] |= ((voltages >> 0) & 3) << ((3-spos)*2);
		lut[1*7 + pos] |= ((voltages >> 2) & 3) << ((3-spos)*2);
		lut[2*7 + pos] |= ((voltages >> 4) & 3) << ((3-spos)*2);
		lut[3*7 + pos] |= ((voltages >> 6) & 3) << ((3-spos)*2);
		lut[5*7 + pos*5 + spos] = len;
		lut[5*7 + pos*5 + spos] = len;

		spos++;
		if (spos == 2)
		{
			spos = 0;
			pos++;
		}

		list = &list[1];
	}

#ifdef CONFIG_SHA_BADGE_EINK_LUT_DEBUG
	{
		ESP_LOGD(TAG, "dump.");
		char line[3*7+1];
		char *lptr = line;
		int i;
		for (i=0; i<35; i++)
		{
			sprintf(lptr, " %02x", lut[i]);
			lptr = &lptr[3];
			if ((i % 7) == 6)
			{
				ESP_LOGD(TAG, "%s", line);
				lptr = line;
			}
		}
		for (; i<70; i++)
		{
			sprintf(lptr, " %02x", lut[i]);
			lptr = &lptr[3];
			if ((i % 5) == 4)
			{
				ESP_LOGD(TAG, "%s", line);
				lptr = line;
			}
		}
	}
#endif // CONFIG_SHA_BADGE_EINK_LUT_DEBUG

	return 70;
}

int
badge_eink_lut_generate(const struct badge_eink_lut_entry *list, enum badge_eink_lut_flags flags, uint8_t *lut)
{
	if ( badge_eink_dev_type == BADGE_EINK_GDEH029A1 )
	{
		return badge_eink_lut_generate_gdeh029a1(list, flags, lut);
	}

	if ( badge_eink_dev_type == BADGE_EINK_DEPG0290B1 )
	{
		return badge_eink_lut_generate_depg0290b1(list, flags, lut);
	}

	return 0;
}
