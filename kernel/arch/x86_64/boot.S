.section .multiboot,"a",@progbits
.align 8
.globl multiboot_header_start
multiboot_header_start:
    .long 0xe85250d6
    .long 0
    .long multiboot_header_end - multiboot_header_start
    .long -(0xe85250d6 + (multiboot_header_end - multiboot_header_start))
    .short 5
    .short 0
    .long 20
    .long 0
    .long 0
    .long 0
    .align 8
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
    mov %eax, boot_magic
    mov %ebx, boot_info
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
    mov $stack_top, %rax
    mov %rax, tss_rsp0(%rip)
    lea tss(%rip), %rax
    lea tss_descriptor(%rip), %rdi
    movw $0x0067, 0(%rdi)
    mov %ax, 2(%rdi)
    mov %rax, %rdx
    shr $16, %rdx
    mov %dl, 4(%rdi)
    movb $0x89, 5(%rdi)
    shr $8, %rdx
    mov %dl, 6(%rdi)
    mov %rax, %rdx
    shr $32, %rdx
    mov %dx, 8(%rdi)
    shr $16, %rdx
    mov %dx, 10(%rdi)
    mov $0x28, %ax
    ltr %ax
    mov boot_magic(%rip), %edi
    mov boot_info(%rip), %esi
    cld
    call mantle_kernel_main

.halt:
    hlt
    jmp .halt

.section .data
.align 8
gdt:
    .quad 0x0000000000000000
    .quad 0x00af9a000000ffff
    .quad 0x00cf92000000ffff
    .quad 0x00cff2000000ffff
    .quad 0x00affa000000ffff
tss_descriptor:
    .skip 16
gdt_descriptor:
    .word . - gdt - 1
    .long gdt

.align 4096
pml4_table:
    .quad pdpt_table + 0x003
    .skip 4088
pdpt_table:
    .quad pd_table0 + 0x003
    .quad pd_table1 + 0x003
    .quad pd_table2 + 0x003
    .quad pd_table3 + 0x003
    .skip 4064
pd_table0:
    .set page, 0
    .rept 2
    .quad (page * 0x200000) + 0x083
    .set page, page + 1
    .endr
    .quad user_page_table + 0x003
    .set page, 3
    .rept 509
    .quad (page * 0x200000) + 0x083
    .set page, page + 1
    .endr
pd_table1:
    .set page, 512
    .rept 512
    .quad (page * 0x200000) + 0x083
    .set page, page + 1
    .endr
pd_table2:
    .set page, 1024
    .rept 512
    .quad (page * 0x200000) + 0x083
    .set page, page + 1
    .endr
pd_table3:
    .set page, 1536
    .rept 512
    .quad (page * 0x200000) + 0x083
    .set page, page + 1
    .endr

.section .bss
.align 16
boot_magic:
    .long 0
boot_info:
    .long 0
.align 16
stack_bottom:
    .skip 16384
stack_top:

.align 4096
.globl user_page_table
user_page_table:
    .skip 4096

.align 16
tss:
    .skip 4
tss_rsp0:
    .quad 0
    .skip 96

.section .text
.code64
.globl mantle_enter_user
.type mantle_enter_user, @function
mantle_enter_user:
    cli
    mov $0x18, %ax
    mov %ax, %ds
    mov %ax, %es
    mov $0x18, %rax
    push %rax
    push %rsi
    mov $0x2, %rax
    push %rax
    mov $0x20, %rax
    push %rax
    push %rdi
    iretq

.globl mantle_syscall_entry
.type mantle_syscall_entry, @function
.extern mantle_syscall_dispatch
mantle_syscall_entry:
    cld
    mov %r11, syscall_saved_rflags(%rip)
    mov %rsp, syscall_saved_user_rsp(%rip)
    mov $syscall_stack_top, %rsp
    sub $128, %rsp
    mov %rax, 0(%rsp)
    mov %rcx, 8(%rsp)
    mov syscall_saved_rflags(%rip), %r11
    mov %r11, 16(%rsp)
    mov syscall_saved_user_rsp(%rip), %r11
    mov %r11, 24(%rsp)
    mov %rdi, 32(%rsp)
    mov %rsi, 40(%rsp)
    mov %rdx, 48(%rsp)
    mov %r10, 56(%rsp)
    mov %r8, 64(%rsp)
    mov %r9, 72(%rsp)
    mov %rsp, %rdi
    call mantle_syscall_dispatch
    mov 0(%rsp), %rax
    mov 8(%rsp), %rcx
    mov 16(%rsp), %r11
    mov 24(%rsp), %rsp
    sysretq

.section .bss
.align 16
syscall_stack_bottom:
    .skip 16384
syscall_stack_top:

.align 8
syscall_saved_rflags:
    .quad 0
syscall_saved_user_rsp:
    .quad 0

.section .note.GNU-stack,"",@progbits
