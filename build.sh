#!/bin/sh

python calculate_tables.py > tables.c

avr-gcc -Wall -O2 -DF_CPU=8000000 -mmcu=attiny85 -o lightmeter.out lightmeter.c
avr-objcopy -O ihex -R .eeprom lightmeter.out lightmeter.hex
cp lightmeter.hex /tmp

