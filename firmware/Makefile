#
# See the following for a description of the method used to automatically
# calculate dependencies for C modules.
#
#     http://scottmcpeak.com/autodepend/autodepend.html
#

# ARM architecture object files have the extension .out.
# Build platform architecture object files have the extension .o.

ARMCC := arm-none-eabi-gcc
ARMCFLAGS := -g -Wall -Os -mcpu=cortex-m0 -ffunction-sections -fdata-sections -nostdlib -mthumb -DSTM32F030 -DUSE_FULL_ASSERT -I ./ -I ./stm -Wall
OBJS := accel.out bcd.out buttons.out debugging.out display.out exposure.out goetzel.out \
        hamming.out hfsdp.out i2c.out main.out meter.out myassert.out mymemset.out piezo.out \
        state.out sysinit.out ui.out menus/menu_strings.out menus/menu_strings_table.out \
        bitmaps/bitmaps.out tables.out \
        $(patsubst %.c,%.out,$(shell echo stm/*.c))

bitmaps/bitmaps.h bitmaps/bitmaps.c: $(shell find ./bitmaps -name '*.png') bitmaps/process_bitmaps.py
	cd bitmaps && python3 process_bitmaps.py output
bitmaps/bitmaps.c: bitmaps/bitmaps.h

menus/menu_strings_table.h menus/menus_strings_table.c: menus/strings_english menus/process_strings.py
	cd menus && python3 process_strings.py strings_english
menus/menus_strings_table.c: menus/menu_strings_table.h

tables.h tables.c: calculate_tables.py
	python3 calculate_tables.py output
tables.c: tables.h

stm/startup_stm32f030.out: stm/startup_stm32f030.s
	$(ARMCC) $(ARMCFLAGS) -c stm/startup_stm32f030.s -o stm/startup_stm32f030.out

.PHONY: prereq
prereq: bitmaps/bitmaps.c menus/menus_strings_table.c tables.c stm/startup_stm32f030.out

-include $(OBJS:.out=.d)

%.out: %.c
	$(ARMCC) -c $(ARMCFLAGS) $*.c -o $*.out
	$(ARMCC) -MM $(ARMCFLAGS) $*.c > $*.d

/stm/STM32F030K6_FLASH.ld: /stm/STM32F030K6_FLASH.ld

out.elf: prereq /stm/STM32F030K6_FLASH.ld stm/startup_stm32f030.out $(OBJS)
	$(ARMCC) $(ARMCFLAGS) -Wl,--gc-sections -T ./stm/STM32F030K6_FLASH.ld $(OBJS) stm/startup_stm32f030.out -o out.elf

.PHONY: clean
clean:
	rm -f bitmaps/bitmaps.c bitmaps/bitmaps.h
	rm -f menus/menus_strings_table.c menus/menus_strings_table.h
	rm -f *.d bitmaps/*.d menus/*.d
	rm -f *.out bitmaps/*.out menus/*.out
	rm -f *.o bitmaps/*.o menus/*.o

GCC := gcc
GCCFLAGS := -I./

%.o: %.c
	$(GCC) -c $(GCCFLAGS) $*.c -o $*.o
	$(GCC) -MM $(GCCFLAGS) $*.c > $*.d

bcdtest: GCCFLAGS := $(GCCFLAGS) -DTEST
bcdtest: bcd.o mymemset.o
	$(GCC) $(GCCFLAGS) mymemset.o bcd.o -o bcdtest

exposuretest: GCCFLAGS := $(GCCFLAGS) -DTEST
exposuretest: exposuretest_bcd exposure.o tables.o mymemset.o
	$(GCC) $(GCCFLAGS) bcd.o exposure.o tables.o mymemset.o -o testexposure

# Required so that we don't compile bcd with -DTEST when building exposure with -DTEST.
exposuretest_bcd: GCCFLAGS:= $(GCCFLAGS)
exposuretest_bcd: bcd.o
