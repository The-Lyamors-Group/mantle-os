# Compatibilité Unix/Linux

MantleOS conserve les commandes POSIX apportées par BusyBox et ajoute des implémentations natives compilées dans le build : `mantle-shell` (dash compilé depuis ses sources), GNU Make, OpenSSL, curl, Git, rsync, OpenSSH, sudo et pkg-config. Le profil `full` ajoute Python. Le profil SDK (`MANTLE_SDK=1`) ajoute GCC, son frontend C++, CMake et Clang/LLVM. Les compilateurs ne sont jamais remplacés par des alias.

Le script `build-compat.sh` reçoit le rootfs et le sysroot MantleOS. Il ne copie aucun `/usr` d’une distribution et ne crée pas de commandes factices. `wget` reste fourni par l’applet BusyBox jusqu’à l’intégration d’un client dédié ; il s’agit d’une implémentation réelle, pas d’un script d’alias.
