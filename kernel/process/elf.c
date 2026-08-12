#include "elf.h"
#include "../fs/rootfs.h"

#define USER_BASE 0x00400000u
#define USER_LIMIT 0x00600000u
#define USER_STACK_TOP USER_LIMIT

#define EI_NIDENT 16u
#define ELFCLASS64 2u
#define ELFDATA2LSB 1u
#define ET_EXEC 2u
#define EM_X86_64 62u
#define PT_LOAD 1u
#define PF_X 1u
#define PF_W 2u
#define PF_R 4u
#define PAGE_PRESENT 1ull
#define PAGE_WRITE 2ull
#define PAGE_USER 4ull
#define PAGE_NX 0x8000000000000000ull

struct elf_header {
    uint8_t ident[EI_NIDENT];
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint64_t entry;
    uint64_t phoff;
    uint64_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
};

struct elf_program_header {
    uint32_t type;
    uint32_t flags;
    uint64_t offset;
    uint64_t vaddr;
    uint64_t paddr;
    uint64_t filesz;
    uint64_t memsz;
    uint64_t align;
};

extern uint8_t mantle_user_memory[];
extern uint64_t user_page_table[];

static void copy_bytes(uint8_t *destination, const uint8_t *source, uint32_t size)
{
    uint32_t index;
    for (index = 0u; index < size; ++index) {
        destination[index] = source[index];
    }
}

static void clear_bytes(uint8_t *destination, uint32_t size)
{
    uint32_t index;
    for (index = 0u; index < size; ++index) {
        destination[index] = 0u;
    }
}

static int range64(uint64_t offset, uint64_t length, uint64_t size)
{
    return offset <= size && length <= size - offset;
}

static int user_range(uint64_t address, uint64_t size)
{
    return address >= USER_BASE && address <= USER_LIMIT && size <= USER_LIMIT - address;
}

int mantle_elf_load(const uint8_t *image, uint32_t size, struct mantle_user_image *result)
{
    const struct elf_header *header;
    uint16_t index;
    uint16_t load_count = 0u;
    int entry_valid = 0;

    if (image == (const uint8_t *)0 || result == (struct mantle_user_image *)0 ||
        size < sizeof(struct elf_header)) {
        return -1;
    }
    header = (const struct elf_header *)image;
    if (header->ident[0] != 0x7fu || header->ident[1] != 'E' || header->ident[2] != 'L' ||
        header->ident[3] != 'F' || header->ident[4] != ELFCLASS64 || header->ident[5] != ELFDATA2LSB ||
        header->type != ET_EXEC || header->machine != EM_X86_64 || header->version != 1u ||
        header->ehsize != sizeof(struct elf_header) || header->phentsize != sizeof(struct elf_program_header) ||
        header->phnum == 0u || header->phnum > 64u || !range64(header->phoff,
        (uint64_t)header->phnum * header->phentsize, size)) {
        return -1;
    }
    if (!user_range(header->entry, 1u)) {
        return -1;
    }
    clear_bytes(mantle_user_memory, 0x200000u);
    for (index = 0u; index < 512u; ++index) {
        user_page_table[index] = ((uint64_t)(uintptr_t)mantle_user_memory +
            ((uint64_t)index * 4096u)) | PAGE_PRESENT | PAGE_USER | PAGE_NX;
    }
    for (index = 496u; index < 512u; ++index) {
        user_page_table[index] = ((uint64_t)(uintptr_t)mantle_user_memory +
            ((uint64_t)index * 4096u)) | PAGE_PRESENT | PAGE_WRITE | PAGE_USER | PAGE_NX;
    }
    for (index = 0u; index < header->phnum; ++index) {
        const struct elf_program_header *program =
            (const struct elf_program_header *)(image + header->phoff + ((uint64_t)index * header->phentsize));
        uint64_t end;
        uint32_t destination;
        uint32_t first_page;
        uint32_t last_page;
        uint32_t page;
        uint64_t permissions;

        if (program->type != PT_LOAD) {
            continue;
        }
        if (program->filesz > program->memsz || program->memsz == 0u ||
            !range64(program->offset, program->filesz, size) ||
            !user_range(program->vaddr, program->memsz) ||
            ((program->flags & (PF_W | PF_X)) == (PF_W | PF_X))) {
            return -1;
        }
        end = program->vaddr + program->memsz;
        if (end < program->vaddr || (program->align != 0u && (program->align & (program->align - 1u)) != 0u) ||
            (program->align > 1u && (program->vaddr & (program->align - 1u)) !=
            (program->offset & (program->align - 1u)))) {
            return -1;
        }
        if ((program->flags & PF_X) != 0u && header->entry >= program->vaddr && header->entry < end) {
            entry_valid = 1;
        }
        destination = (uint32_t)(program->vaddr - USER_BASE);
        copy_bytes(mantle_user_memory + destination, image + program->offset, (uint32_t)program->filesz);
        clear_bytes(mantle_user_memory + destination + (uint32_t)program->filesz,
            (uint32_t)(program->memsz - program->filesz));
        first_page = destination / 4096u;
        last_page = (destination + (uint32_t)program->memsz + 4095u) / 4096u;
        permissions = PAGE_PRESENT | PAGE_USER;
        if ((program->flags & PF_W) != 0u) {
            permissions |= PAGE_WRITE;
        }
        if ((program->flags & PF_X) == 0u) {
            permissions |= PAGE_NX;
        }
        for (page = first_page; page < last_page; ++page) {
            user_page_table[page] = ((uint64_t)(uintptr_t)mantle_user_memory +
                ((uint64_t)page * 4096u)) | permissions;
        }
        ++load_count;
    }
    if (load_count == 0u || !entry_valid) {
        return -1;
    }
    result->entry = (uintptr_t)header->entry;
    result->stack = USER_STACK_TOP;
    return 0;
}
