#ifndef UI_H
#define UI_H

#include <stdint.h>
#include <stddef.h>
#include <state.h>

uint8_t ui_top_status_line_at_6col(const meter_state_t *ms,
                                  uint8_t *out,
                                  uint8_t pages_per_col,
                                  uint8_t x);

size_t ui_main_reading_display_at_12col_state_size();
void ui_main_reading_display_at_12col(void *func_state_,
                                      const meter_state_t *ms,
                                      const transient_meter_state_t *tms,
                                      uint8_t *out,
                                      uint8_t pages_per_col,
                                      uint8_t x);

#endif
