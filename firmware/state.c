#include <state.h>
#include <exposure.h>

#include <avr/eeprom.h>

meter_state_t global_meter_state;

void write_meter_state(const meter_state_t *ms)
{
    eeprom_write_block((void *)ms, STATE_BLOCK_START_ADDR, sizeof(meter_state_t));
}

void read_meter_state(meter_state_t *ms)
{
    eeprom_read_block(ms, STATE_BLOCK_START_ADDR, sizeof(meter_state_t));
}

void initialize_global_meter_state()
{
    read_meter_state(&global_meter_state);
}

transient_meter_state_t global_transient_meter_state = {
    80,80,
    { { 0,0,0,0,0,0,0,0,0,0,0 }, 0 },
    { { 0,0,0,0,0,0,0,0,0}, 0 },
    0,
    false
};
