.section .text
.globl mantle_read
.type mantle_read, @function
mantle_read:
    mov $0, %rax
    syscall
    ret
.globl mantle_write
.type mantle_write, @function
mantle_write:
    mov $1, %rax
    syscall
    ret
.globl mantle_exec
.type mantle_exec, @function
mantle_exec:
    mov $6, %rax
    syscall
    ret
.globl mantle_chdir
.type mantle_chdir, @function
mantle_chdir:
    mov $9, %rax
    syscall
    ret
.globl mantle_getcwd
.type mantle_getcwd, @function
mantle_getcwd:
    mov $10, %rax
    syscall
    ret
.globl mantle_getpid
.type mantle_getpid, @function
mantle_getpid:
    mov $8, %rax
    syscall
    ret
.globl mantle_readdir
.type mantle_readdir, @function
mantle_readdir:
    mov $13, %rax
    syscall
    ret
.globl mantle_uname
.type mantle_uname, @function
mantle_uname:
    mov $14, %rax
    syscall
    ret
.globl mantle_exit
.type mantle_exit, @function
mantle_exit:
    mov $11, %rax
    syscall
    ret

.section .note.GNU-stack,"",@progbits
