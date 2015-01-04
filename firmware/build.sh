#!/bin/sh

CFLAGS=`cat CFLAGS`
ST=-save-temps

CC=arm-none-eabi-gcc
CFLAGS="$ST $CFLAGS"

python calculate_tables.py output

OBJS=""

# Arm STM stuff.
$CC $CFLAGS -c ./stm/system_stm32f0xx.c -o ./stm/system_stm32f0xx.o
OBJS="$OBJS ./stm/system_stm32f0xx.o"
$CC $CFLAGS -c ./stm/startup_stm32f030.s -o ./stm/startup_stm32f030.o
OBJS="$OBJS ./stm/startup_stm32f030.o"

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
$CC $CFLAGS -c ./stm/stm32f0xx_gpio.c -o ./stm/stm32f0xx_gpio.o
OBJS="$OBJS ./stm/stm32f0xx_gpio.o"
$CC $CFLAGS -c ./stm/stm32f0xx_rcc.c -o ./stm/stm32f0xx_rcc.o
OBJS="$OBJS ./stm/stm32f0xx_rcc.o"
$CC $CFLAGS -c ./stm/stm32f0xx_i2c.c -o ./stm/stm32f0xx_i2c.o
OBJS="$OBJS ./stm/stm32f0xx_i2c.o"
$CC $CFLAGS -c main.c -o main.o
OBJS="$OBJS main.o"

$CC $CFLAGS -T ./stm/STM32F030R8_FLASH.ld $OBJS -o out.o
