# Noyau MantleOS

Le noyau actif de l’image est le code freestanding de `arch/x86_64/`. Il ne
utilise ni noyau Linux, ni libc, ni rootfs. GRUB le charge avec Multiboot2.

La première tranche initialise le mode long x86_64, une table de pages
identité minimale, une GDT, la console VGA et le port série COM1. Elle affiche
`MANTLE_KERNEL_OK`, puis reste volontairement dans une boucle `hlt` stable.

Les répertoires `memory/`, `interrupts/`, `drivers/`, `syscall/` et
`scheduler/` contiennent les frontières des futures sous-systèmes noyau. Ils ne
prétendent pas encore fournir ces fonctionnalités.
