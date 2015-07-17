#ifndef HAMMING_H
#define HAMMING_H

#include <stdint.h>
#include <stdbool.h>

uint32_t hammingify_uint32(uint32_t n);
int32_t dehammingify_uint32(uint32_t n);
uint32_t hamming_get_init_sequence_byte_length();
uint32_t hamming_get_encoded_message_byte_length(uint32_t len);
uint32_t hamming_get_encoded_message_byte_length_with_init_sequence(uint32_t len);
uint32_t hamming_get_max_output_length_given_input_length(uint32_t len);
void hamming_encode_message(const uint8_t *in, unsigned length, uint8_t *out, bool withInit);
void hamming_bitshift_buffer_forward(uint8_t *buffer, unsigned length, unsigned nbits);

typedef struct {
    int bit_index;
    unsigned count;
} hamming_scan_for_init_sequence_result_t;
hamming_scan_for_init_sequence_result_t hamming_scan_for_init_sequence(const uint8_t *input, unsigned length);

#endif
