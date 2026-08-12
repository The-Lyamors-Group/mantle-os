#include <mantle/types.h>
#include "../kernel/boot/multiboot2.h"

static int expect_failure(const void *tag, uint32_t size)
{
    struct mantle_mb2_module module;
    return mantle_mb2_parse_module(tag, size, &module) != 0;
}

int main(void)
{
    uint8_t raw[30] = {0};
    struct mb2_tag_module *tag = (struct mb2_tag_module *)raw;
    struct mantle_mb2_module module;

    tag->type = 3u;
    tag->size = 30u;
    tag->mod_start = 0x4000u;
    tag->mod_end = 0xd240u;
    raw[16] = 'm'; raw[17] = 'a'; raw[18] = 'n'; raw[19] = 't';
    raw[20] = 'l'; raw[21] = 'e'; raw[22] = '-'; raw[23] = 'r';
    raw[24] = 'o'; raw[25] = 'o'; raw[26] = 't'; raw[27] = 'f';
    raw[28] = 's'; raw[29] = '\0';
    if (mantle_mb2_parse_module(raw, sizeof(raw), &module) != 0 ||
        module.start != (uintptr_t)0x4000u || module.end != (uintptr_t)0xd240u ||
        module.size != 37440u) {
        return 1;
    }
    raw[29] = 'x';
    if (!expect_failure(raw, sizeof(raw))) return 2;
    raw[29] = '\0';
    tag->size = 29u;
    if (!expect_failure(raw, sizeof(raw))) return 3;
    tag->size = 15u;
    if (!expect_failure(raw, sizeof(raw))) return 4;
    tag->size = 30u;
    if (!expect_failure(raw, 29u)) return 5;
    return 0;
}
