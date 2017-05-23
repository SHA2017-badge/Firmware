#ifndef BADGE_MPR121_H
#define BADGE_MPR121_H

typedef void (*badge_mpr121_intr_t)(void*);

extern void badge_mpr121_init(void);

extern int badge_mpr121_set_io_direction(uint8_t pin, uint8_t direction);
extern int badge_mpr121_set_output_state(uint8_t pin, uint8_t state);
extern int badge_mpr121_set_output_high_z(uint8_t pin, uint8_t high_z);
extern int badge_mpr121_set_input_default_state(uint8_t pin, uint8_t state);
extern int badge_mpr121_set_pull_enable(uint8_t pin, uint8_t enable);
extern int badge_mpr121_set_pull_down_up(uint8_t pin, uint8_t up);
extern int badge_mpr121_set_interrupt_enable(uint8_t pin, uint8_t enable);
extern void badge_mpr121_set_interrupt_handler(uint8_t pin, badge_mpr121_intr_t handler, void *arg);

extern int badge_mpr121_get_input(void);
extern int badge_mpr121_get_interrupt_status(void);

#endif // BADGE_MPR121_H
