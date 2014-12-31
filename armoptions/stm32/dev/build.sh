#!/bin/sh
CC=~/opt/gcc-arm-none-eabi-4_9-2014q4/bin/arm-none-eabi-gcc
CFLAGS="-DSTM32F030 -nostdlib -I./include"
$CC $CFLAGS -c test.c -o test.o
$CC $CFLAGS -c startup_stm32f030.s -o startup_stm32f030.o
$CC $CFLAGS -c system_stm32f0xx.c -o system_stm32f0xx.o
$CC $CFLAGS -T STM32F030R8_FLASH.ld test.o startup_stm32f030.o system_stm32f0xx.o -o test.elf

