#define REFERENCE_VOLTAGE_MV 5500

// Divide by number specified, then << by number specified.
#define OP_AMP_GAIN_DIVIDE 5
#define OP_AMP_GAIN_LSHIFT 0

// See p. 133 of atiny85 data sheet.
// TODO: At some point these values should be
// moved to EPROM so that they can be set differently for each chip
// without recompiling.
#define ADC_TEMPERATURE_OFFSET 0
