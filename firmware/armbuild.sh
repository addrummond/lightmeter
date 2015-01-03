#!/bin/sh

CFLAGS=`cat CFLAGS`
LINKFLAGS=`cat LINKFLAGS`
ST=-save-temps

CC=arm-none-eabi-gcc
CFLAGS="$ST $CFLAGS"

OBJS=""

# Arm STM stuff.
$CC $CFLAGS -c ./stm/system_stm32f0xx.c -o ./stm/system_stm32f0xx.o
OBJS="$OBJS ./stm/system_stm32f0xx.o"
$CC $CFLAGS -c ./stm/startup_stm32f030.s -o ./stm/startup_stm32f030.o
OBJS="$OBJS ./stm/startup_stm32f030.o"

$CC $CFLAGS -c exposure.c -o exposure.o
OBJS="$OBJS exposure.o"
$CC $CFLAGS -c bcd.c -o bcd.o
OBJS="$OBJS bcd.o"

