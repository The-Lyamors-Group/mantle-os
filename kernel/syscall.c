#include "syscall.h"
#include "arch/x86_64/kernel.h"
#include "fs/rootfs.h"
#include "process/elf.h"
#include "process/process.h"

#define SYS_READ 0u
#define SYS_WRITE 1u
#define SYS_OPEN 2u
#define SYS_CLOSE 3u
#define SYS_STAT 4u
#define SYS_LSEEK 5u
#define SYS_EXEC 6u
#define SYS_WAIT 7u
#define SYS_GETPID 8u
#define SYS_CHDIR 9u
#define SYS_GETCWD 10u
#define SYS_EXIT 11u
#define SYS_REBOOT 12u
#define SYS_READDIR 13u
#define SYS_UNAME 14u

#define USER_BASE 0x00400000u
#define USER_LIMIT 0x00600000u
#define MAX_PATH 128u
#define MAX_FDS 8u

struct file_descriptor {
    int used;
    struct mantle_rootfs_file file;
    uint32_t offset;
};

static struct file_descriptor descriptors[MAX_FDS];
static int user_marker_seen;

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a" (value) : "Nd" (port));
    return value;
}

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile ("outb %0, %1" : : "a" (value), "Nd" (port));
}

static int user_range(uint64_t address, uint64_t size)
{
    return address >= USER_BASE && address <= USER_LIMIT && size <= USER_LIMIT - address;
}

static uint32_t string_length_user(uint64_t address)
{
    const char *text = (const char *)(uintptr_t)address;
    uint32_t length = 0u;
    if (!user_range(address, 1u)) {
        return 0xffffffffu;
    }
    while (length < MAX_PATH && user_range(address + length, 1u) && text[length] != '\0') {
        ++length;
    }
    if (length == MAX_PATH || !user_range(address + length, 1u)) {
        return 0xffffffffu;
    }
    return length;
}

static uint8_t keyboard_read(void)
{
    static const char keymap[] = "??1234567890-=??qwertyuiop[]??asdfghjkl;'`?\\zxcvbnm,./";
    uint8_t scan;
    for (;;) {
        if ((inb(0x64u) & 1u) == 0u) {
            continue;
        }
        scan = inb(0x60u);
        if ((scan & 0x80u) != 0u || scan >= sizeof(keymap) - 1u) {
            continue;
        }
        if (scan == 0x1cu) {
            return '\n';
        }
        if (scan == 0x39u) {
            return ' ';
        }
        if (keymap[scan] == '?') {
            continue;
        }
        return (uint8_t)keymap[scan];
    }
}

static int32_t syscall_write(uint64_t fd, uint64_t address, uint64_t length)
{
    uint32_t index;
    const char *text;
    if ((fd != 1u && fd != 2u) || !user_range(address, length)) {
        return -1;
    }
    text = (const char *)(uintptr_t)address;
    for (index = 0u; index < (uint32_t)length; ++index) {
        char buffer[2];
        buffer[0] = text[index];
        buffer[1] = '\0';
        mantle_console_write(buffer);
    }
    return (int32_t)length;
}

static int32_t syscall_read(uint64_t fd, uint64_t address, uint64_t length)
{
    uint32_t index;
    uint8_t value;
    char *destination;
    if (fd != 0u || length == 0u || !user_range(address, length)) {
        return -1;
    }
    destination = (char *)(uintptr_t)address;
    for (index = 0u; index < (uint32_t)length; ++index) {
        value = keyboard_read();
        destination[index] = (char)value;
        if (value == '\n') {
            return (int32_t)(index + 1u);
        }
    }
    return (int32_t)length;
}

static int32_t syscall_open(uint64_t address)
{
    char path[MAX_PATH];
    uint32_t length = string_length_user(address);
    uint32_t index;
    if (length == 0xffffffffu || length >= MAX_PATH) {
        return -1;
    }
    for (index = 0u; index <= length; ++index) {
        path[index] = ((const char *)(uintptr_t)address)[index];
    }
    for (index = 3u; index < MAX_FDS; ++index) {
        if (!descriptors[index].used && mantle_rootfs_open(path, &descriptors[index].file) == 0) {
            descriptors[index].used = 1;
            descriptors[index].offset = 0u;
            return (int32_t)index;
        }
    }
    return -1;
}

