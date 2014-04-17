#!/bin/sh

VUSB=./vusb-20121206/usbdrv/
CFLAGS=`cat CFLAGS`

python calculate_tables.py > tables.c

# Build files for vusb library.
avr-gcc $CFLAGS -o $VUSB/usbdrv.o -c $VUSB/usbdrv.c
avr-gcc $CFLAGS -o $VUSB/oddebug.o -c $VUSB/oddebug.c
avr-gcc $CFLAGS -o $VUSB/usbdrvasm.o -c $VUSB/usbdrvasm.S
VUSB_OBJS="$VUSB/usbdrv.o $VUSB/oddebug.o $VUSB/usbdrvasm.o"

avr-gcc $CFLAGS -o tables.out -c tables.c
avr-gcc $CFLAGS -o divmulutils.out -c divmulutils.c
avr-gcc $CFLAGS -o calculate.out -c calculate.c
avr-gcc $CFLAGS -o usbdevicedata.out -c usbdevicedata.c
avr-gcc $CFLAGS -o state.out -c state.c
avr-gcc $CFLAGS -o bcd.out -c bcd.c
avr-gcc $CFLAGS -o exposure.out -c exposure.c
avr-gcc $CFLAGS -o lightmeter.out -c lightmeter.c
avr-gcc $CFLAGS calculate.out tables.out divmulutils.out usbdevicedata.out state.out bcd.out exposure.out $VUSB_OBJS lightmeter.out -o main.out
avr-objcopy -O ihex -R .eeprom main.out lightmeter.hex
cp lightmeter.hex /tmp
