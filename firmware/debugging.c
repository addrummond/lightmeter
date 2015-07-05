#include <stdint.h>

// See http://electronics.stackexchange.com/questions/149387/how-do-i-print-debug-messages-to-gdb-console-with-stm32-discovery-board-using-gd/149403#149403

#ifndef TEST
static void send_command(int command, void *message)
{
    asm(
        "mov r0, %[cmd];"
        "mov r1, %[msg];"
        "bkpt #0xAB"
          :
          : [cmd] "r" (command), [msg] "r" (message)
          : "r0", "r1", "memory"
    );
}

void debugging_write(const char *string, uint32_t length)
{
    uint32_t msg[] = { 2/*stderr*/, (uint32_t)string, length };
    send_command(0x05, msg);
}

void debugging_putc(char c)
{
    asm (
        "mov r0, #0x03\n"	// SYS_WRITEC
        "mov r1, %[msg]\n"
        "bkpt #0xAB\n"
          :
          : [msg] "r" (&c)
          : "r0", "r1"
    );
}
#endif

static char *debugging_write_uint32_to_string(char *ds, uint32_t i)
{
    unsigned j = 9;
    uint32_t d = 1;
    do {
        ds[j] = '0' + ((i/d) % 10);
        d *= 10;
    } while (j-- > 0);
    j = 0;
    while (*ds == '0' && ++j < 10)
        ++ds;
    return ds;
}

static char *debugging_write_int32_to_string(char *ds, int32_t i)
{
    if (i >= 0) {
        return debugging_write_uint32_to_string(ds, (uint32_t)i);
    }
    else {
        char *beg = debugging_write_uint32_to_string(ds, (uint32_t)-i);
        --beg;
        *beg = '-';
        return beg;
    }
}

#ifndef TEST
void debugging_write_uint32(uint32_t i)
{
    char ds[10];
    char *beg = debugging_write_uint32_to_string(ds, i);
    debugging_write(beg, 10-(beg-ds));
}

void debugging_write_int32(int32_t i)
{
    char ds[11];
    char *beg = debugging_write_int32_to_string(ds, i);
    debugging_write(beg, 11-(beg-ds));
}

void debugging_write_uint16(uint16_t i)
{
    debugging_write_uint32(i);
}

void debugging_write_uint8(uint8_t i)
{
    debugging_write_uint32(i);
}
#endif

#ifdef TEST

#include <stdio.h>
#include <string.h>

int main()
{
    char s8[11];
    s8[10] = '\0';
    uint32_t i = 0;
    const char *beg;
    do {
        beg = debugging_write_uint32_to_string(s8, i);
        printf("%s\n", beg);
    } while (i++ < 65535);
    beg = debugging_write_uint32_to_string(s8, 1000000023);
    printf("%s\n", beg);

    printf("\n\n*****\n\n");

    int32_t j;
    char s8_2[12];
    s8_2[11] = '\0';
    j = -65536;
    do {
        beg = debugging_write_int32_to_string(s8_2, j);
        printf("%s\n", beg);
    } while (j++ < 65536);
}

#endif
