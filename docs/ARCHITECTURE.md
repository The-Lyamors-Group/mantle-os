# Architecture MantleOS

MantleOS ne repose pas sur Linux comme noyau et ne construit pas un rootfs de
distribution. L’image active est une image Multiboot2 contenant le noyau
freestanding MantleOS compilé depuis `kernel/`.

```text
UEFI → GRUB → Multiboot2 → kernel/arch/x86_64 → framebuffer → module rootfs
MantleOS → ELF64 ring 3 → /sbin/init → mantle-shell console
```

Le noyau actuel :

- entre dans `_start` depuis GRUB Multiboot2 ;
- installe une GDT et passe en mode long x86_64 ;
- installe des tables de pages d'identité et une fenêtre userspace protégée ;
- initialise le framebuffer direct RGB, COM1 et la mémoire vidéo VGA sans libc ;
- dessine le fond et le logo du splash MantleOS lorsque GRUB fournit un framebuffer ;
- écrit `MantleOS`, `MantleOS kernel demarre` et `MANTLE_KERNEL_OK` ;
- écrit `MANTLE_GRAPHICS_OK` uniquement après le rendu framebuffer ;
- charge `/sbin/init` depuis le module rootfs et passe réellement en ring 3 ;
- expose une ABI syscall minimale décrite dans `docs/syscalls.md` ;
- laisse `mantle-shell` fonctionner en console avec clavier PS/2.

Les processus multiples, interruptions, scheduler, VFS mutable, stockage
persistant, réseau et libc POSIX complète sont encore TODO. Le rootfs actuel
est un module en lecture seule suffisant pour le premier userspace ; il ne
réutilise aucun rootfs Linux.

GRUB est actuellement le bootloader autorisé. Le fichier ISO est généré par
`grub-mkrescue`, puis démarré avec OVMF dans QEMU. Aucune archive de noyau,
BusyBox, musl, initramfs, rootfs Linux ou binaire de distribution n’est requis
par la chaîne active.
