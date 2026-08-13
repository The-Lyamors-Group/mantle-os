BUILD ?= ../build/work/userspace

# GNU Make fournit historiquement CC=cc par défaut. MSYS2 ne garantit pas
# l'existence d'un alias cc utilisable : sélectionner gcc uniquement lorsque
# l'appelant n'a pas explicitement choisi un compilateur.
ifeq ($(origin CC),default)
CC := gcc
endif
ifeq ($(origin LD),default)
LD := ld
endif

CFLAGS := -m64 -std=c11 -ffreestanding -fno-builtin -fno-pie -fno-stack-protector -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -Wall -Wextra -Wpedantic -Werror -O2 -Ilibc
LDFLAGS := -m elf_x86_64 -nostdlib -z max-page-size=0x1000 -T linker.ld

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
