#ifndef SYSTIME_H
#define SYSTIME_H

#include <stdint.h>

typedef struct {
    uint32_t st, en;
} systime_var_pair_t;

#define SYSTIME_START(name)  systime_var_pair_t name = { SysTick->VAL, 0 }
#define SYSTIME_UPDATE(name) do { name.st = SysTick->VAL; } while (0)
#define SYSTIME_WAIT_UNTIL_CYCLES_(name, cycles, op, expr)        \
    do {                                                          \
        name.en = name.st - (cycles);                             \
        if (name.st > name.en)                                    \
            while ((SysTick->VAL > name.en) op (expr));           \
        else {                                                    \
            uint32_t v;                                           \
            do {                                                  \
                v = SysTick->VAL;                                 \
            } while ((v > name.en || v <= name.st) op (expr));    \
        }                                                         \
    } while(0)
#define SYSTIME_WAIT_UNTIL_CYCLES_OR(name, cycles, expr) \
    SYSTIME_WAIT_UNTIL_CYCLES_(name, (cycles), &&, (! expr))
#define SYSTIME_UNTIL_FOR_CYCLES_AND(name, cycles, expr) \
    SYSTIME_WAIT_UNTIL_CYCLES_(name, (cycles), ||, (! (expr))
#define SYSTIME_WAIT_UNTIL_CYCLES(name, cycles) \
    SYSTIME_WAIT_UNTIL_CYCLES_(name, (cycles), ||, 0)

#endif
