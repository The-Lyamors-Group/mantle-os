# Graphiques MantleOS

Le premier composant graphique actif est un renderer framebuffer freestanding.
GRUB transmet le framebuffer direct RGB via Multiboot2 ; le kernel dessine le
fond et le logo de démarrage MantleOS, puis écrit `MANTLE_GRAPHICS_OK` sur COM1
uniquement après initialisation et rendu réussis.

Ce n’est pas encore une session Wayland, un compositeur DRM/KMS ou un bureau.
Le terminal, le login et les applications nécessitent d’abord la libc, l’ABI,
les appels système et un userspace MantleOS.
