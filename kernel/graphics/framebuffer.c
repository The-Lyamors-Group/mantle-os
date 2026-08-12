#include "framebuffer.h"

#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36d76289u
#define MULTIBOOT_TAG_TYPE_END 0u
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER 8u
#define MULTIBOOT_FRAMEBUFFER_TYPE_DIRECT_RGB 1u

struct multiboot_tag {
    uint32_t type;
    uint32_t size;
};

struct multiboot_tag_framebuffer_common {
    uint32_t type;
    uint32_t size;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint16_t reserved;
};

struct framebuffer_state {
    volatile uint8_t *address;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint8_t bytes_per_pixel;
    uint8_t red_position;
    uint8_t green_position;
    uint8_t blue_position;
};

static struct framebuffer_state state;

static uint32_t color(uint8_t red, uint8_t green, uint8_t blue)
{
    return ((uint32_t)red << state.red_position) |
           ((uint32_t)green << state.green_position) |
           ((uint32_t)blue << state.blue_position);
}

static void pixel(uint32_t x, uint32_t y, uint32_t value)
{
    volatile uint8_t *destination;
    uint8_t byte;

    if (x >= state.width || y >= state.height) {
        return;
    }
    destination = state.address + ((uintptr_t)y * state.pitch) + ((uintptr_t)x * state.bytes_per_pixel);
    for (byte = 0u; byte < state.bytes_per_pixel; ++byte) {
        destination[byte] = (uint8_t)(value >> (byte * 8u));
    }
}

static void rectangle(uint32_t left, uint32_t top, uint32_t width, uint32_t height, uint32_t value)
{
    uint32_t y;
    uint32_t x;

    for (y = top; y < top + height && y < state.height; ++y) {
        for (x = left; x < left + width && x < state.width; ++x) {
            pixel(x, y, value);
        }
    }
}

static void line(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t value)
{
    int32_t delta_y;
    int32_t delta_x;
    int32_t current_y;
    int32_t end_y;
    int32_t step_y;
    uint32_t x;
    uint32_t y;

    if (x0 == x1) {
        current_y = (int32_t)y0;
        end_y = (int32_t)y1;
        step_y = current_y <= end_y ? 1 : -1;
        for (;;) {
            pixel(x0, (uint32_t)current_y, value);
            if (current_y == end_y) {
                break;
            }
            current_y += step_y;
        }
        return;
    }
    if (x0 > x1) {
        return;
    }
    delta_x = (int32_t)x1 - (int32_t)x0;
    delta_y = (int32_t)y1 - (int32_t)y0;
    for (x = x0; x <= x1; ++x) {
        y = (uint32_t)((int32_t)y0 + (delta_y * ((int32_t)x - (int32_t)x0)) / delta_x);
        pixel(x, y, value);
    }
}

static void draw_mantle_splash(void)
{
    uint32_t background = color(12u, 19u, 29u);
    uint32_t panel = color(24u, 36u, 49u);
    uint32_t accent = color(108u, 174u, 255u);
    uint32_t white = color(238u, 244u, 249u);
    uint32_t mark_width = state.width > 640u ? 180u : 96u;
    uint32_t mark_height = state.height > 480u ? 136u : 80u;
    uint32_t left = (state.width - mark_width) / 2u;
    uint32_t top = (state.height - mark_height) / 2u;
    uint32_t bar_top = state.height > 64u ? state.height - 48u : 0u;

    rectangle(0u, 0u, state.width, state.height, background);
    rectangle(0u, 0u, state.width, 2u, accent);
    rectangle(0u, bar_top, state.width, 1u, panel);

    /* MantleOS mark: an M built from the same open geometry as the SVG asset. */
    line(left, top + mark_height, left, top, white);
    line(left, top, left + mark_width / 2u, top + mark_height / 2u, white);
    line(left + mark_width / 2u, top + mark_height / 2u, left + mark_width, top, white);
    line(left + mark_width, top, left + mark_width, top + mark_height, white);
    line(left + 10u, top + mark_height + 12u, left + mark_width / 2u, top + mark_height / 2u + 12u, accent);
    line(left + mark_width / 2u, top + mark_height / 2u + 12u, left + mark_width - 10u, top + mark_height + 12u, accent);
    rectangle(left + mark_width / 2u - 38u, bar_top - 13u, 76u, 3u, panel);
    rectangle(left + mark_width / 2u - 38u, bar_top - 13u, 25u, 3u, accent);
}

int mantle_framebuffer_init(uint32_t multiboot_magic, uintptr_t multiboot_info)
{
    struct multiboot_tag *tag;
    struct multiboot_tag_framebuffer_common *framebuffer;
    const uint8_t *rgb_fields;
    uintptr_t cursor;

    if (multiboot_magic != MULTIBOOT2_BOOTLOADER_MAGIC || multiboot_info == 0u) {
        return -1;
    }

    cursor = multiboot_info + 8u;
    for (;;) {
        tag = (struct multiboot_tag *)cursor;
        if (tag->type == MULTIBOOT_TAG_TYPE_END) {
            return -1;
        }
        if (tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER && tag->size >= 38u) {
            framebuffer = (struct multiboot_tag_framebuffer_common *)tag;
            rgb_fields = (const uint8_t *)tag + 32u;
            if (framebuffer->framebuffer_type != MULTIBOOT_FRAMEBUFFER_TYPE_DIRECT_RGB ||
                framebuffer->framebuffer_addr == 0u || framebuffer->framebuffer_bpp < 24u ||
                framebuffer->framebuffer_bpp > 32u) {
                return -1;
            }
            state.address = (volatile uint8_t *)(uintptr_t)framebuffer->framebuffer_addr;
            state.pitch = framebuffer->framebuffer_pitch;
            state.width = framebuffer->framebuffer_width;
            state.height = framebuffer->framebuffer_height;
            state.bytes_per_pixel = (uint8_t)((framebuffer->framebuffer_bpp + 7u) / 8u);
            state.red_position = rgb_fields[0];
            state.green_position = rgb_fields[2];
            state.blue_position = rgb_fields[4];
            if (state.width == 0u || state.height == 0u || state.pitch == 0u) {
                return -1;
            }
            draw_mantle_splash();
            return 0;
        }
        cursor += (tag->size + 7u) & ~7u;
    }
}
