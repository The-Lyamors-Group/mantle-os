# Architecture MantleOS

MantleOS ne repose pas sur Linux comme noyau et ne construit pas un rootfs de
distribution. L’image active est une image Multiboot2 contenant le noyau
freestanding MantleOS compilé depuis `kernel/`.

```text
UEFI → GRUB → Multiboot2 → kernel/arch/x86_64 → framebuffer + VGA/COM1 → idle
```

Le noyau actuel :

- entre dans `_start` depuis GRUB Multiboot2 ;
- installe une GDT et passe en mode long x86_64 ;
- installe une identité de pages de 2 MiB pour son code initial ;
- initialise le framebuffer direct RGB, COM1 et la mémoire vidéo VGA sans libc ;
- dessine le fond et le logo du splash MantleOS lorsque GRUB fournit un framebuffer ;
- écrit `MantleOS`, `MantleOS kernel demarre` et `MANTLE_KERNEL_OK` ;
- écrit `MANTLE_GRAPHICS_OK` uniquement après le rendu framebuffer ;
- reste dans une boucle `hlt` stable.

Les frontières `memory/`, `interrupts/`, `drivers/`, `syscall/` et `scheduler/`
sont préparées, mais leurs sous-systèmes sont encore TODO. `libc/`,
`userspace/`, le shell et les anciens services sont conservés dans le dépôt
pour les étapes suivantes ; ils ne sont ni compilés ni embarqués par l’ISO.

GRUB est actuellement le bootloader autorisé. Le fichier ISO est généré par
`grub-mkrescue`, puis démarré avec OVMF dans QEMU. Aucune archive de noyau,
BusyBox, musl, initramfs, rootfs Linux ou binaire de distribution n’est requis
par la chaîne active.
