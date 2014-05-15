#!/bin/sh

HEXFILE=$1
EEPROMFILE=$2

if [ "$1" != "X" ]; then
    echo "Programming flash file $HEXFILE..."
    avrdude -p attiny1634 -c avrispmkII -P usb -B 10 -Uflash:w:$HEXFILE
fi
if [ -n "$EEPROMFILE" ]; then
    echo "Programming eeprom file $EEPROMFILE..."
    avrdude -p attiny1634 -c avrispmkII -P usb -B 10 -Ueeprom:w:$EEPROMFILE
fi
