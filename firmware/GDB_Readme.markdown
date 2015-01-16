A record of some easy-to-forget-and-hard-to-google commands.

Basic stop-flash-start sequence:

    monitor reset halt
    load ~/progs/lightmeter/firmware/out.elf
    continue

Enable semihosting:

    arm semihosting enable

Debug messages appear on OpenOCD stdout (not in GDB console).

