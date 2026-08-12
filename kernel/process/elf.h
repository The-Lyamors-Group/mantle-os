#ifndef MANTLE_KERNEL_ELF_H
#define MANTLE_KERNEL_ELF_H

#include <mantle/types.h>

struct mantle_user_image {
    uintptr_t entry;
    uintptr_t stack;
};

int mantle_elf_load(const uint8_t *image, uint32_t size, struct mantle_user_image *result);

#endif
