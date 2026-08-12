SHELL := /bin/sh
PROFILE ?= personal
export MANTLE_PROFILE=$(PROFILE)
.PHONY: iso clean test-uefi test-boot test-graphics
iso:
	sh ./build.sh
test-uefi: iso
	sh ./build/test-qemu.sh
test-boot: iso
	sh ./tests/qemu-boot.sh
test-graphics: iso
	sh ./tests/qemu-graphics.sh
clean:
	rm -rf build/work build/out
