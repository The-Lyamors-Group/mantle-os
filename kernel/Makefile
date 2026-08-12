ROOT := $(abspath ..)
BUILD := $(ROOT)/build/work/kernel
CC ?= gcc
LD ?= ld
CFLAGS := -std=c11 -ffreestanding -fno-builtin -fno-pie -fno-stack-protector -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -Wall -Wextra -Wpedantic -Werror -O2 -I$(ROOT)/include

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

$(BUILD)/mantle-kernel.elf: $(BUILD)/boot.o $(BUILD)/kernel.o $(BUILD)/framebuffer.o linker.ld
	$(LD) -nostdlib -z max-page-size=0x1000 -T linker.ld -o "$@" $(BUILD)/boot.o $(BUILD)/kernel.o $(BUILD)/framebuffer.o

clean:
	rm -rf "$(BUILD)"
