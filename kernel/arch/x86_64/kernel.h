#ifndef MANTLE_KERNEL_ARCH_H
#define MANTLE_KERNEL_ARCH_H

#include <mantle/types.h>

struct mantle_iret_frame {
    uint64_t ss;
    uint64_t rsp;
    uint64_t rflags;
    uint64_t cs;
    uint64_t rip;
};

_Static_assert(sizeof(struct mantle_iret_frame) == 40u, "iret frame size");
_Static_assert(__builtin_offsetof(struct mantle_iret_frame, ss) == 0u, "iret SS offset");
_Static_assert(__builtin_offsetof(struct mantle_iret_frame, rsp) == 8u, "iret RSP offset");
_Static_assert(__builtin_offsetof(struct mantle_iret_frame, rflags) == 16u, "iret RFLAGS offset");
_Static_assert(__builtin_offsetof(struct mantle_iret_frame, cs) == 24u, "iret CS offset");
_Static_assert(__builtin_offsetof(struct mantle_iret_frame, rip) == 32u, "iret RIP offset");

void mantle_console_write(const char *text);
void mantle_arch_user_memory_init(void);
void mantle_arch_syscall_init(void);
void mantle_arch_exception_init(void);
void mantle_enter_user(uintptr_t entry, uintptr_t stack);

#endif
