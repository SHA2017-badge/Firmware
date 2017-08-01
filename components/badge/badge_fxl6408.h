/** @file badge_fxl6408.h */
#ifndef BADGE_FXL6408_H
#define BADGE_FXL6408_H

#include <stdint.h>
#include <esp_err.h>

__BEGIN_DECLS

/** port-expander interrupt handler */
typedef void (*badge_fxl6408_intr_t)(void*);

/** initialize port-expander
 * @return ESP_OK on success; any other value indicates an error
 */
extern esp_err_t badge_fxl6408_init(void);

/** configure port-expander gpio port - io direction */
extern esp_err_t badge_fxl6408_set_io_direction(uint8_t pin, uint8_t direction);
/** configure port-expander gpio port - output state */
extern esp_err_t badge_fxl6408_set_output_state(uint8_t pin, uint8_t state);
/** configure port-expander gpio port - output high-z */
extern esp_err_t badge_fxl6408_set_output_high_z(uint8_t pin, uint8_t high_z);
/** configure port-expander gpio port - input default state */
extern esp_err_t badge_fxl6408_set_input_default_state(uint8_t pin, uint8_t state);
/** configure port-expander gpio port - pull enable */
extern esp_err_t badge_fxl6408_set_pull_enable(uint8_t pin, uint8_t enable);
/** configure port-expander gpio port - pull down/up */
extern esp_err_t badge_fxl6408_set_pull_down_up(uint8_t pin, uint8_t up);
/** configure port-expander gpio port - interrupt enable/disable */
extern esp_err_t badge_fxl6408_set_interrupt_enable(uint8_t pin, uint8_t enable);
/** configure port-expander gpio port - set interrupt callback method */
extern void badge_fxl6408_set_interrupt_handler(uint8_t pin, badge_fxl6408_intr_t handler, void *arg);

/** configure port-expander gpio port - get input status */
extern int badge_fxl6408_get_input(void);
/** configure port-expander gpio port - get interrupt status */
extern int badge_fxl6408_get_interrupt_status(void);

__END_DECLS

#endif // BADGE_FXL6408_H
