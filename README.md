# MantleOS

MantleOS est un système d’exploitation français et européen, open source,
construit indépendamment de Linux comme noyau et orienté confidentialité,
sécurité, compatibilité et souveraineté numérique.

> MantleOS est en développement. Le projet n’est ni certifié, ni homologué,
> ni approuvé par l’ANSSI, la DGSI ou une administration.

## Vision

MantleOS vise un ordinateur local contrôlable : pas de télémétrie obligatoire,
pas de publicité, pas de compte cloud imposé, pas de collecte silencieuse, des
standards ouverts et une architecture auditable. La compatibilité Unix/Linux
concernera le futur userspace ; elle ne signifie pas que Linux est le noyau de
MantleOS.

## État du projet

Le niveau réellement atteint est :

- noyau x86_64 MantleOS minimal compilé depuis `kernel/` ;
- image ISO GRUB/UEFI avec chargement Multiboot2 ;
- sortie console VGA et série COM1 ;
- marqueur de boot `MANTLE_KERNEL_OK` ;
- build reproductible et test QEMU UEFI dans GitHub Actions ;
- userspace, libc, services, réseau, interface graphique et installateur encore
  indisponibles dans l’image indépendante actuelle.

Le boot n’est déclaré validé qu’après le test QEMU de la CI. Aucun résultat
local non exécuté ne doit être présenté comme un boot réussi.

## Fonctionnalités

### Fonctionnel dans la fondation actuelle

- compilation freestanding du noyau MantleOS avec GCC et binutils ;
- passage x86_64 long mode, GDT et pagination identité initiale ;
- affichage VGA et port série ;
- génération de `build/out/mantleos-amd64.iso` ;
- hash SHA-256, informations de build et contrôle Multiboot2 ;
- démarrage automatisé avec GRUB, UEFI/OVMF et QEMU.

### Expérimental ou non embarqué

Les sources historiques de `init/`, `system/`, `services/`, `shell/`,
`scripting/` et `compat/` sont conservées pour leur valeur de travail, mais ne
sont pas compilées ni exécutées par la chaîne noyau actuelle. Elles ne
constituent pas encore un userspace MantleOS fonctionnel.

### Prévu

libc et ABI MantleOS, mémoire, interruptions, pilotes, scheduler, appels
système, processus, stockage, réseau, shell, session Wayland, Mantle Shell,
Settings, Files, Terminal, Software, mises à jour et installateur.

## Architecture

```text
UEFI → GRUB → Multiboot2 → noyau MantleOS → VGA/COM1 → état idle stable
```

Le noyau est entièrement dans ce dépôt : `kernel/arch/x86_64/boot.S`,
`kernel/arch/x86_64/kernel.c` et `kernel/linker.ld`. Il ne lie aucune libc et
ne démarre ni Linux, ni BusyBox, ni un initramfs ou rootfs Linux. GRUB est
conservé comme bootloader pragmatique et QEMU/OVMF comme environnement de test.

## Structure du dépôt

```text
kernel/     noyau freestanding MantleOS et sous-systèmes à venir
arch/       frontières d’architecture futures
boot/       configuration GRUB et ressources de boot
libc/       frontière de la future libc MantleOS
userspace/  frontière du futur userspace
shell/      sources préparatoires du shell MantleOS
include/    en-têtes partagés
init/       anciens travaux userspace conservés, non actifs
services/   anciens travaux de services conservés, non actifs
ui/         design system et futures interfaces
apps/       applications et intégrations futures
site/       présentation locale statique hors ligne
tests/      tests de layout, image et boot QEMU
docs/       architecture, build, sécurité et formats
build/      génération de l’ISO et artefacts
```

## Build

Sur Linux ou dans le conteneur de build :

```sh
MANTLE_PROFILE=personal sh ./build.sh
```

Profils acceptés : `personal`, `education`, `government`. Ils sont enregistrés
dans les métadonnées ; les politiques spécialisées ne sont pas encore
implémentées.

Sous Windows avec Docker Desktop Linux :

```powershell
.\build.ps1 -Profile personal
```

Sorties :

```text
build/out/mantleos-amd64.iso
build/out/mantleos-amd64.iso.sha256
build/out/mantleos-build-info.txt
build/out/build.log
```

## Test en VM

Après le build :

```sh
sh ./tests/verify-image.sh
sh ./tests/qemu-boot.sh
```

La seconde commande utilise OVMF et QEMU, privilégie KVM quand disponible et
capture `build/out/qemu-serial.log`. Elle échoue si le noyau ne produit pas
`MANTLE_KERNEL_OK` dans le délai prévu.

Commande interactive :

```sh
sh ./build/test-qemu.sh
```

## Scripts, paquets et site local

Les formats `.mt`, `.mtc` et `.mtpkg` sont documentés comme travaux futurs et
ne sont pas des composants exécutables de l’ISO actuelle. Leur présence dans
le dépôt ne doit pas être confondue avec une fonctionnalité déjà disponible.

`site/` reste une page statique de présentation hors ligne. Il n’est ni un
panneau d’administration, ni une dépendance de boot, et n’utilise pas de CDN,
police distante, analytics ou tracker.

## Sécurité et confidentialité

La fondation actuelle ne fournit pas encore les mécanismes de production.
Secure Boot, TPM 2.0, isolation, chiffrement, signatures de mises à jour,
permissions, pare-feu et journalisation contrôlée restent à concevoir et à
tester dans le noyau et le userspace MantleOS.

La chaîne n’impose aucun compte en ligne, aucune télémétrie, aucune publicité
et aucun service cloud. Ces engagements ne remplacent pas un audit.

## Profils

- **Personal** : poste local quotidien, lorsque le userspace sera disponible.
- **Education** : politiques et gestion d’établissement sans surveillance cachée.
- **Government** : dépôts internes, réseaux isolés et durcissement futur.

Ces profils sont des axes d’architecture, pas des certifications.

## Contribution

Lire [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) et
[docs/BUILDING.md](docs/BUILDING.md), conserver la séparation noyau/userspace,
ajouter des tests aux composants et documenter les limites. Ne jamais annoncer
un boot ou une capacité qui n’a pas été réellement compilée et testée.

## Licence

Le code original est distribué sous [licence MIT](LICENSE). Les composants
tiers conservés dans le dépôt gardent leurs licences respectives.

## Avertissement

MantleOS est un noyau expérimental indépendant, pas encore un système utilisable
au quotidien. Ne l’utilisez pas dans un environnement critique, sur des données
irremplaçables ou comme système certifié avant stabilisation et audits.
