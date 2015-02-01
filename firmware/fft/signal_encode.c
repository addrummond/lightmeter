#include <stdint.h>
#include <fix_fft.h>

//
// Encoding scheme uses two signals, one at 20kHz and the other at 21kHz.
// The 20kHz signal is a clock signal. Data is read at the second consecutive
// rising edge. (I.e. two clock pulses for every data bit.)
//
// Data is transmitted in 8-byte chunks, which includes 1 byte of parity.
//
// Only initial byte can have the value 0xFF. This value is used to signal
// the beginning of a transmission.
//
// The value of the remaining bytes is interpreted literally if between 0 and
// 243 inclusive. If it is 244, then this byte is discarded and 1 is added to
// the value of the next byte.
//
// After each 8-byte chunk is transmitted, the receiver waits for an
// acknowledgement. This is either a parity error acknowledgement or an 'ok'
// acknowledgement with a parity byte for the whole 8-byte chunk (including
// the original parity byte).
//

#define SIGNAL_CHUNK_BYTE_LENGTH         32
#define SIGNAL_OUTPUT_ARRAY_LENGTH       256

// Returns number of bytes encoded.
// Output is an array of 256 16-bit signed integers.
unsigned encode_message(const uint8_t *message, int16_t *out)
{
    // Each iteration is 1/8 * (1/20000) of a second = 6.25e-06 seconds.

    unsigned i;
    for (i = 0, i < 256; i += 2) {
        unsigned mi = i / 8;


    }
}
