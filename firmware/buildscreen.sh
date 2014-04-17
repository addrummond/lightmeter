#!/bin/sh

CFLAGS=`cat CFLAGS`

cd bitmaps
python process_bitmaps.py output
cd ../

avr-gcc $CFLAGS -c screentest.c -o screentest.out
avr-gcc $CFLAGS -c bitmaps/bitmaps.c -o bitmaps.out
avr-gcc $CFLAGS -c ui.c -o ui.out
avr-gcc $CFLAGS -c state.c -o state.out
avr-gcc $CFLAGS -c display.c -o display.out
avr-gcc $CFLAGS bitmaps.out display.out ui.out state.out screentest.out -o screentestmain.out
avr-objcopy -O ihex -R .eeprom screentestmain.out screentest.hex
cp screentest.hex /tmp
