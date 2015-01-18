#include <state.h>
#include <exposure.h>

meter_state_t global_meter_state;

void write_meter_state(const meter_state_t *ms)
{
    // TODO.
}

void read_meter_state(meter_state_t *ms)
{
    // TODO.
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
