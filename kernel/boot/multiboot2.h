#ifndef MANTLE_MULTIBOOT2_H
#define MANTLE_MULTIBOOT2_H

#include <mantle/types.h>

struct mb2_tag_module {
    uint32_t type;
    uint32_t size;
    uint32_t mod_start;
    uint32_t mod_end;
    char cmdline[];
};

struct mantle_mb2_module {
    uintptr_t start;
    uintptr_t end;
    uint32_t size;
};

_Static_assert(__builtin_offsetof(struct mb2_tag_module, type) == 0u, "MB2 type offset");
_Static_assert(__builtin_offsetof(struct mb2_tag_module, size) == 4u, "MB2 size offset");
_Static_assert(__builtin_offsetof(struct mb2_tag_module, mod_start) == 8u, "MB2 start offset");
_Static_assert(__builtin_offsetof(struct mb2_tag_module, mod_end) == 12u, "MB2 end offset");
_Static_assert(__builtin_offsetof(struct mb2_tag_module, cmdline) == 16u, "MB2 cmdline offset");

int mantle_mb2_parse_module(const void *raw_tag, uint32_t available_size,
    struct mantle_mb2_module *module);

#endif
