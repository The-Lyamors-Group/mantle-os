# MantleOS

MantleOS est un système expérimental construit indépendamment d’un userspace de distribution. La première milestone assemble un noyau Linux compilé depuis ses sources, un initramfs MantleOS, le PID 1 `mantle-init` compilé en C, BusyBox compilé dans le builder et une image GRUB UEFI.

## Construire

Depuis Linux avec Docker : `MANTLE_PROFILE=personal sh ./build.sh`. Depuis Windows avec Docker Desktop : `./build.ps1 -Profile personal`.

L’image produite est `build/out/mantleos-amd64.iso`, avec `mantleos-root.ext4` et `mantleos-disk.img`. Le test QEMU est `sh ./build/test-qemu.sh` ou `make test-uefi`. Le système est actuellement au statut **prototype console avec rootfs persistant préparé, réseau et logs intégrés ; boot QEMU non testé dans cet environnement**. L’interface graphique, l’installateur et le gestionnaire de paquets MantleOS ne sont pas déclarés terminés.

Le profil de compatibilité par défaut est `full` ; pour le SDK développeur : `MANTLE_SDK=1 ./build.sh`. Les outils sont compilés depuis `compat/sources.lock` et ne sont pas des alias de commandes absentes.

La commande locale principale est `mantle`. Elle exécute les scripts `.mt`, les séquences `.mtc`, sert la documentation avec `mantle docs` et refuse les paquets `.mtpkg` sans manifeste SHA-256 et signature approuvée.

Voir [docs/BUILDING.md](docs/BUILDING.md) et [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).
