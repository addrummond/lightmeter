#include <state.h>
#include <exposure.h>
#include <mymemset.h>

meter_state_t global_meter_state;
transient_meter_state_t global_transient_meter_state;

void write_meter_state(const meter_state_t *ms)
{
    // TODO.
}

void read_meter_state(meter_state_t *ms)
{
    // TODO.
}

void initialize_global_meter_state()
{
    meter_state_t *gms = &global_meter_state;

    // Set everything to 0 by default.
    memset8_zero(gms, sizeof(global_meter_state));

    // Set ISO 100 as default ISO.
    memset8_zero(&(gms->bcd_iso_digits), sizeof(global_meter_state.bcd_iso_digits));
    gms->bcd_iso_digits[0] = 1;
    gms->bcd_iso_length = 2;
    gms->iso = 12;

    // Set default fixing to ISO.
    gms->fixing = FIXING_ISO;

    ev_with_fracs_init(gms->exp_comp);
    gms->exp_comp_sign = 1;

    gms->ui_mode = UI_MODE_INIT;
    memset8_zero(&(gms->ui_mode_state), sizeof(global_meter_state.ui_mode_state));

    gms->meter_mode = METER_MODE_REFLECTIVE;

    gms->precision_mode = PRECISION_MODE_TENTH;

    ev_with_fracs_init(gms->fixed_shutter_speed);
    ev_with_fracs_init(gms->fixed_aperture);
    gms->fixed_iso = 6; // TODO TODO CHECK
}

void initialize_global_transient_meter_state()
{
    transient_meter_state_t *tms = &global_transient_meter_state;

    ev_with_fracs_init(tms->last_ev_with_fracs);
    ev_with_fracs_init(tms->aperture);
    ev_with_fracs_init(tms->shutter_speed);
    tms->iso = 0;

    tms->exposure_ready = false;
}
