/* What hasm lays out, for the oracle: ld has to put things where hasm does, or
   their bytes could not be compared. See Image::lay_out. */
ENTRY(_start)

SECTIONS
{
    . = 0x401000;
    .text : { *(.text) }
    . = ALIGN(0x1000);
    .rodata : { *(.rodata) }
    . = ALIGN(0x1000);
    .data : { *(.data) }
    . = ALIGN(0x1000);
    .bss : { *(.bss) }
    /DISCARD/ : { *(.note*) *(.comment) }
}
