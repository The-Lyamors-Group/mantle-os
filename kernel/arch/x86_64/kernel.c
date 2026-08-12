#include <mantle/types.h>

#define COM1 0x3f8u
#define VGA_MEMORY 0xb8000u

static uint16_t vga_cursor;

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

static void print(const char *text)
{
    while (*text != '\0') {
        serial_putc(*text);
        vga_putc(*text);
        ++text;
    }
}

void mantle_kernel_main(void)
{
    serial_init();
    print("MantleOS\n");
    print("MantleOS kernel demarre\n");
    print("MANTLE_KERNEL_OK\n");
    for (;;) {
        __asm__ volatile ("hlt");
    }
}
