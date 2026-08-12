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
volatile uint32_t mantle_syscall_origin_user;
extern uint64_t user_page_table[];
extern uint64_t mantle_gdt[];
extern uint64_t mantle_pd_table0[];
extern uint64_t mantle_tss_rsp0;
extern void mantle_syscall_entry(void);
extern void mantle_exception_de(void);
extern void mantle_exception_ud(void);
extern void mantle_exception_df(void);
extern void mantle_exception_np(void);
extern void mantle_exception_ss(void);
extern void mantle_exception_gp(void);
extern void mantle_exception_pf(void);

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
    /* STAR[63:48] is the user SYSRET base.  0x13 + 16 = CS 0x23 and
       0x13 + 8 = SS 0x1b; STAR[47:32] is the kernel CS 0x08. */
    write_msr(0xc0000081u, (0x13ull << 48u) | (0x08ull << 32u));
    write_msr(0xc0000082u, (uint64_t)(uintptr_t)mantle_syscall_entry);
    write_msr(0xc0000084u, 0x200ull);
}

struct idt_gate {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t attributes;
    uint16_t offset_middle;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed));

struct idt_descriptor {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static void serial_hex(uint64_t value);
static void serial_decimal(uint32_t value);

struct mantle_exception_frame {
    uint64_t vector;
    uint64_t error_code;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

static struct idt_gate idt[256];

static void idt_set(uint32_t vector, void (*handler)(void))
{
    uintptr_t address = (uintptr_t)handler;
    idt[vector].offset_low = (uint16_t)address;
    idt[vector].selector = 0x08u;
    idt[vector].ist = 0u;
    idt[vector].attributes = 0x8eu;
    idt[vector].offset_middle = (uint16_t)(address >> 16u);
    idt[vector].offset_high = (uint32_t)(address >> 32u);
    idt[vector].reserved = 0u;
}

void mantle_arch_exception_init(void)
{
    struct idt_descriptor descriptor;
    uint32_t index;
    for (index = 0u; index < 256u; ++index) {
        idt[index].offset_low = 0u;
        idt[index].selector = 0u;
        idt[index].ist = 0u;
        idt[index].attributes = 0u;
        idt[index].offset_middle = 0u;
        idt[index].offset_high = 0u;
        idt[index].reserved = 0u;
    }
    idt_set(0u, mantle_exception_de);
    idt_set(6u, mantle_exception_ud);
    idt_set(8u, mantle_exception_df);
    idt_set(11u, mantle_exception_np);
    idt_set(12u, mantle_exception_ss);
    idt_set(13u, mantle_exception_gp);
    idt_set(14u, mantle_exception_pf);
    descriptor.limit = (uint16_t)(sizeof(idt) - 1u);
    descriptor.base = (uint64_t)(uintptr_t)idt;
    __asm__ volatile ("lidt %0" : : "m" (descriptor));
}

void mantle_exception_dispatch(struct mantle_exception_frame *frame)
{
    uint64_t cr2 = 0u;
    __asm__ volatile ("mov %%cr2, %0" : "=r" (cr2));
    mantle_console_write("EXCEPTION=");
    serial_decimal((uint32_t)frame->vector);
    mantle_console_write("\nRIP="); serial_hex(frame->rip);
    mantle_console_write("\nRSP="); serial_hex(frame->rsp);
    mantle_console_write("\nCS="); serial_hex(frame->cs);
    mantle_console_write("\nSS="); serial_hex(frame->ss);
    mantle_console_write("\nRFLAGS="); serial_hex(frame->rflags);
    mantle_console_write("\nERROR_CODE="); serial_hex(frame->error_code);
    mantle_console_write("\nCPL="); serial_decimal((uint32_t)(frame->cs & 3u));
    if (frame->vector == 14u) {
        mantle_console_write("\nCR2="); serial_hex(cr2);
    }
    mantle_console_write("\n");
    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}

static int mantle_tss_loaded(void)
{
    uint16_t selector;
    __asm__ volatile ("str %0" : "=r" (selector));
    return selector == 0x28u && mantle_tss_rsp0 != 0u;
}

static int ring3_diagnostics(uintptr_t entry, uintptr_t stack)
{
    uint64_t entry_pte;
    uint64_t stack_pte;
    uint64_t cr3;
    uint32_t entry_index = (uint32_t)((entry - 0x400000u) / 4096u);
    uint32_t stack_index = (uint32_t)((stack - 1u - 0x400000u) / 4096u);
    volatile uint64_t *stack_word = (volatile uint64_t *)(stack - 8u);
    const uint64_t sentinel = 0x4d414e544c455553ull;

    mantle_console_write("MANTLE_RING3_PREPARE\n");
    if (mantle_gdt[3] != 0x00cff2000000ffffull || mantle_gdt[4] != 0x00affa000000ffffull) {
        mantle_console_write("MANTLE_RING3_GDT_ERROR\n");
        return 0;
    }
    mantle_console_write("MANTLE_RING3_GDT_OK\n");
    if (!mantle_tss_loaded()) {
        mantle_console_write("MANTLE_RING3_TSS_ERROR\n");
        return 0;
    }
    mantle_console_write("MANTLE_TSS_LOADED\n");
    mantle_console_write("MANTLE_RING3_TSS_OK\n");
    if (stack < 0x400000u || stack > 0x600000u || (stack & 0xfu) != 0u) {
        mantle_console_write("MANTLE_RING3_STACK_ERROR\n");
        return 0;
    }
    *stack_word = sentinel;
    if (*stack_word != sentinel) {
        mantle_console_write("MANTLE_RING3_STACK_ERROR\n");
        return 0;
    }
    mantle_console_write("MANTLE_USER_STACK_OK\n");
    mantle_console_write("MANTLE_RING3_STACK_OK\n");
    if (entry < 0x400000u || entry >= 0x600000u || entry_index >= 512u || stack_index >= 512u) {
        mantle_console_write("MANTLE_RING3_PAGING_ERROR\n");
        return 0;
    }
    entry_pte = user_page_table[entry_index];
    stack_pte = user_page_table[stack_index];
    mantle_console_write("ENTRY_PTE="); serial_hex(entry_pte);
    mantle_console_write("\nSTACK_PTE="); serial_hex(stack_pte);
    mantle_console_write("\nKERNEL_PDE="); serial_hex(mantle_pd_table0[0]);
    mantle_console_write("\n");
    if ((entry_pte & 0x7u) != 0x5u || (entry_pte & 0x8000000000000000ull) != 0u ||
        (stack_pte & 0x7u) != 0x7u || (stack_pte & 0x8000000000000000ull) == 0u ||
        (mantle_pd_table0[0] & 0x4u) != 0u) {
        mantle_console_write("MANTLE_RING3_PAGING_ERROR\n");
        return 0;
    }
    mantle_console_write("MANTLE_RING3_PAGING_OK\n");
    __asm__ volatile ("mov %%cr3, %0" : "=r" (cr3));
    mantle_console_write("USER_ENTRY="); serial_hex(entry);
    mantle_console_write("\nUSER_STACK_TOP="); serial_hex(stack);
    mantle_console_write("\nUSER_CS=0x23\nUSER_SS=0x1b\nUSER_RFLAGS=0x2\nCR3=");
    serial_hex(cr3);
    mantle_console_write("\nMANTLE_RING3_ENTRY_OK\nMANTLE_RING3_IRET\n");
    return 1;
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
    if (mantle_rootfs_open("/bin/hello", &init_file) != 0 ||
        mantle_elf_load(init_file.data, init_file.size, &init_image) != 0) {
        mantle_console_write("MANTLE_ELF_ERROR\n");
        for (;;) {
            __asm__ volatile ("cli; hlt");
        }
    }
    mantle_console_write("MANTLE_ELF_OK\n");
    mantle_arch_exception_init();
    if (ring3_diagnostics(init_image.entry, init_image.stack)) {
        mantle_enter_user(init_image.entry, init_image.stack);
    }
rootfs_error:
    mantle_console_write("MANTLE_ROOTFS_ERROR\n");
    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}
