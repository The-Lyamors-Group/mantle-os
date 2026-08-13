#include <efi.h>
#include <efiapi.h>
#include <efiprot.h>

typedef struct {
    unsigned char e_ident[16];
    uint16_t type, machine;
    uint32_t version;
    uint64_t entry, phoff, shoff;
    uint32_t flags;
    uint16_t ehsize, phentsize, phnum, shentsize, shnum, shstrndx;
} elf64_header;

typedef struct {
    uint32_t type, flags;
    uint64_t offset, vaddr, paddr, filesz, memsz, align;
} elf64_program;

typedef struct {
    uint32_t total_size, reserved;
} mb2_info_header;

typedef struct {
    uint32_t type, size, start, end;
    char command[14];
} mb2_module;

typedef struct {
    uint32_t type, size;
    uint64_t address;
    uint32_t pitch, width, height;
    uint8_t bpp, framebuffer_type;
    uint16_t reserved;
    uint8_t red_pos, red_mask, green_pos, green_mask, blue_pos, blue_mask;
} mb2_framebuffer;

static EFI_SYSTEM_TABLE *system_table;

static void *copy_bytes(void *destination, const void *source, UINTN size)
{
    UINTN index;
    unsigned char *to = destination;
    const unsigned char *from = source;
    for (index = 0; index < size; ++index) {
        to[index] = from[index];
    }
    return destination;
}

static void clear_bytes(void *destination, UINTN size)
{
    UINTN index;
    unsigned char *to = destination;
    for (index = 0; index < size; ++index) {
        to[index] = 0;
    }
}

static EFI_STATUS open_root(EFI_HANDLE image, EFI_FILE_HANDLE *root)
{
    EFI_STATUS status;
    EFI_LOADED_IMAGE_PROTOCOL *loaded;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *filesystem;
    EFI_GUID loaded_guid = LOADED_IMAGE_PROTOCOL;
    EFI_GUID filesystem_guid = SIMPLE_FILE_SYSTEM_PROTOCOL;

    status = system_table->BootServices->HandleProtocol(image, &loaded_guid,
        (void **)&loaded);
    if (EFI_ERROR(status)) return status;
    status = system_table->BootServices->HandleProtocol(loaded->DeviceHandle,
        &filesystem_guid, (void **)&filesystem);
    if (EFI_ERROR(status)) return status;
    return filesystem->OpenVolume(filesystem, root);
}

static EFI_STATUS read_file(EFI_FILE_HANDLE root, CHAR16 *path,
    void **buffer, UINTN *size)
{
    EFI_STATUS status;
    EFI_FILE_HANDLE file;
    UINTN info_size = 0;
    UINTN read_size;
    EFI_FILE_INFO *info;
    EFI_GUID info_guid = EFI_FILE_INFO_ID;

    status = root->Open(root, &file, path, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) return status;
    status = file->GetInfo(file, &info_guid, &info_size, NULL);
    if (status != EFI_BUFFER_TOO_SMALL) { file->Close(file); return status; }
    status = system_table->BootServices->AllocatePool(EfiLoaderData, info_size,
        (void **)&info);
    if (EFI_ERROR(status)) { file->Close(file); return status; }
    status = file->GetInfo(file, &info_guid, &info_size, info);
    if (EFI_ERROR(status)) { system_table->BootServices->FreePool(info); file->Close(file); return status; }
    *size = (UINTN)info->FileSize;
    status = system_table->BootServices->AllocatePool(EfiLoaderData, *size,
        buffer);
    if (EFI_ERROR(status)) { system_table->BootServices->FreePool(info); file->Close(file); return status; }
    read_size = *size;
    status = file->Read(file, &read_size, *buffer);
    system_table->BootServices->FreePool(info);
    file->Close(file);
    if (EFI_ERROR(status) || read_size != *size) {
        system_table->BootServices->FreePool(*buffer);
        *buffer = NULL;
        return EFI_LOAD_ERROR;
    }
    return EFI_SUCCESS;
}

static EFI_STATUS load_kernel(const void *image, UINTN image_size,
    UINTN *entry)
{
    const elf64_header *header = image;
    const unsigned char *bytes = image;
    UINT16 index;

    if (image_size < sizeof(*header) || header->e_ident[0] != 0x7f ||
        header->e_ident[1] != 'E' || header->e_ident[2] != 'L' ||
        header->e_ident[3] != 'F' || header->e_ident[4] != 2 ||
        header->e_ident[5] != 1 || header->machine != 62 ||
        header->phentsize != sizeof(elf64_program)) return EFI_LOAD_ERROR;
    if (header->phoff > image_size || header->phnum > 32 ||
        header->phoff + (UINTN)header->phnum * header->phentsize > image_size)
        return EFI_LOAD_ERROR;
    for (index = 0; index < header->phnum; ++index) {
        const elf64_program *program = (const elf64_program *)(bytes +
            header->phoff + (UINTN)index * header->phentsize);
        EFI_PHYSICAL_ADDRESS address = program->paddr;
        UINTN pages;
        if (program->type != 1) continue;
        if (program->filesz > program->memsz || program->offset > image_size ||
            program->offset + program->filesz > image_size ||
            program->paddr + program->memsz < program->paddr) return EFI_LOAD_ERROR;
        pages = (UINTN)((program->memsz + 0xfff) / 0x1000);
        if (EFI_ERROR(system_table->BootServices->AllocatePages(AllocateAddress,
            EfiLoaderData, pages, &address)) || address != program->paddr)
            return EFI_LOAD_ERROR;
        clear_bytes((void *)(UINTN)program->paddr, pages * 0x1000);
        copy_bytes((void *)(UINTN)program->paddr, bytes + program->offset,
            (UINTN)program->filesz);
    }
    *entry = (UINTN)header->entry;
    return EFI_SUCCESS;
}

