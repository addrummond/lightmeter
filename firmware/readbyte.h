#ifdef __AVR__
#include <avr/pgmspace.h>
#else
#define pgm_read_byte(x) (*(x))
#define PROGMEM
#endif
