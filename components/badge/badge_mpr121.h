#ifndef BADGE_MPR121_H
#define BADGE_MPR121_H

typedef void (*badge_mpr121_intr_t)(void*);

extern void badge_mpr121_init(void);

extern void badge_mpr121_set_interrupt_handler(uint8_t pin, badge_mpr121_intr_t handler, void *arg);

extern int badge_mpr121_get_interrupt_status(void);

#endif // BADGE_MPR121_H
