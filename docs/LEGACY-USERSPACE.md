# Travaux userspace historiques

Les anciens fichiers de `init/`, `system/`, `services/`, `shell/`, `scripting/`
et `compat/` ont été conservés pour ne pas perdre le travail préparatoire.
Ils ciblaient une architecture Linux userspace et ne sont pas utilisés par la
chaîne active.

La chaîne actuelle est explicitement noyau-only : elle ne télécharge, ne
compile, n’embarque et ne démarre ni noyau Linux, ni BusyBox, ni musl, ni
initramfs ou rootfs de distribution. Toute réactivation nécessitera d’abord
une libc et une ABI MantleOS réellement implémentées.
