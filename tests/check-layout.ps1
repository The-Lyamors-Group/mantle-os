$ErrorActionPreference = 'Stop'
$required = @(
  'kernel/arch/x86_64/boot.S',
  'kernel/arch/x86_64/kernel.c',
  'kernel/graphics/framebuffer.c',
  'kernel/graphics/framebuffer.h',
  'kernel/process/elf.c',
  'kernel/process/elf.h',
  'kernel/process/process.c',
  'kernel/process/process.h',
  'kernel/fs/rootfs.c',
  'kernel/fs/rootfs.h',
  'kernel/syscall.c',
  'kernel/syscall.h',
  'kernel/linker.ld',
  'kernel/Makefile',
  'userspace/Makefile',
  'userspace/linker.ld',
  'userspace/build-rootfs.py',
  'userspace/libc/mantle.h',
  'userspace/libc/start.S',
  'userspace/libc/syscalls.S',
  'userspace/libc/mantle-string.c',
  'include/mantle/types.h',
  'build/make-image.sh',
  'boot/grub/grub.cfg',
  'tests/verify-image.sh',
  'tests/verify-userspace.sh',
  'tests/qemu-boot.sh',
  'tests/qemu-graphics.sh',
  'tests/verify-kernel-source.sh',
  'tests/check-layout.sh',
  'sources.lock'
)
foreach ($path in $required) { if (-not (Test-Path $path)) { throw "Fichier absent: $path" } }
$activeFiles = @('build.sh', 'build/make-image.sh', 'boot/grub/grub.cfg', '.github/workflows/mantleos-build.yml')
$active = ($activeFiles | ForEach-Object { Get-Content -Raw $_ }) -join "`n"
$forbidden = @('linux-[0-9]', 'busybox-[0-9]', 'musl-[0-9]', 'cdn.kernel.org', 'busybox.net', 'musl.libc.org', 'initramfs', 'rootfs.ext4', 'vmlinuz', '(?m)^\s*linux\s+')
foreach ($pattern in $forbidden) { if ($active -match $pattern) { throw "Dépendance Linux active détectée: $pattern" } }
$grub = Get-Content -Raw boot/grub/grub.cfg
if ($grub -notmatch 'multiboot2 /boot/mantle-kernel\.elf') { throw 'Entrée Multiboot2 absente de GRUB' }
if ((Get-Content -Raw sources.lock) -match '(?i)linux|busybox|musl|https?://') { throw 'sources.lock contient une dépendance externe interdite' }
Write-Output 'MantleOS kernel boundary checks: OK'
