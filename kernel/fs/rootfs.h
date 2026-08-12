#ifndef MANTLE_KERNEL_ROOTFS_H
#define MANTLE_KERNEL_ROOTFS_H

#include <mantle/types.h>

struct mantle_rootfs_file {
    const uint8_t *data;
    uint32_t size;
    uint16_t mode;
};

enum mantle_rootfs_status {
    MANTLE_ROOTFS_VALID = 0,
    MANTLE_ROOTFS_INVALID = -1
};

int mantle_rootfs_mount(const uint8_t *image, uint32_t size);
int mantle_rootfs_open(const char *path, struct mantle_rootfs_file *file);
int mantle_rootfs_list(const char *path, char *output, uint32_t output_size);
int mantle_rootfs_ready(void);

#endif
