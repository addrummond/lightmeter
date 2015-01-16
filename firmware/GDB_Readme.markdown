A record of some easy-to-forget-and-hard-to-google commands.

Initialize GDB connection to OpenOCD:

    target remote localhost:3333

Enable semihosting:

    monitor arm semihosting enable

Basic stop-flash-start sequence:

    monitor reset halt
    load ~/progs/lightmeter/firmware/out.elf
    continue

Enable semihosting:

    arm semihosting enable

Debug messages appear on OpenOCD stdout (not in GDB console).
