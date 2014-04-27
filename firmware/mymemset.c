#include <mymemset.h>

void memset8_zero(void *dest, uint8_t len)
{
    uint8_t i = len;
    do {
        --i;
        ((uint8_t *)dest)[i] = 0;
    } while (i > 0);
}
