#ifndef PIEZO_H
#define PIEZO_H

void piezo_init(void);
void piezo_set_period(uint16_t period);
void piezo_turn_on(void);
void piezo_pause(void);
void piezo_deinit(void);

#endif
