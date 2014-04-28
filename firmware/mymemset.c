#include <mymemset.h>

void memset8_zero(void *dest, uint8_t len)
{
    do {
        --len;
        ((uint8_t *)dest)[len] = 0;
    } while (len > 0);
}
