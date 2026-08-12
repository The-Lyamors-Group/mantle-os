.section .multiboot
.align 8
.globl multiboot_header_start
multiboot_header_start:
    .long 0xe85250d6
    .long 0
    .long multiboot_header_end - multiboot_header_start
    .long -(0xe85250d6 + (multiboot_header_end - multiboot_header_start))
    .short 0
    .short 0
    .long 8
multiboot_header_end:

.section .text
.code32
.globl _start
.type _start, @function
.extern mantle_kernel_main
_start:
    cli
    mov $stack_top, %esp
    mov $pml4_table, %eax
    mov %eax, %cr3
    mov %cr4, %eax
    or $0x20, %eax
    mov %eax, %cr4
    mov $0xc0000080, %ecx
    rdmsr
    or $0x100, %eax
    wrmsr
    mov %cr0, %eax
    or $0x80000000, %eax
    mov %eax, %cr0
    lgdt gdt_descriptor
    ljmp $0x08, $long_mode_entry

.code64
long_mode_entry:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov $stack_top, %rsp
    cld
    call mantle_kernel_main

.halt:
    hlt
    jmp .halt

.section .rodata
.align 8
gdt:
    .quad 0x0000000000000000
    .quad 0x00af9a000000ffff
    .quad 0x00cf92000000ffff
gdt_descriptor:
    .word . - gdt - 1
    .long gdt

.section .data
.align 4096
pml4_table:
    .quad pdpt_table + 0x003
    .skip 4088
pdpt_table:
    .quad pd_table + 0x003
    .skip 4088
pd_table:
    .quad 0x0000000000000083
    .skip 4088

.section .bss
.align 16
stack_bottom:
    .skip 16384
stack_top:
