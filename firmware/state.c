#include <state.h>
#include <exposure.h>

#include <avr/eeprom.h>

meter_state_t global_meter_state = {
    { 0, 0, 0, 0, 1, 0, 0 },   // bcd_iso
    3,                         // bcd_iso_length
    4,                         // stops_iso

    SHUTTER_PRIORITY,

    48,                        // aperture (f8)
    96,                        // shutter speed (1/125)

    // aperture_string and shutter_speed_string are initialized in
    // initialize_global_meter_state();
};

void write_meter_state(const meter_state_t *ms)
{
    //    eeprom_write_block((void *)ms, STATE_BLOCK_START_ADDR, sizeof(meter_state_t));
}

void read_meter_state(meter_state_t *ms)
{
    //    eeprom_read_block(ms, STATE_BLOCK_START_ADDR, sizeof(meter_state_t));
}

void initialize_global_meter_state()
{
    aperture_to_string(global_meter_state.aperture, &(global_meter_state.aperture_string));
    shutter_speed_to_string(global_meter_state.shutter_speed, &(global_meter_state.shutter_speed_string));

    // We assume that EEPROM has never been written if the first byte is zero.
    // In this case, we don't do anything, since the global_meter_state variable
    // has already been initialized with the defaults.
    //    if (eeprom_read_byte((void *)0) != 0) {
    //        read_meter_state(&global_meter_state);
        //    }
}
