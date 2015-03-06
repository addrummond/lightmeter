#ifndef PIEZO_H
#define PIEZO_H

void piezo_init(void);
void piezo_set_periods(uint16_t period1, uint16_t period2);
void piezo_turn_on(void);
void piezo_pause(void);
void piezo_deinit(void);

#endif
