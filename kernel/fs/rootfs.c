#include "rootfs.h"

#define MANTLE_ROOTFS_MAGIC 0x31534652u
#define MANTLE_ROOTFS_MAX_ENTRIES 128u

struct rootfs_header {
    uint32_t magic;
    uint32_t entries;
};

struct rootfs_entry {
    uint16_t path_length;
    uint16_t mode;
    uint32_t offset;
    uint32_t size;
};

struct rootfs_state {
    const uint8_t *image;
    uint32_t size;
    uint32_t entries;
};

static struct rootfs_state state;

static int range_valid(uint32_t offset, uint32_t length, uint32_t size)
{
    return offset <= size && length <= size - offset;
}

static int path_equal(const uint8_t *left, uint16_t left_length, const char *right)
{
    uint16_t index = 0u;

    while (right[index] != '\0') {
        if (index >= left_length || left[index] != (uint8_t)right[index]) {
            return 0;
        }
        ++index;
    }
    return index == left_length;
}

static int name_seen(const char *output, uint32_t length, const uint8_t *name, uint16_t name_length)
{
    uint32_t cursor = 0u;
    while (cursor < length) {
        uint32_t start = cursor;
        uint16_t index = 0u;
        while (cursor < length && output[cursor] != ' ' && output[cursor] != '\n') {
            ++cursor;
        }
        if (cursor - start == (uint32_t)name_length) {
            while (index < name_length && output[start + index] == (char)name[index]) {
                ++index;
            }
            if (index == name_length) {
                return 1;
            }
        }
        ++cursor;
    }
    return 0;
}

int mantle_rootfs_mount(const uint8_t *image, uint32_t size)
{
    const struct rootfs_header *header;
    uint32_t cursor;
    uint32_t index;

    state.image = (const uint8_t *)0;
    state.size = 0u;
    state.entries = 0u;
    if (image == (const uint8_t *)0 || size < sizeof(struct rootfs_header)) {
        return -1;
    }
    header = (const struct rootfs_header *)image;
    if (header->magic != MANTLE_ROOTFS_MAGIC || header->entries == 0u ||
        header->entries > MANTLE_ROOTFS_MAX_ENTRIES) {
        return -1;
    }
    cursor = (uint32_t)sizeof(struct rootfs_header);
    for (index = 0u; index < header->entries; ++index) {
        const struct rootfs_entry *entry;
        if (!range_valid(cursor, (uint32_t)sizeof(struct rootfs_entry), size)) {
            return -1;
        }
        entry = (const struct rootfs_entry *)(image + cursor);
        cursor += (uint32_t)sizeof(struct rootfs_entry);
        if (entry->path_length == 0u || entry->path_length > 255u ||
            !range_valid(cursor, entry->path_length, size) ||
            !range_valid(entry->offset, entry->size, size)) {
            return -1;
        }
        cursor += entry->path_length;
    }
    state.image = image;
    state.size = size;
    state.entries = header->entries;
    return 0;
}

int mantle_rootfs_open(const char *path, struct mantle_rootfs_file *file)
{
    uint32_t cursor;
    uint32_t index;

    if (!mantle_rootfs_ready() || path == (const char *)0 || file == (struct mantle_rootfs_file *)0) {
        return -1;
    }
    cursor = (uint32_t)sizeof(struct rootfs_header);
    for (index = 0u; index < state.entries; ++index) {
        const struct rootfs_entry *entry = (const struct rootfs_entry *)(state.image + cursor);
        const uint8_t *name;
        cursor += (uint32_t)sizeof(struct rootfs_entry);
        name = state.image + cursor;
        if (path_equal(name, entry->path_length, path)) {
            file->data = state.image + entry->offset;
            file->size = entry->size;
            file->mode = entry->mode;
            return 0;
        }
        cursor += entry->path_length;
    }
    return -1;
}

int mantle_rootfs_list(const char *path, char *output, uint32_t output_size)
{
    uint32_t cursor;
    uint32_t index;
    uint32_t written = 0u;

    if (!mantle_rootfs_ready() || path == (const char *)0 || output == (char *)0 ||
        output_size == 0u || path[0] != '/' || path[1] != '\0') {
        return -1;
    }
    cursor = (uint32_t)sizeof(struct rootfs_header);
    for (index = 0u; index < state.entries; ++index) {
        const struct rootfs_entry *entry = (const struct rootfs_entry *)(state.image + cursor);
        const uint8_t *name;
        uint16_t name_length = 0u;
        cursor += (uint32_t)sizeof(struct rootfs_entry);
        name = state.image + cursor + 1u;
        if (entry->path_length > 1u) {
            while (name_length + 1u < entry->path_length && name[name_length] != '/' &&
                name[name_length] != '\0') {
                ++name_length;
            }
            if (!name_seen(output, written, name, name_length)) {
                if (written + (uint32_t)name_length + 1u >= output_size) {
                    return -1;
                }
                for (uint16_t part = 0u; part < name_length; ++part) {
                    output[written++] = (char)name[part];
                }
                output[written++] = ' ';
            }
        }
        cursor += entry->path_length;
    }
    if (written == 0u || written + 1u >= output_size) {
        return -1;
    }
    output[written - 1u] = '\n';
    output[written] = '\0';
    return (int)written;
}

int mantle_rootfs_ready(void)
{
    return state.image != (const uint8_t *)0;
}
