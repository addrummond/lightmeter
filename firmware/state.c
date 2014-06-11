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

void initialize_global_meter_states()
{
    read_meter_state(&global_meter_state);

    global_transient_meter_state.op_amp_resistor_stage = 2;
}

transient_meter_state_t global_transient_meter_state = {
    { 80, 0 },
    { 48, 0 },
    { 88, 0 },

    false,

    1
};
