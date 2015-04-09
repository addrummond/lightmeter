A record of some easy-to-forget-and-hard-to-google commands.

Start up OpenOCD for STM32F0 discovery board:

    openocd -f /usr/local/share/openocd/scripts/board/stm32f0discovery.cfg

Or for prototype board:

    openocd -f /usr/local/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/local/share/openocd/scripts/target/stm32f0x_stlink.cfg

Initialize GDB connection to OpenOCD (from GDB console):

    target remote localhost:3333

Enable semihosting (from GDB console):

    monitor arm semihosting enable

Basic stop-flash-start sequence (from GDB console):

    monitor reset halt
    load ~/progs/lightmeter/firmware/out.elf
    continue

Debug messages appear on OpenOCD stdout (not in GDB console).
