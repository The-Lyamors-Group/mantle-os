BUILD ?= ../build/work/kernel

ifeq ($(origin CC),default)
CC := gcc
endif
ifeq ($(origin LD),default)
LD := ld
endif

CFLAGS := -std=c11 -ffreestanding -fno-builtin -fno-pie -fno-stack-protector -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -Wall -Wextra -Wpedantic -Werror -O2 -I../include

.PHONY: all clean
all: $(BUILD)/mantle-kernel.elf

$(BUILD):
	mkdir -p "$@"

$(BUILD)/boot.o: arch/x86_64/boot.S | $(BUILD)
	$(CC) $(CFLAGS) -c "$<" -o "$@"

$(BUILD)/kernel.o: arch/x86_64/kernel.c | $(BUILD)
	$(CC) $(CFLAGS) -c "$<" -o "$@"

$(BUILD)/framebuffer.o: graphics/framebuffer.c graphics/framebuffer.h | $(BUILD)
	$(CC) $(CFLAGS) -c "$<" -o "$@"

$(BUILD)/multiboot2.o: boot/multiboot2.c boot/multiboot2.h | $(BUILD)
	$(CC) $(CFLAGS) -c "$<" -o "$@"

$(BUILD)/syscall.o: syscall.c syscall.h | $(BUILD)
	$(CC) $(CFLAGS) -c "$<" -o "$@"

$(BUILD)/rootfs.o: fs/rootfs.c fs/rootfs.h | $(BUILD)
	$(CC) $(CFLAGS) -c "$<" -o "$@"

$(BUILD)/elf.o: process/elf.c process/elf.h fs/rootfs.h | $(BUILD)
	$(CC) $(CFLAGS) -c "$<" -o "$@"

$(BUILD)/process.o: process/process.c process/process.h | $(BUILD)
	$(CC) $(CFLAGS) -c "$<" -o "$@"

$(BUILD)/mantle-kernel.elf: $(BUILD)/boot.o $(BUILD)/kernel.o $(BUILD)/framebuffer.o $(BUILD)/multiboot2.o $(BUILD)/syscall.o $(BUILD)/rootfs.o $(BUILD)/elf.o $(BUILD)/process.o linker.ld
	$(LD) -m elf_x86_64 -nostdlib -z max-page-size=0x1000 -T linker.ld -o "$@" $(BUILD)/boot.o $(BUILD)/kernel.o $(BUILD)/framebuffer.o $(BUILD)/multiboot2.o $(BUILD)/syscall.o $(BUILD)/rootfs.o $(BUILD)/elf.o $(BUILD)/process.o

clean:
	rm -rf "$(BUILD)"
