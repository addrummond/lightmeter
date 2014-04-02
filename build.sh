#!/bin/sh

CFLAGS="-I ./ -Wall -O2 -DF_CPU=16000000 -mmcu=attiny85"

python calculate_tables.py > tables.c

avr-gcc $CFLAGS -o tables.out -c tables.c
avr-gcc $CFLAGS -o divmulutils.out -c divmulutils.c
avr-gcc $CFLAGS -o calculate.out -c calculate.c
avr-gcc $CFLAGS -o led_debug.out -c led_debug.c
avr-gcc $CFLAGS -o lightmeter.out -c lightmeter.c
avr-gcc $CFLAGS calculate.out tables.out divmulutils.out led_debug.out lightmeter.out -o main.out
avr-objcopy -O ihex -R .eeprom main.out lightmeter.hex
cp lightmeter.hex /tmp
