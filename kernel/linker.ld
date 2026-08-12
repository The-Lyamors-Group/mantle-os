OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS
{
    . = 1M;
    .multiboot ALIGN(8) : { KEEP(*(.multiboot)) }
    .text ALIGN(16) : { *(.text .text.*) }
    .rodata ALIGN(16) : { *(.rodata .rodata.*) }
    .data ALIGN(4096) : { *(.data .data.*) }
    .bss ALIGN(16) : { *(COMMON) *(.bss .bss.*) }
    /DISCARD/ : { *(.eh_frame) *(.comment) *(.note.GNU-stack) }
}
