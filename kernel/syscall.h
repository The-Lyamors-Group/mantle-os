#ifndef MANTLE_KERNEL_SYSCALL_H
#define MANTLE_KERNEL_SYSCALL_H

#include <mantle/types.h>

struct mantle_syscall_frame {
    uint64_t rax;
    uint64_t rip;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t r10;
    uint64_t r8;
    uint64_t r9;
};

void mantle_syscall_dispatch(struct mantle_syscall_frame *frame);

#endif
