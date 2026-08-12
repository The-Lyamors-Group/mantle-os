ROOT := $(abspath ..)
BUILD ?= $(ROOT)/build/work/userspace
CC ?= gcc
LD ?= ld
CFLAGS := -m64 -std=c11 -ffreestanding -fno-builtin -fno-pie -fno-stack-protector -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -Wall -Wextra -Wpedantic -Werror -O2 -I$(ROOT)/userspace/libc
LDFLAGS := -nostdlib -z max-page-size=0x1000 -T $(ROOT)/userspace/linker.ld

PROGRAMS := hello init mantle mantle-shell

.PHONY: all clean
all: $(PROGRAMS:%=$(BUILD)/%.elf)

$(BUILD):
	mkdir -p "$@"

$(BUILD)/start.o: libc/start.S | $(BUILD)
	$(CC) -m64 -ffreestanding -fno-pie -c "$<" -o "$@"

$(BUILD)/syscalls.o: libc/syscalls.S | $(BUILD)
	$(CC) -m64 -ffreestanding -fno-pie -c "$<" -o "$@"

$(BUILD)/string.o: libc/mantle-string.c | $(BUILD)
	$(CC) $(CFLAGS) -c "$<" -o "$@"

$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) $(CFLAGS) -c "$<" -o "$@"

$(BUILD)/hello.elf: $(BUILD)/start.o $(BUILD)/syscalls.o $(BUILD)/string.o $(BUILD)/hello.o
	$(LD) $(LDFLAGS) -o "$@" $^

$(BUILD)/init.elf: $(BUILD)/start.o $(BUILD)/syscalls.o $(BUILD)/string.o $(BUILD)/init.o
	$(LD) $(LDFLAGS) -o "$@" $^

$(BUILD)/mantle.elf: $(BUILD)/start.o $(BUILD)/syscalls.o $(BUILD)/string.o $(BUILD)/mantle.o
	$(LD) $(LDFLAGS) -o "$@" $^

$(BUILD)/mantle-shell.elf: $(BUILD)/start.o $(BUILD)/syscalls.o $(BUILD)/string.o $(BUILD)/mantle-shell.o
	$(LD) $(LDFLAGS) -o "$@" $^

clean:
	rm -rf "$(BUILD)"
