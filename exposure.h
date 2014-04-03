#ifndef EXPOSURE_H
#define EXPOSURE_H

#define SS_1M       0
#define SS_8S       24
#define SS_1S       48
#define SS_8TH      64
#define SS_15TH     80
#define SS_1000TH   128
#define SS_8000TH   152
#define SS_16000TH  160

#define SS_MIN 0
#define SS_MAX 160

#define AP_F8       40

#define AP_MIN      0
#define AP_MAX      72

typedef struct exposure_string_output {
    uint8_t chars[9];
    uint8_t length; // Does not include null terminator.
} exposure_string_output_t;

typedef struct aperture_string_output {
    uint8_t chars[4];
    uint8_t length; // Does not incluze null terminator.
} aperture_string_output_t;

#endif
