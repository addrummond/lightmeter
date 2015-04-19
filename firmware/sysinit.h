#ifndef SYSINIT_H
#define SYSINIT_H

#include <stdbool.h>

void sysinit_init();
void sysinit_enter_sleep_mode();
bool sysinit_is_time_to_sleep();
void sysinit_reset_sleep_counter();
void sysinit_after_wakeup_init();

#endif
