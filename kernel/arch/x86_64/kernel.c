#include <mantle/types.h>
#include "../../graphics/framebuffer.h"
#include "../../fs/rootfs.h"
#include "../../process/elf.h"
#include "../../process/process.h"
#include "../../boot/multiboot2.h"
#include "kernel.h"

#define COM1 0x3f8u
#define VGA_MEMORY 0xb8000u

static uint16_t vga_cursor;

uint8_t mantle_user_memory[0x200000u] __attribute__((aligned(4096)));
extern uint64_t user_page_table[];
extern void mantle_syscall_entry(void);

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile ("outb %0, %1" : : "a" (value), "Nd" (port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a" (value) : "Nd" (port));
    return value;
}

static void serial_init(void)
{
    outb(COM1 + 1u, 0u);
    outb(COM1 + 3u, 0x80u);
    outb(COM1 + 0u, 1u);
    outb(COM1 + 1u, 0u);
    outb(COM1 + 3u, 3u);
    outb(COM1 + 2u, 0xc7u);
    outb(COM1 + 4u, 0x0bu);
}

static void serial_putc(char value)
{
    while ((inb(COM1 + 5u) & 0x20u) == 0u) {
    }
    outb(COM1, (uint8_t)value);
}

static void vga_putc(char value)
{
    volatile uint16_t *video = (volatile uint16_t *)(uintptr_t)VGA_MEMORY;
    if (value == '\n') {
        vga_cursor = (uint16_t)(((vga_cursor / 80u) + 1u) * 80u);
        return;
    }
    if (vga_cursor >= 80u * 25u) {
        vga_cursor = 0u;
    }
    video[vga_cursor++] = (uint16_t)(0x0700u | (uint8_t)value);
}

void mantle_console_write(const char *text)
{
    while (*text != '\0') {
        serial_putc(*text);
        vga_putc(*text);
        ++text;
    }
}

static inline void write_msr(uint32_t msr, uint64_t value)
{
    uint32_t low = (uint32_t)value;
    uint32_t high = (uint32_t)(value >> 32u);
    __asm__ volatile ("wrmsr" : : "c" (msr), "a" (low), "d" (high));
}

void mantle_arch_user_memory_init(void)
{
    uint32_t index;
    for (index = 0u; index < 512u; ++index) {
        user_page_table[index] = ((uint64_t)(uintptr_t)mantle_user_memory +
            ((uint64_t)index * 4096u)) | 0x8000000000000005ull;
    }
}

void mantle_arch_syscall_init(void)
{
    write_msr(0xc0000081u, (0x10ull << 48u) | (0x08ull << 32u));
    write_msr(0xc0000082u, (uint64_t)(uintptr_t)mantle_syscall_entry);
    write_msr(0xc0000084u, 0x200ull);
}

struct multiboot_tag {
    uint32_t type;
    uint32_t size;
};

struct multiboot_info_header {
    uint32_t total_size;
    uint32_t reserved;
};

static void serial_hex(uint64_t value)
{
    static const char digits[] = "0123456789abcdef";
    int shift;
    serial_putc('0');
    serial_putc('x');
    for (shift = 60; shift >= 0; shift -= 4) {
        serial_putc(digits[(value >> (uint32_t)shift) & 0xfu]);
    }
}

static void serial_decimal(uint32_t value)
{
    char digits[10];
    uint32_t count = 0u;
    if (value == 0u) {
        serial_putc('0');
        return;
    }
    while (value != 0u && count < sizeof(digits)) {
        digits[count++] = (char)('0' + (value % 10u));
        value /= 10u;
    }
    while (count != 0u) {
        serial_putc(digits[--count]);
    }
}

static void serial_tag(uint32_t type, uint32_t size)
{
    mantle_console_write("[mb2] tag type=");
    serial_decimal(type);
    mantle_console_write(" size=");
    serial_decimal(size);
    mantle_console_write("\n");
}

static void serial_byte(uint8_t value)
{
    static const char digits[] = "0123456789abcdef";
    serial_putc(digits[(value >> 4u) & 0xfu]);
    serial_putc(digits[value & 0xfu]);
}

static int find_rootfs(uintptr_t multiboot_info, const uint8_t **image, uint32_t *size)
{
    const struct multiboot_tag *tag;
    const struct multiboot_info_header *header;
    uintptr_t cursor;
    uintptr_t end;

    if (multiboot_info == 0u || image == (const uint8_t **)0 || size == (uint32_t *)0) {
        return -1;
    }
    header = (const struct multiboot_info_header *)multiboot_info;
    if (header->total_size < 8u || header->total_size > 0x100000u) {
        return -2;
    }
    cursor = multiboot_info + 8u;
    end = multiboot_info + header->total_size;

    while (cursor + sizeof(struct multiboot_tag) <= end) {
        tag = (const struct multiboot_tag *)cursor;
        serial_tag(tag->type, tag->size);
        if (tag->size < 8u || cursor + tag->size > end ||
            cursor + ((tag->size + 7u) & ~7u) < cursor) {
            return -2;
        }
        if (tag->type == 0u) {
            break;
        }
        if (tag->type == 3u && tag->size >= 16u) {
            const struct mb2_tag_module *module = (const struct mb2_tag_module *)tag;
            struct mantle_mb2_module parsed;
            uint32_t payload_size = tag->size - 16u;
            uint32_t index;
            mantle_console_write("[mb2] module start=");
            serial_hex(module->mod_start);
            mantle_console_write(" end=");
            serial_hex(module->mod_end);
            mantle_console_write(" size=");
            serial_decimal(module->mod_end > module->mod_start ? module->mod_end - module->mod_start : 0u);
            mantle_console_write(" cmdline=");
            for (index = 0u; index < payload_size; ++index) {
                if (module->cmdline[index] == '\0') {
                    break;
                }
                serial_putc(module->cmdline[index]);
            }
            mantle_console_write("\n");
            mantle_console_write("[mb2] cmdline hex=");
            for (index = 0u; index < payload_size; ++index) {
                serial_byte((uint8_t)module->cmdline[index]);
                serial_putc(index + 1u == payload_size ? '\n' : ' ');
            }
            if (mantle_mb2_parse_module(module, tag->size, &parsed) == 0) {
                *image = (const uint8_t *)parsed.start;
                *size = parsed.size;
                mantle_console_write("MANTLE_MB2_MODULE_FOUND\n");
                mantle_console_write("ROOTFS_START=");
                serial_hex(parsed.start);
                mantle_console_write("\nROOTFS_END=");
                serial_hex(parsed.end);
                mantle_console_write("\nROOTFS_SIZE=");
                serial_decimal(*size);
                mantle_console_write("\n");
                return 0;
            }
        }
        cursor += (tag->size + 7u) & ~7u;
    }
    return -1;
}

void mantle_kernel_main(uint32_t multiboot_magic, uintptr_t multiboot_info)
{
    const uint8_t *rootfs_image;
    uint32_t rootfs_size;
    struct mantle_rootfs_file init_file;
    struct mantle_user_image init_image;

    serial_init();
    mantle_console_write("MantleOS\n");
    mantle_console_write("MantleOS kernel demarre\n");
    mantle_console_write("MANTLE_KERNEL_OK\n");
    if (multiboot_magic == 0x36d76289u) {
        mantle_console_write("MANTLE_MB2_MAGIC_OK\n");
    } else {
        mantle_console_write("MANTLE_MB2_MAGIC_ERROR\n");
    }
    if (mantle_framebuffer_init(multiboot_magic, multiboot_info) == 0) {
        mantle_console_write("MANTLE_GRAPHICS_OK\n");
    } else {
        mantle_console_write("MANTLE_GRAPHICS_UNAVAILABLE\n");
    }
    if (multiboot_magic != 0x36d76289u) {
        mantle_console_write("MANTLE_ROOTFS_NOT_FOUND\n");
        goto rootfs_error;
    }
    {
        int lookup = find_rootfs(multiboot_info, &rootfs_image, &rootfs_size);
        if (lookup == -2) {
            mantle_console_write("MANTLE_ROOTFS_INVALID\n");
            goto rootfs_error;
        }
        if (lookup != 0) {
            mantle_console_write("MANTLE_ROOTFS_NOT_FOUND\n");
            goto rootfs_error;
        }
    }
    if (rootfs_size > 0xffffffffu - (uint32_t)(uintptr_t)rootfs_image ||
        (uintptr_t)rootfs_image + rootfs_size < (uintptr_t)rootfs_image) {
        mantle_console_write("MANTLE_ROOTFS_MAPPING_ERROR\n");
        goto rootfs_error;
    }
    if (mantle_rootfs_mount(rootfs_image, rootfs_size) != 0) {
        mantle_console_write("MANTLE_ROOTFS_INVALID\n");
        goto rootfs_error;
    }
    mantle_console_write("MANTLE_ROOTFS_OK\n");
    mantle_arch_user_memory_init();
    mantle_arch_syscall_init();
    mantle_process_init();
    if (mantle_rootfs_open("/sbin/init", &init_file) != 0 ||
        mantle_elf_load(init_file.data, init_file.size, &init_image) != 0) {
        mantle_console_write("MANTLE_ELF_ERROR\n");
        for (;;) {
            __asm__ volatile ("cli; hlt");
        }
    }
    mantle_console_write("MANTLE_ELF_OK\n");
    mantle_enter_user(init_image.entry, init_image.stack);
rootfs_error:
    mantle_console_write("MANTLE_ROOTFS_ERROR\n");
    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}