static int32_t syscall_exec(struct mantle_syscall_frame *frame, uint64_t address)
{
    char path[MAX_PATH];
    struct mantle_rootfs_file file;
    struct mantle_user_image image;
    uint32_t length = string_length_user(address);
    uint32_t index;
    if (length == 0xffffffffu || length >= MAX_PATH) {
        return -1;
    }
    for (index = 0u; index <= length; ++index) {
        path[index] = ((const char *)(uintptr_t)address)[index];
    }
    if (mantle_rootfs_open(path, &file) != 0 || mantle_elf_load(file.data, file.size, &image) != 0) {
        return -1;
    }
    frame->rip = image.entry;
    frame->rsp = image.stack;
    return 0;
}

void mantle_syscall_dispatch(struct mantle_syscall_frame *frame)
{
    struct mantle_process *process = mantle_current_process();
    int32_t result = -1;

    if (!user_marker_seen) {
        user_marker_seen = 1;
        mantle_console_write("MANTLE_USERSPACE_OK\n");
    }
    switch ((uint32_t)frame->rax) {
    case SYS_READ:
        result = syscall_read(frame->rdi, frame->rsi, frame->rdx);
        break;
    case SYS_WRITE:
        result = syscall_write(frame->rdi, frame->rsi, frame->rdx);
        break;
    case SYS_OPEN:
        result = syscall_open(frame->rdi);
        break;
    case SYS_CLOSE:
        if (frame->rdi < MAX_FDS && descriptors[frame->rdi].used) {
            descriptors[frame->rdi].used = 0;
            result = 0;
        }
        break;
    case SYS_EXEC:
        result = syscall_exec(frame, frame->rdi);
        break;
    case SYS_GETPID:
        result = (int32_t)process->pid;
        break;
    case SYS_CHDIR:
        if (string_length_user(frame->rdi) == 1u &&
            ((const char *)(uintptr_t)frame->rdi)[0] == '/' &&
            ((const char *)(uintptr_t)frame->rdi)[1] == '\0') {
            uint32_t length = string_length_user(frame->rdi);
            if (length < sizeof(process->cwd)) {
                uint32_t index;
                for (index = 0u; index <= length; ++index) {
                    process->cwd[index] = ((const char *)(uintptr_t)frame->rdi)[index];
                }
                result = 0;
            }
        }
        break;
    case SYS_GETCWD:
        if (user_range(frame->rdi, frame->rsi)) {
            uint32_t length = 0u;
            while (process->cwd[length] != '\0') {
                ++length;
            }
            if (frame->rsi > length) {
                uint32_t index;
                for (index = 0u; index <= length; ++index) {
                    ((char *)(uintptr_t)frame->rdi)[index] = process->cwd[index];
                }
                result = (int32_t)length;
            }
        }
        break;
    case SYS_READDIR:
        if (string_length_user(frame->rdi) == 1u && user_range(frame->rsi, frame->rdx)) {
            result = mantle_rootfs_list((const char *)(uintptr_t)frame->rdi,
                (char *)(uintptr_t)frame->rsi, (uint32_t)frame->rdx);
        }
        break;
    case SYS_UNAME:
        if (user_range(frame->rdi, frame->rsi) && frame->rsi >= 18u) {
            static const char name[] = "MantleOS x86_64\0";
            uint32_t index;
            for (index = 0u; index < sizeof(name); ++index) {
                ((char *)(uintptr_t)frame->rdi)[index] = name[index];
            }
            result = (int32_t)(sizeof(name) - 1u);
        }
        break;
    case SYS_EXIT:
        process->state = MANTLE_PROCESS_EXITED;
        process->exit_code = (int32_t)frame->rdi;
        mantle_console_write("MANTLE_USERSPACE_EXIT\n");
        for (;;) {
            __asm__ volatile ("cli; hlt");
        }
    case SYS_REBOOT:
        outb(0x64u, 0xfeu);
        for (;;) {
            __asm__ volatile ("cli; hlt");
        }
    default:
        result = -1;
        break;
    }
    frame->rax = result < 0 ? (~(uint64_t)(-result) + 1u) : (uint64_t)result;
}
