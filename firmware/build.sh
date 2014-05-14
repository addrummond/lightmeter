#!/bin/sh

CFLAGS=`cat CFLAGS`
LINKFLAGS=`cat LINKFLAGS`

if [ "$1" == "debug" ]
then
    echo "Building in debug mode"
    CFLAGS="$CFLAGS -DDEBUG"
fi

python output_initial_eeprom.py
cp lightmeter.eep /tmp

python calculate_tables.py output
cd bitmaps
python process_bitmaps.py output
cd ../

ST=-save-temps

avr-gcc $ST $CFLAGS -c basic_serial/basic_serial.S -o basic_serial/basic_serial.out &&
avr-gcc $ST $CFLAGS -o mymemset.out -c mymemset.c &&
avr-gcc $ST $CFLAGS -o tables.out -c tables.c &&
avr-gcc $ST $CFLAGS -o state.out -c state.c &&
avr-gcc $ST $CFLAGS -o bcd.out -c bcd.c &&
avr-gcc $ST $CFLAGS -o exposure.out -c exposure.c &&
avr-gcc $ST $CFLAGS -o display.out -c display.c &&
avr-gcc $ST $CFLAGS -o ui.out -c ui.c &&
avr-gcc $ST $CFLAGS -o bitmaps.out -c bitmaps/bitmaps.c &&
avr-gcc $ST $CFLAGS -o lightmeter.out -c lightmeter.c &&
avr-gcc $CFLAGS $LINKFLAGS mymemset.out tables.out state.out bcd.out exposure.out display.out ui.out bitmaps.out basic_serial/basic_serial.out lightmeter.out -o main.out
avr-objcopy -O ihex -R .eeprom main.out lightmeter.hex &&
cp lightmeter.hex /tmp
