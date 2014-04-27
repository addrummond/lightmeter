#!/bin/sh

CFLAGS=`cat CFLAGS`
LINKFLAGS=`cat LINKFLAGS`

python calculate_tables.py output
cd bitmaps
python process_bitmaps.py output
cd ../

ST=-save-temps

avr-gcc $ST $CFLAGS -o tables.out -c tables.c
avr-gcc $ST $CFLAGS -o divmulutils.out -c divmulutils.c
avr-gcc $ST $CFLAGS -o calculate.out -c calculate.c
avr-gcc $ST $CFLAGS -o state.out -c state.c
avr-gcc $ST $CFLAGS -o bcd.out -c bcd.c
avr-gcc $ST $CFLAGS -o exposure.out -c exposure.c
avr-gcc $ST $CFLAGS -o display.out -c display.c
avr-gcc $ST $CFLAGS -o ui.out -c ui.c
avr-gcc $ST $CFLAGS -o bitmaps.out -c bitmaps/bitmaps.c
avr-gcc $ST $CFLAGS -o lightmeter.out -c lightmeter.c
avr-gcc $CFLAGS $LINKFLAGS calculate.out tables.out divmulutils.out state.out bcd.out exposure.out display.out ui.out bitmaps.out lightmeter.out -o main.out
avr-objcopy -O ihex -R .eeprom main.out lightmeter.hex
cp lightmeter.hex /tmp
