# Noyau MantleOS

Le noyau actif de l’image est le code freestanding de `arch/x86_64/`. Il ne
utilise ni noyau Linux, ni libc, ni rootfs. GRUB le charge avec Multiboot2.

La première tranche initialise le mode long x86_64, une table de pages
identité minimale, une GDT, le framebuffer direct RGB fourni par Multiboot2,
la console VGA et le port série COM1. Elle dessine le splash MantleOS et affiche
`MANTLE_GRAPHICS_OK` uniquement après un rendu framebuffer réussi. Le kernel
reste ensuite volontairement dans une boucle `hlt` stable.

Les répertoires `memory/`, `interrupts/`, `drivers/`, `syscall/` et
`scheduler/` contiennent les frontières des futures sous-systèmes noyau. Ils ne
prétendent pas encore fournir ces fonctionnalités.
