/** @file badge_portexp.h */
#ifndef BADGE_PORTEXP_H
#define BADGE_PORTEXP_H

#include <stdint.h>

/** port-expander interrupt handler */
typedef void (*badge_portexp_intr_t)(void*);

/** initialize port-expander */
extern void badge_portexp_init(void);

/** configure port-expander gpio port - io direction */
extern int badge_portexp_set_io_direction(uint8_t pin, uint8_t direction);
/** configure port-expander gpio port - output state */
extern int badge_portexp_set_output_state(uint8_t pin, uint8_t state);
/** configure port-expander gpio port - output high-z */
extern int badge_portexp_set_output_high_z(uint8_t pin, uint8_t high_z);
/** configure port-expander gpio port - input default state */
extern int badge_portexp_set_input_default_state(uint8_t pin, uint8_t state);
/** configure port-expander gpio port - pull enable */
extern int badge_portexp_set_pull_enable(uint8_t pin, uint8_t enable);
/** configure port-expander gpio port - pull down/up */
extern int badge_portexp_set_pull_down_up(uint8_t pin, uint8_t up);
/** configure port-expander gpio port - interrupt enable/disable */
extern int badge_portexp_set_interrupt_enable(uint8_t pin, uint8_t enable);
/** configure port-expander gpio port - set interrupt callback method */
extern void badge_portexp_set_interrupt_handler(uint8_t pin, badge_portexp_intr_t handler, void *arg);

/** configure port-expander gpio port - get input status */
extern int badge_portexp_get_input(void);
/** configure port-expander gpio port - get interrupt status */
extern int badge_portexp_get_interrupt_status(void);

#endif // BADGE_PORTEXP_H
