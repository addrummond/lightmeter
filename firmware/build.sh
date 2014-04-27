#!/bin/sh

CFLAGS=`cat CFLAGS`
LINKFLAGS=`cat LINKFLAGS`

python calculate_tables.py output
cd bitmaps
python process_bitmaps.py output
cd ../

avr-gcc $CFLAGS -o tables.out -c tables.c
avr-gcc $CFLAGS -o divmulutils.out -c divmulutils.c
avr-gcc $CFLAGS -o calculate.out -c calculate.c
avr-gcc $CFLAGS -o state.out -c state.c
avr-gcc $CFLAGS -o bcd.out -c bcd.c
avr-gcc $CFLAGS -o exposure.out -c exposure.c
avr-gcc $CFLAGS -o display.out -c display.c
avr-gcc $CFLAGS -o ui.out -c ui.c
avr-gcc $CFLAGS -o bitmaps.out -c bitmaps/bitmaps.c
avr-gcc $CFLAGS -o lightmeter.out -c lightmeter.c
avr-gcc $CFLAGS $LINKFLAGS calculate.out tables.out divmulutils.out state.out bcd.out exposure.out display.out ui.out bitmaps.out lightmeter.out -o main.out
avr-objcopy -O ihex -R .eeprom main.out lightmeter.hex
cp lightmeter.hex /tmp
