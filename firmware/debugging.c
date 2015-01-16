#include <stdint.h>

// See http://electronics.stackexchange.com/questions/149387/how-do-i-print-debug-messages-to-gdb-console-with-stm32-discovery-board-using-gd/149403#149403

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
