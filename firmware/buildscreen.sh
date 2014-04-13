#!/bin/sh

CFLAGS="-I ./ -I./vusb-20121206/usbdrv/ -Wall -Wpadded -O2 -fno-move-loop-invariants -fno-tree-scev-cprop -fno-inline-small-functions -DF_CPU=16500000L -mmcu=attiny85"

cd bitmaps
python process_bitmaps.py output
cd ../

avr-gcc $CFLAGS -c screentest.c -o screentest.out
avr-gcc $CFLAGS -c bitmaps/bitmaps.c -o bitmaps.out
avr-gcc $CFLAGS -c display.c -o display.out
avr-gcc $CFLAGS bitmaps.out display.out screentest.out -o screentestmain.out
avr-objcopy -O ihex -R .eeprom screentestmain.out screentest.hex
cp screentest.hex /tmp
