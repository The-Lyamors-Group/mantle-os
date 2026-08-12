.section .text
.globl _start
.type _start, @function
.extern main
.extern mantle_exit
_start:
    xor %ebp, %ebp
    call main
    mov %rax, %rdi
    call mantle_exit
1:
    hlt
    jmp 1b

.section .note.GNU-stack,"",@progbits