static EFI_STATUS build_info(void *rootfs, UINTN rootfs_size, UINTN *info_address)
{
    EFI_PHYSICAL_ADDRESS address = 0xffffffffu;
    mb2_info_header *info;
    mb2_module *module;
    mb2_framebuffer *framebuffer;
    uint32_t offset = 8;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    UINTN pages = 1;

    if (EFI_ERROR(system_table->BootServices->AllocatePages(AllocateMaxAddress,
        EfiLoaderData, pages, &address))) return EFI_OUT_OF_RESOURCES;
    info = (mb2_info_header *)(UINTN)address;
    clear_bytes(info, 0x1000);
    module = (mb2_module *)((unsigned char *)info + offset);
    module->type = 3; module->size = 30;
    module->start = (uint32_t)(UINTN)rootfs;
    module->end = module->start + (uint32_t)rootfs_size;
    copy_bytes(module->command, "mantle-rootfs", 13);
    offset += 32;

    if (!EFI_ERROR(system_table->BootServices->LocateProtocol(&gop_guid, NULL,
        (void **)&gop)) && gop && gop->Mode) {
        framebuffer = (mb2_framebuffer *)((unsigned char *)info + offset);
        framebuffer->type = 8; framebuffer->size = 38;
        framebuffer->address = gop->Mode->FrameBufferBase;
        framebuffer->pitch = gop->Mode->Info->PixelsPerScanLine * 4;
        framebuffer->width = gop->Mode->Info->HorizontalResolution;
        framebuffer->height = gop->Mode->Info->VerticalResolution;
        framebuffer->bpp = 32; framebuffer->framebuffer_type = 1;
        framebuffer->red_pos = 0; framebuffer->green_pos = 8; framebuffer->blue_pos = 16;
        offset += 40;
    }
    *(uint32_t *)((unsigned char *)info + offset) = 0;
    *(uint32_t *)((unsigned char *)info + offset + 4) = 8;
    info->total_size = offset + 8;
    *info_address = (UINTN)info;
    return EFI_SUCCESS;
}

__attribute__((noreturn)) static void jump_kernel(UINTN entry, UINTN info)
{
    __asm__ volatile("movl $0x36d76289, %%eax; movq %0, %%rbx; jmp *%1" ::
        "r"(info), "r"(entry) : "rax", "rbx", "memory");
    __builtin_unreachable();
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *table)
{
    EFI_STATUS status;
    EFI_FILE_HANDLE root;
    void *kernel = NULL, *rootfs = NULL;
    UINTN kernel_size, rootfs_size, entry, info;
    UINTN map_size = 0, map_key, descriptor_size;
    UINT32 descriptor_version;
    void *memory_map;

    system_table = table;
    status = open_root(image, &root);
    if (EFI_ERROR(status)) return status;
    status = read_file(root, u"\\boot\\mantle-kernel.elf", &kernel, &kernel_size);
    if (EFI_ERROR(status)) return status;
    status = read_file(root, u"\\boot\\mantle-rootfs.mfs", &rootfs, &rootfs_size);
    if (EFI_ERROR(status)) return status;
    if (rootfs_size > 0xffffffffu) return EFI_LOAD_ERROR;
    status = load_kernel(kernel, kernel_size, &entry);
    if (EFI_ERROR(status)) return status;
    status = build_info(rootfs, rootfs_size, &info);
    if (EFI_ERROR(status)) return status;

    status = table->BootServices->GetMemoryMap(&map_size, NULL, &map_key,
        &descriptor_size, &descriptor_version);
    if (status != EFI_BUFFER_TOO_SMALL) return status;
    map_size += descriptor_size * 2;
    status = table->BootServices->AllocatePool(EfiLoaderData, map_size,
        &memory_map);
    if (EFI_ERROR(status)) return status;
    status = table->BootServices->GetMemoryMap(&map_size, memory_map, &map_key,
        &descriptor_size, &descriptor_version);
    if (EFI_ERROR(status)) return status;
    status = table->BootServices->ExitBootServices(image, map_key);
    if (EFI_ERROR(status)) return status;
    jump_kernel(entry, info);
    return EFI_SUCCESS;
}
