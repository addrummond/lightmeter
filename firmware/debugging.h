#ifndef DEBUGGING_H
#define DEBUGGING_H

void debugging_putc(char c);
void debugging_write(const char *string, uint32_t length);
#define debugging_writec(s) debugging_write((s), sizeof(s)/sizeof(char))

#endif
