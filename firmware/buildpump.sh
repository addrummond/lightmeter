#!/bin/sh

CFLAGS=`cat CFLAGS`

avr-gcc $CFLAGS -c chargepumptest.c -o chargepumptest.out
avr-objcopy -O ihex -R .eeprom chargepumptest.out chargepumptest.hex
cp chargepumptest.hex /tmp
