#!/bin/sh

CFLAGS=`cat CFLAGS`

python calculate_tables.py > tables.c

avr-gcc $CFLAGS -o tables.out -c tables.c
avr-gcc $CFLAGS -o divmulutils.out -c divmulutils.c
avr-gcc $CFLAGS -o calculate.out -c calculate.c
avr-gcc $CFLAGS -o state.out -c state.c
avr-gcc $CFLAGS -o bcd.out -c bcd.c
avr-gcc $CFLAGS -o exposure.out -c exposure.c
avr-gcc $CFLAGS -o lightmeter.out -c lightmeter.c
avr-gcc $CFLAGS calculate.out tables.out divmulutils.out state.out bcd.out exposure.out lightmeter.out -o main.out
avr-objcopy -O ihex -R .eeprom main.out lightmeter.hex
cp lightmeter.hex /tmp
