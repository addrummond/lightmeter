#!/bin/sh

HEXFILE=$1
EEPROMFILE=$2

echo "Programming flash file $HEXFILE and eeprom file $EEPROMFILE..."
avrdude -p attiny1634 -c avrispmkII -P usb -B 3 -Uflash:w:$HEXFILE -Ueeprom:w:$EEPROMFILE
