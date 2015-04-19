#!/bin/sh

XCFLAGS=`cat CFLAGS`
XCFLAGS="$XCFLAGS $CFLAGS"
CFLAGS="$XCFLAGS"
ST=-save-temps

CC=arm-none-eabi-gcc
CFLAGS="$ST $CFLAGS"

python3 calculate_tables.py output

cd bitmaps
python3 process_bitmaps.py output
cd ..

cd menus
python3 process_strings.py strings_english
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
$CC $CFLAGS -c ./stm/stm32f0xx_adc.c -o ./stm/stm32f0xx_adc.o
OBJS="$OBJS ./stm/stm32f0xx_adc.o"
$CC $CFLAGS -c ./stm/stm32f0xx_tim.c -o ./stm/stm32f0xx_tim.o
OBJS="$OBJS ./stm/stm32f0xx_tim.o"
$CC $CFLAGS -c ./stm/stm32f0xx_misc.c -o ./stm/stm32f0xx_misc.o
OBJS="$OBJS ./stm/stm32f0xx_misc.o"
$CC $CFLAGS -c ./stm/stm32f0xx_dma.c -o ./stm/stm32f0xx_dma.o
OBJS="$OBJS ./stm/stm32f0xx_dma.o"
$CC $CFLAGS -c ./stm/stm32f0xx_syscfg.c -o ./stm/stm32f0xx_syscfg.o
OBJS="$OBJS ./stm/stm32f0xx_syscfg.o"
$CC $CFLAGS -c ./stm/stm32f0xx_exti.c -o ./stm/stm32f0xx_exti.o
OBJS="$OBJS ./stm/stm32f0xx_exti.o"
$CC $CFLAGS -c ./stm/stm32f0xx_pwr.c -o ./stm/stm32f0xx_pwr.o
OBJS="$OBJS ./stm/stm32f0xx_pwr.o"

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
$CC $CFLAGS -c meter.c -o meter.o
OBJS="$OBJS meter.o"
$CC $CFLAGS -c bcd.c -o bcd.o
OBJS="$OBJS bcd.o"
$CC $CFLAGS -c i2c.c -o i2c.o
OBJS="$OBJS i2c.o"
$CC $CFLAGS -c state.c -o state.o
OBJS="$OBJS state.o"
$CC $CFLAGS -c buttons.c -o buttons.o
OBJS="$OBJS buttons.o"
$CC $CFLAGS -c accel.c -o accel.o
OBJS="$OBJS accel.o"
$CC $CFLAGS -c display.c -o display.o
OBJS="$OBJS display.o"
$CC $CFLAGS -c piezo.c -o piezo.o
OBJS="$OBJS piezo.o"
$CC $CFLAGS -c ui.c -o ui.o
OBJS="$OBJS ui.o"
$CC $CFLAGS -c bitmaps/bitmaps.c -o bitmaps/bitmaps.o
OBJS="$OBJS bitmaps/bitmaps.o"
$CC $CFLAGS -c menus/menu_strings.c -o menus/menu_strings.o
OBJS="$OBJS menus/menu_strings.o"
$CC $CFLAGS -c menus/menu_strings_table.c -o menus/menu_strings_table.o
OBJS="$OBJS menus/menu_strings_table.o"
$CC $CFLAGS -c goetzel.c -o goetzel.o
OBJS="$OBJS goetzel.o"

$CC $CFLAGS -c main.c -o main.o
OBJS="$OBJS main.o"

$CC $CFLAGS -Wl,--gc-sections -T ./stm/STM32F030K6_FLASH.ld $OBJS -o out.elf

arm-none-eabi-objcopy -O binary out.elf out.bin
cp out.bin /tmp
