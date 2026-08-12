# Construire MantleOS

## Linux ou CI

La chaîne active nécessite GCC, binutils, GNU Make, les modules GRUB EFI et
PC, xorriso et mtools. Le test de boot nécessite QEMU et OVMF. Sur Ubuntu :

```sh
sudo apt-get update
sudo apt-get install -y build-essential gcc binutils make grub-efi-amd64-bin grub-pc-bin grub-common xorriso mtools qemu-system-x86 ovmf
MANTLE_PROFILE=personal sh ./build.sh
sh ./tests/verify-image.sh
sh ./tests/qemu-boot.sh
```

Le résultat est `build/out/mantleos-amd64.iso`. Le script écrit également sa
somme SHA-256, `mantleos-build-info.txt`, `build.log` et, après le test QEMU,
`qemu-serial.log`.

Le build compile exclusivement `kernel/arch/x86_64/` depuis le dépôt. Il ne
télécharge ni Linux, ni BusyBox, ni musl, ni une archive de distribution.

## Windows

Le dépôt ne compile pas directement un noyau ELF depuis PowerShell. Utiliser
Docker Desktop Linux :

```powershell
.\build.ps1 -Profile personal
```

Le conteneur est un environnement d’outillage de build uniquement ; il ne
définit pas l’architecture de MantleOS.

## Compilation directe du noyau

Dans un environnement POSIX équipé de GCC et binutils :

```sh
make -C kernel clean all
```

Le fichier produit est `build/work/kernel/mantle-kernel.elf`. La cible ISO
exécute aussi `grub-file --is-x86-multiboot2` avant `grub-mkrescue`.

## CI

Le workflow `.github/workflows/mantleos-build.yml` installe les outils de
construction, vérifie la frontière d’architecture, produit l’ISO, vérifie son
hash, démarre l’image en UEFI QEMU et attend `MANTLE_KERNEL_OK`. Une erreur de
boot fait échouer le job. L’ISO et les diagnostics sont publiés comme artefacts
uniquement après réussite.
