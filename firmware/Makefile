CC=arm-none-eabi-gcc
CFLAGS=-g -Wall -Os -mcpu=cortex-m0 -ffunction-sections -fdata-sections -nostdlib -mthumb -DSTM32F030 -DUSE_FULL_ASSERT -I ./ -I ./stm -Wall
OBJS=accel.o bcd.o buttons.o debugging.o display.o exposure.o goetzel.o \
     hamming.o hfsdp.o i2c.o main.o meter.o myassert.o mymemset.o piezo.o \
		 state.o sysinit.o ui.o menus/menu_strings.o menus/menu_strings_table.o \
		 bitmaps/bitmaps.o tables.o \
		 $(patsubst %.c,%.o,$(shell echo stm/*.c))

bitmaps/bitmaps.h bitmaps/bitmaps.c: $(shell find ./bitmaps -name '*.png') bitmaps/process_bitmaps.py
	cd bitmaps && python3 process_bitmaps.py output
bitmaps/bitmaps.c: bitmaps/bitmaps.h

menus/menu_strings_table.h menus/menus_strings_table.c: menus/strings_english menus/process_strings.py
	cd menus && python3 process_strings.py strings_english
menus/menus_strings_table.c: menus/menu_strings_table.h

tables.h tables.c: calculate_tables.py
	python3 calculate_tables.py output
tables.c: tables.h

stm/startup_stm32f030.o: stm/startup_stm32f030.s
	$(CC) $(CFLAGS) -c stm/startup_stm32f030.s -o stm/startup_stm32f030.o

.PHONY: prereq
prereq: bitmaps/bitmaps.c menus/menus_strings_table.c tables.c stm/startup_stm32f030.o

-include $(OBJS:.o=.d)

%.o: %.c
	$(CC) -c $(CFLAGS) $*.c -o $*.o
	$(CC) -MM $(CFLAGS) $*.c > $*.d

/stm/STM32F030K6_FLASH.ld: /stm/STM32F030K6_FLASH.ld

out.elf: prereq /stm/STM32F030K6_FLASH.ld $(OBJS) stm/startup_stm32f030.o
	$(CC) $(CFLAGS) -Wl,--gc-sections -T ./stm/STM32F030K6_FLASH.ld $(OBJS) stm/startup_stm32f030.o -o out.elf

out.bin: out.elf
	arm-none-eabi-objcopy -O binary out.elf out.bin

.PHONY: clean
clean:
	rm -f bitmaps/bitmaps.c bitmaps/bitmaps.h; \
	rm -f menus/menus_strings_table.c menus/menus_strings_table.h \
  rm -f *.d bitmaps/*.d menus/*.d
	rm -f *.o bitmaps/*.o menus/*.o
