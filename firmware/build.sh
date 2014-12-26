#!/bin/sh

CFLAGS=`cat CFLAGS`
LINKFLAGS=`cat LINKFLAGS`
ST=-save-temps

if [ "$1" == "debug" ]
then
    echo "Building in debug mode"
    CFLAGS="$CFLAGS -DDEBUG"
fi
if [ "$1" == "pdebug" ]
then
    echo "Building in protohack debug mode"
    CFLAGS="$CFLAGS -DDEBUG -DPDEBUG"
fi

python output_initial_eeprom.py

python calculate_tables.py output

cd bitmaps
python process_bitmaps.py output
cd ../

cd menus
python process_strings.py strings_english
cd ../

avr-gcc $ST $CFLAGS -c basic_serial/basic_serial.S -o basic_serial/basic_serial.out &&
avr-gcc $ST $CFLAGS -o mymemset.out -c mymemset.c &&
avr-gcc $ST $CFLAGS -o tables.out -c tables.c &&
avr-gcc $ST $CFLAGS -o state.out -c state.c &&
avr-gcc $ST $CFLAGS -o bcd.out -c bcd.c &&
avr-gcc $ST $CFLAGS -o exposure.out -c exposure.c &&
avr-gcc $ST $CFLAGS -o display.out -c display.c &&
avr-gcc $ST $CFLAGS -o ui.out -c ui.c &&
avr-gcc $ST $CFLAGS -o bitmaps.out -c bitmaps/bitmaps.c &&
avr-gcc $ST $CFLAGS -o menu_strings_table.out -c menus/menu_strings_table.c &&
avr-gcc $ST $CFLAGS -o menu_strings.out -c menus/menu_strings.c &&
avr-gcc $ST $CFLAGS -o lightmeter.out -c lightmeter.c &&
avr-gcc $ST $CFLAGS -o shiftregister.out -c shiftregister.c &&
avr-gcc $ST $CFLAGS -o accel.out -c accel.c &&
avr-gcc $ST $CFLAGS -o i2c.out -c i2c.c &&
avr-gcc $CFLAGS $LINKFLAGS mymemset.out tables.out state.out bcd.out exposure.out display.out ui.out bitmaps.out basic_serial/basic_serial.out menu_strings_table.out menu_strings.out lightmeter.out shiftregister.out accel.out i2c.out -o main.out
avr-objcopy -O ihex -R .eeprom main.out lightmeter.hex &&
cp lightmeter.hex /tmp
