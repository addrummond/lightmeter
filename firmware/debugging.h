#ifndef DEBUGGING_H
#define DEBUGGING_H

void debugging_putc(char c);
void debugging_write(const char *string, uint32_t length);
#define debugging_writec(s) debugging_write((s), sizeof(s)/sizeof(char))
void debugging_write_uint8(uint8_t i);
void debugging_write_uint16(uint16_t i);
void debugging_write_uint32(uint32_t i);

#endif
