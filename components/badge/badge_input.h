/** @file badge_input.h */
#ifndef BADGE_INPUT_H
#define BADGE_INPUT_H

#include <stdbool.h>
#include <stdint.h>

extern void badge_input_init(void);

#define EVENT_BUTTON_RELEASED false
#define EVENT_BUTTON_PRESSED  true
#define NOT_IN_ISR false
#define IN_ISR true
extern void badge_input_add_event(uint32_t button_id, bool pressed, bool in_isr);

// returns button_id if button is pressed.
// returns 0 if timeout is reached
// timeout is in milliseconds; -1 is infinity
extern uint32_t badge_input_get_event(int timeout);

extern uint32_t badge_input_button_state;

extern void (*badge_input_notify)(void);

#endif // BADGE_INPUT_H
