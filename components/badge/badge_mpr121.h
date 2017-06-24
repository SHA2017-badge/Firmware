/** @file badge_mpr121.h */
#ifndef BADGE_MPR121_H
#define BADGE_MPR121_H

#include <stdbool.h>
#include <stdint.h>

/** interrupt handler type */
typedef void (*badge_mpr121_intr_t)(void*, bool);

/** touch info */
struct badge_mpr121_touch_info {
	/** bitmapped touch state */
	uint32_t touch_state;

	/** the electrode data for touch inputs 0..7 (10 bits) */
	uint32_t data[8];

	/** the baseline for touch inputs 0..7 (8 bits) */
	uint32_t baseline[8];

	/** the touch threshold for touch inputs 0..7 (8 bits) */
	uint32_t touch[8];

	/** the release threshold for touch inputs 0..7 (8 bits) */
	uint32_t release[8];
};

/**
 * Initialize interrupt-handling for the MPR121.
 * @param baseline if not NULL, the MPR121 will be configured to use
 *   these baseline values for inputs 0..7.
 */
extern void badge_mpr121_init(const uint32_t *baseline);

/**
 * Configure interrupt handler for a specific pin.
 * @param pin the pin-number on the mpr121 chip.
 * @param handler the handler to be called on an interrupt.
 * @param arg the argument passed on to the handler.
 */
extern void badge_mpr121_set_interrupt_handler(uint8_t pin, badge_mpr121_intr_t handler, void *arg);

/**
 * Retrieve the mpr121 status.
 * @return the status registers; or -1 on error
 */
extern int badge_mpr121_get_interrupt_status(void);

/**
 * Retrieve mpr121 internal state (for the mpr121 demo)
 * @param data pointer to uint32_t data[32] data structure.
 * @return 0 on success; -1 on error
 * @deprecated use badge_mpr121_get_touch_info() instead.
 */
extern int badge_mpr121_get_electrode_data(uint32_t *data) __attribute__((deprecated));

/**
 * Retrieve mpr121 touch info
 * @param info touch info will be written to this structure.
 * @return 0 on success; -1 on error
 */
extern int badge_mpr121_get_touch_info(struct badge_mpr121_touch_info *info);

/** gpio config settings */
enum badge_mpr121_gpio_config
{
	MPR121_DISABLED         = 0x00,
	MPR121_INPUT            = 0x08,
	MPR121_INPUT_PULL_DOWN  = 0x09,
	MPR121_INPUT_PULL_UP    = 0x0b,
	MPR121_OUTPUT           = 0x0c,
	MPR121_OUTPUT_LOW_ONLY  = 0x0e,
	MPR121_OUTPUT_HIGH_ONLY = 0x0f,
};

/**
 * Configure GPIO pin on the MPR121.
 * @param pin the pin-number on the mpr121 chip.
 * @param config the new gpio pin config.
 * @return 0 on success; -1 on error
 */
extern int badge_mpr121_configure_gpio(int pin, enum badge_mpr121_gpio_config config);

/**
 * Retrieve the level of a GPIO pin.
 * @param pin the pin-number on the mpr121 chip.
 * @return 0 when low; 1 when high; -1 on error
 */
extern int badge_mpr121_get_gpio_level(int pin);

/**
 * Set the level of a GPIO pin.
 * @param pin the pin-number on the mpr121 chip.
 * @param value 0 is low; 1 is high
 * @return 0 on succes; -1 on error
 */
extern int badge_mpr121_set_gpio_level(int pin, int value);

#endif // BADGE_MPR121_H
