#!/bin/sh

VUSB=./vusb-20121206/usbdrv/
CFLAGS="-I ./ -I./vusb-20121206/usbdrv/ -Wall -Wpadded -O2 -fno-move-loop-invariants -fno-tree-scev-cprop -fno-inline-small-functions -DF_CPU=16500000L -mmcu=attiny85"

avr-gcc $CFLAGS -c screentest.c -o screentest.out
avr-gcc $CFLAGS -c i2cmaster.S -o i2cmaster.out
avr-gcc $CFLAG usitwislave.out screentest.out -o screentestmain.out
avr-objcopy -O ihex -R .eeprom screentestmain.out screentest.hex
cp screentest.hex /tmp
