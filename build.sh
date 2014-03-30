#!/bin/sh

CFLAGS="-Wall -O2 -DF_CPU=8000000 -mmcu=attiny85"

python calculate_tables.py > tables.c

avr-gcc $CFLAGS -o tables.out -c tables.c
avr-gcc $CFLAGS -o calculate.out -c calculate.c
avr-gcc $CFLAGS -o lightmeter.out -c lightmeter.c
avr-gcc $CFLAGS calculate.out tables.out lightmeter.out -o main.out
avr-objcopy -O ihex -R .eeprom main.out lightmeter.hex
cp lightmeter.hex /tmp
