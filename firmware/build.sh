#!/bin/sh

XCFLAGS=`cat CFLAGS`
XCFLAGS="$XCFLAGS $CFLAGS"
CFLAGS="$XCFLAGS"
ST=-save-temps

CC=arm-none-eabi-gcc
CFLAGS="$ST $CFLAGS"

python calculate_tables.py output

cd bitmaps
python process_bitmaps.py output
cd ..

cd menus
python process_strings.py strings_english
cd ..

OBJS=""

# Arm STM stuff.
$CC $CFLAGS -c ./stm/system_stm32f0xx.c -o ./stm/system_stm32f0xx.o
OBJS="$OBJS ./stm/system_stm32f0xx.o"
$CC $CFLAGS -c ./stm/startup_stm32f030.s -o ./stm/startup_stm32f030.o
OBJS="$OBJS ./stm/startup_stm32f030.o"
$CC $CFLAGS -c ./stm/stm32f0xx_gpio.c -o ./stm/stm32f0xx_gpio.o
OBJS="$OBJS ./stm/stm32f0xx_gpio.o"
$CC $CFLAGS -c ./stm/stm32f0xx_rcc.c -o ./stm/stm32f0xx_rcc.o
OBJS="$OBJS ./stm/stm32f0xx_rcc.o"
$CC $CFLAGS -c ./stm/stm32f0xx_i2c.c -o ./stm/stm32f0xx_i2c.o
OBJS="$OBJS ./stm/stm32f0xx_i2c.o"

$CC $CFLAGS -c debugging.c -o debugging.o
OBJS="$OBJS debugging.o"
$CC $CFLAGS -c tables.c -o tables.o
OBJS="$OBJS tables.o"
$CC $CFLAGS -c myassert.c -o myassert.o
OBJS="$OBJS myassert.o"
$CC $CFLAGS -c mymemset.c -o mymemset.o
OBJS="$OBJS mymemset.o"
$CC $CFLAGS -c exposure.c -o exposure.o
OBJS="$OBJS exposure.o"
$CC $CFLAGS -c bcd.c -o bcd.o
OBJS="$OBJS bcd.o"
$CC $CFLAGS -c i2c.c -o i2c.o
OBJS="$OBJS i2c.o"
$CC $CFLAGS -c state.c -o state.o
OBJS="$OBJS state.o"
$CC $CFLAGS -c display.c -o display.o
OBJS="$OBJS display.o"
$CC $CFLAGS -c ui.c -o ui.o
OBJS="$OBJS ui.o"
$CC $CFLAGS -c bitmaps/bitmaps.c -o bitmaps/bitmaps.o
OBJS="$OBJS bitmaps/bitmaps.o"
$CC $CFLAGS -c menus/menu_strings.c -o menus/menu_strings.o
OBJS="$OBJS menus/menu_strings.o"
$CC $CFLAGS -c menus/menu_strings_table.c -o menus/menu_strings_table.o
OBJS="$OBJS menus/menu_strings_table.o"

$CC $CFLAGS -c main.c -o main.o
OBJS="$OBJS main.o"

$CC $CFLAGS -Wl,--gc-sections -T ./stm/STM32F030R8_FLASH.ld $OBJS -o out.elf
