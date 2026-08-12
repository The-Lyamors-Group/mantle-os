# Construire MantleOS

## Linux

Installer `docker`, puis :

```sh
MANTLE_PROFILE=personal sh ./build.sh
```

Le builder télécharge des versions épinglées du noyau Linux et de BusyBox, compile le noyau, compile les utilitaires, compile `mantle-init`, génère l’initramfs cpio gzip et assemble `build/out/mantleos-amd64.iso` avec GRUB UEFI.

La libc est musl 1.2.5, compilée dans `build/work/sysroot`. Aucun binaire userspace n’est pris depuis la distribution de l’hôte. `MANTLE_COMPAT=core` réduit les outils additionnels ; `MANTLE_COMPAT=full` ajoute Python. `MANTLE_SDK=1` ajoute la compilation GCC/C++ et LLVM/Clang, puis CMake. La construction des images ext4 nécessite les privilèges de montage du conteneur ; `build.ps1` lance Docker en mode privilégié pour cette étape.

## Windows

Docker Desktop étant installé et démarré :

```powershell
.\build.ps1 -Profile personal
```

Le résultat n’est déclaré amorçable qu’après lancement dans QEMU : `make test-uefi` sur une machine Linux équipée de QEMU/KVM.

Après construction, vérifier les composants sans démarrer la VM avec `sh ./tests/verify-image.sh`.

La CI Ubuntu construit l’image dans un environnement Linux privilégié et vérifie l’initramfs, le rootfs ext4, le disque persistant et la somme SHA-256. Le test de démarrage UEFI QEMU (`make test-boot`) est séparé car il nécessite OVMF et un runner virtualisé.
