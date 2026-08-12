#include "multiboot2.h"

#define MB2_TAG_MODULE 3u
#define MB2_MODULE_PREFIX_SIZE 16u
#define MB2_ROOTFS_CMDLINE "mantle-rootfs"
#define MB2_ROOTFS_CMDLINE_LENGTH 13u

int mantle_mb2_parse_module(const void *raw_tag, uint32_t available_size,
    struct mantle_mb2_module *module)
{
    const struct mb2_tag_module *tag = (const struct mb2_tag_module *)raw_tag;
    uint32_t cmdline_size;
    uint32_t index;

    if (tag == (const struct mb2_tag_module *)0 ||
        module == (struct mantle_mb2_module *)0 ||
        available_size < MB2_MODULE_PREFIX_SIZE || tag->size < MB2_MODULE_PREFIX_SIZE ||
        tag->size > available_size || tag->type != MB2_TAG_MODULE ||
        tag->mod_end <= tag->mod_start) {
        return -1;
    }
    cmdline_size = tag->size - MB2_MODULE_PREFIX_SIZE;
    if (cmdline_size != MB2_ROOTFS_CMDLINE_LENGTH + 1u) {
        return -1;
    }
    for (index = 0u; index < MB2_ROOTFS_CMDLINE_LENGTH; ++index) {
        if (tag->cmdline[index] != MB2_ROOTFS_CMDLINE[index]) {
            return -1;
        }
    }
    if (tag->cmdline[MB2_ROOTFS_CMDLINE_LENGTH] != '\0') {
        return -1;
    }
    module->start = (uintptr_t)tag->mod_start;
    module->end = (uintptr_t)tag->mod_end;
    module->size = tag->mod_end - tag->mod_start;
    return 0;
}
