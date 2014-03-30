#include <stdint.h>

//
// Mapping from units to unsigned 8-bit quantities.
//
// Temperature: 8-bit, -51C to 51C in 0.4C intervals.
// Voltage:     8-bit, 250mV to 550mV in 1.18mV intervals.
// Current:     8-bit, log10 current in pA with offset +1.0, fixed point 00.000000
// Irradience:  8-bit, log10 irradience in microW multiplied by 60.
// EV:          8-bit, 1/8th EV units with 0 = -4.
//

// Open circuit voltage for the photodiode at a given temperature.
//
// http://www.vishay.com/docs/80085/measurem.pdf (Fig 20, p. 7)
// 
// Equation for line: 425 - (temperature*2.5)
// Temp is between -51C (=0) and 51C (=255) in intervals of 0.4C.
// OCV is between 250mV (=0) and 550mV (=255) in intervals of 1.18 mV.
// Revised equation: 264 - (temperature*0.86)
//                  = (255 - (temperature*0.86)) + 9
uint8_t ocv_at_temperature(uint8_t temp)
{
    // Approximating multiply by 0.86 as subtract 1/7th of original.
    return 255 - (temperature - (temperature / 7)) + 9;
}

// Reverse dark current vs. temperature.
//
// http://www.vishay.com/docs/81521/bpw34.pdf (Fig 1. p. 2)
// http://www.vishay.com/docs/80085/measurem.pdf (Fig 18, p.7, lower line)
//
// Slope with log10 RDC is 1/25, or 1/63 in units of 0.4C.
// Constant is 2.41 when current is in pA (10^n -> 10^n+3), but 1.41 because of 1.0 offset.
// Slope becomes 1 when we take into account 00.00... fix point.
//
uint8_t rdc_at_temperature(uint8_t temperature)
{
    return temperature + 0b01011010; // 1.41
}

// The graph in Fig 19 of p. 7 of http://www.vishay.com/docs/80085/measurem.pdf
// shows 5 lines:
//
//     (i)         10pA,
//     (ii)        100 pA,
//     (iii) 1nA  = 1000pA,
//     (iv)  5nA  = 5000pA,
//     (v)   30nA = 30,000 pA
//
// Case (v) is N/A (it's never going to get that hot).
//
// Slope of all lines (with log x axis) is 60.
//
//
uint8_t irad_at_ocv_and_rdc(uint8_t ocv, uint8_t rdc) 
{
    uint8_t k = 11; // 12.5V
    if (rdc >= 0b10100011) // [log10 3612]-1
        k = 25; // 30V
    else if (rdc >= 0b01111111) // [log10 1000]-1
        k = 42; // 50V
    else if (rdc >= 0b01011111) // [log10 316]-1
        k = 64; // 75V
    else if (rdc >= 0b01000000) // [log10 100]-1
        k = 85; // 100V
    else if (rdc >= 0b00100000) // [log10 32]-1
        k = 106; // 125V
    else
        k = 127; // 150V

    // log10 irradience * 60. (slope is 60).
    return ocv - k;
}

// This table maps [log10 irrad]*60 to EV.
uint8_t irrad_to_ev[] = {
    /* -1 */    24, X,
    /* -0.5 */  28, X
    /* 0 */     32, X,
    /* 0.5 */   36, X,
    /* 1 */     40, X,
    /* 1.5  */  44, X,
    /* 2 */     48, X,
    /* 2.5 */   54, X,
    /* 3 */     58, X,
    /* 4 */     62, X,
    /* 4.5 */   64, X,
    /* 5 */     68, X,
    /* 5.5 */   72, X,
    /* 6 */     76, X,
    /* 6.5 */   80, X,
    /* 7 */     84, X,
    /* 7.5 */   88, X,
    /* 8 */     92, X,
    /* 8.5 */   96, X,
    /* 9 */     100, X,
    /* 9.5 */   104, X,
    /* 10 */    108, X,
    /* 10.5 */  112, X,
    /* 11 */    116, X,
    /* 11.5 */  120, X,
    /* 12 */    124, X,
    /* 12.5 */  128, X,
    /* 13 */    132, X,
    /* 13.5 */  136, X,
    /* 14 */    140, X,
    /* 15 */    144, X
    /* 15.5 */  148, X,
    /* 16 */    152, X,
    /* 16.5 */  156, X,
    /* 17 */    160, X,
    /* 17.5 */  164, X,
    /* 18 */    168, X
};


