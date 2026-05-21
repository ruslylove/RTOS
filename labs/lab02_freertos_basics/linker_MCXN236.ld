/* Linker script for FRDM-MCXN236 — FreeRTOS lab.
 * MCXN236: 1 MB flash @ 0x00000000, 256 KB SRAM @ 0x20000000.
 */
ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 0x00100000
    RAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 0x00040000
}

SECTIONS
{
    .isr_vector :
    {
        KEEP(*(.isr_vector))
    } > FLASH

    .text :
    {
        *(.text*)
        *(.rodata*)
        _etext = .;
    } > FLASH

    .data :
    {
        _sidata = LOADADDR(.data);
        _sdata = .;
        *(.data*)
        _edata = .;
    } > RAM AT > FLASH

    .bss :
    {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
    } > RAM

    .heap :
    {
        . = ALIGN(8);
        _heap_start = .;
        . = . + 0x2000;         /* 8 KB heap (not used by FreeRTOS heap_4) */
        _heap_end = .;
    } > RAM

    .stack :
    {
        . = ALIGN(8);
        _stack_end = .;
        . = . + 0x1000;         /* 4 KB MSP stack for interrupts and boot */
        _stack_top = .;
    } > RAM
}
