#ifndef MANTLE_KERNEL_ARCH_H
#define MANTLE_KERNEL_ARCH_H

#include <mantle/types.h>

void mantle_console_write(const char *text);
void mantle_arch_user_memory_init(void);
void mantle_arch_syscall_init(void);
void mantle_arch_exception_init(void);
void mantle_enter_user(uintptr_t entry, uintptr_t stack);

#endif
