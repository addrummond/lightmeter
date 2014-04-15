#!/bin/sh

CFLAGS="-I ./ -I./vusb-20121206/usbdrv/ -Wall -Wpadded -O2 -fno-move-loop-invariants -fno-tree-scev-cprop -fno-inline-small-functions -DF_CPU=16500000L -mmcu=attiny85"

avr-gcc $CFLAGS -c chargepumptest.c -o chargepumptest.out
avr-objcopy -O ihex -R .eeprom chargepumptest.out chargepumptest.hex
cp chargepumptest.hex /tmp
