#include <mantle/types.h>
#include "../../graphics/framebuffer.h"
#include "../../fs/rootfs.h"
#include "../../process/elf.h"
#include "../../process/process.h"
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

struct multiboot_tag_module {
    uint32_t type;
    uint32_t size;
    uint32_t start;
    uint32_t end;
    uint32_t command_line;
};

static int find_rootfs(uintptr_t multiboot_info, const uint8_t **image, uint32_t *size)
{
    const struct multiboot_tag *tag;
    uintptr_t cursor;

    if (multiboot_info == 0u || image == (const uint8_t **)0 || size == (uint32_t *)0) {
        return -1;
    }
    cursor = multiboot_info + 8u;

    for (;;) {
        tag = (const struct multiboot_tag *)cursor;
        if (tag->type == 0u || tag->size < 8u) {
            return -1;
        }
        if (tag->type == 3u && tag->size >= sizeof(struct multiboot_tag_module)) {
            const struct multiboot_tag_module *module = (const struct multiboot_tag_module *)tag;
            if (module->end > module->start) {
                *image = (const uint8_t *)(uintptr_t)module->start;
                *size = module->end - module->start;
                return 0;
            }
        }
        cursor += (tag->size + 7u) & ~7u;
    }
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
    if (mantle_framebuffer_init(multiboot_magic, multiboot_info) == 0) {
        mantle_console_write("MANTLE_GRAPHICS_OK\n");
    } else {
        mantle_console_write("MANTLE_GRAPHICS_UNAVAILABLE\n");
    }
    if (multiboot_magic != 0x36d76289u || find_rootfs(multiboot_info, &rootfs_image, &rootfs_size) != 0 ||
        mantle_rootfs_mount(rootfs_image, rootfs_size) != 0) {
        mantle_console_write("MANTLE_ROOTFS_ERROR\n");
        for (;;) {
            __asm__ volatile ("cli; hlt");
        }
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
    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}
