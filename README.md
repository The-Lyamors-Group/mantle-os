# MantleOS

MantleOS est un système d’exploitation français et européen, open source,
construit autour de la confidentialité, de la sécurité, de la compatibilité
Unix/Linux et de la souveraineté numérique.

Le projet assemble son propre userspace à partir de sources publiques : noyau
Linux, libc musl, initramfs, services MantleOS, outils Unix et image UEFI. Il
ne s’appuie pas sur un rootfs Debian ou sur une distribution réassemblée.

> MantleOS est un projet en développement. Il n’est ni certifié, ni homologué,
> ni approuvé par l’ANSSI, la DGSI ou une administration française.

## Vision

MantleOS vise un ordinateur local qui reste compréhensible et contrôlable :

- aucune télémétrie obligatoire, publicité ou identifiant publicitaire ;
- aucun compte cloud obligatoire et aucune connexion Internet requise pour le socle ;
- logiciels libres, formats ouverts, composants remplaçables et sources auditables ;
- compatibilité avec les habitudes Unix/Linux plutôt que rupture artificielle ;
- séparation administrateur/utilisateur, validation cryptographique et journalisation locale ;
- profils Personal, Education et Government partageant une même fondation ;
- architecture extensible vers Wayland, l’isolation des applications et les mises à jour hors ligne.

La souveraineté décrit une direction d’architecture, pas une certification. Les
fournisseurs et services externes ne sont pas des dépendances obligatoires de
MantleOS.

## État du projet

MantleOS est actuellement un prototype de fondation système :

- chaîne de build Linux documentée et CI GitHub configurée pour produire une ISO ;
- noyau Linux x86_64 compilé depuis les sources et amorçage GRUB UEFI ;
- initramfs MantleOS, rootfs ext4 persistant et `/sbin/init` MantleOS ;
- console, services locaux, logs, réseau DHCP/statique et utilisateurs de base ;
- `mantle-shell` natif séparé de `/bin/sh`, interpréteurs `.mt` et `.mtc` ;
- compatibilité Unix/Linux construite depuis des sources épinglées ;
- gestionnaire `.mtpkg` expérimental avec manifeste, signatures et transactions ;
- site local statique hors ligne ;
- pile graphique Wayland, login graphique, installateur et applications natives encore en développement.

Le boot réel est déclaré validé uniquement par le workflow CI après démarrage
UEFI dans QEMU et vérification des marqueurs série. Une validation statique
locale ne remplace pas ce test.

## Fonctionnalités

### Fonctionnel dans la chaîne actuelle

Ces éléments sont présents dans le dépôt et couverts par des tests de build ou
de structure. Leur validation de boot dépend de la CI Linux :

- compilation du noyau Linux, de musl, de BusyBox et des composants MantleOS ;
- image rootfs ext4, disque persistant, initramfs cpio gzip et ISO x86_64 UEFI ;
- `/proc`, `/sys`, `/dev`, `/run` et `/tmp` montés par le système d’initialisation ;
- shell POSIX `/bin/sh` fourni par dash et `mantle-shell` MantleOS distinct ;
- services supervisés, journaux locaux avec rotation et commande `mantlectl` ;
- Ethernet DHCP/IP statique, DNS et route vérifiés avant `MANTLE_NETWORK_OK` ;
- commandes Unix compilées depuis leurs sources selon le profil de compatibilité ;
- site de documentation local servi par `mantle docs`, sans CDN ni analytics.

### Expérimental

- langage Mantle Script `.mt` et séquences déclaratives `.mtc` ;
- paquets `.mtpkg`, clés de dépôt, révocation et rollback ;
- profils Personal, Education et Government comme base de politiques ;
- modèle de session console et premières interfaces du Design System ;
- architecture de la session Wayland et des applications natives.

### Prévu

- compositeur Wayland MantleOS avec DRM/KMS, input, fenêtres et espaces de travail ;
- Mantle Shell graphique, écran de connexion et premier démarrage ;
- Mantle Settings, Files, Terminal, Software, Monitor, Security et Update ;
- audio, Wi-Fi, Bluetooth, impression, chiffrement disque et installateur ;
- isolation d’applications, Secure Boot, TPM 2.0 et politiques renforcées ;
- SDK, débogueurs et compatibilité d’applications testée.

## Architecture

```text
UEFI → GRUB → noyau Linux MantleOS → initramfs MantleOS
     → mantle-init PID 1 → montage du rootfs → /sbin/init MantleOS
     → mantle-supervise → logs, réseau, session et applications
```

- **Boot** : GRUB UEFI charge le noyau, l’initramfs et le rootfs de l’ISO.
- **Kernel** : version Linux épinglée, compilée dans la chaîne MantleOS pour x86_64.
- **Libc/toolchain** : musl compilée dans un sysroot MantleOS ; les programmes du système ciblent ce sysroot.
- **Init** : l’initramfs monte les pseudo-filesystems, monte le rootfs réel puis exécute `/sbin/init`.
- **Userspace** : BusyBox fournit les primitives POSIX ; les composants MantleOS sont compilés séparément.
- **Services** : `mantle-supervise`, `mantle-logd`, `mantle-network` et `mantlectl` forment le socle administrable.
- **Shell** : `mantle-shell` possède son lexer, son parseur, ses pipelines, redirections et builtins ; dash reste `/bin/sh`.
- **Paquets** : `.mtpkg` sépare payload, métadonnées, manifeste et signature ; l’installation est transactionnelle.
- **Graphique** : la cible est une session Wayland MantleOS, sans Sway comme bureau final.

## Structure du dépôt

```text
boot/       splash et configuration GRUB UEFI
kernel/     configuration du noyau x86_64
init/       initramfs et premier PID 1
system/     init du rootfs, réseau et logs
services/   superviseur, mantlectl et gestionnaire mantle
shell/      moteur natif mantle-shell
scripting/  interpréteurs Mantle Script et Mantle Command
compat/     sources épinglées et builds des outils Unix/Linux
ui/         tokens du Design System et future session graphique
apps/       intégrations d’applications et MIME
site/       documentation locale statique hors ligne
tests/      validations shell, paquets, image, CI et QEMU
docs/       architecture, sécurité, build et formats MantleOS
build/      scripts de génération et artefacts locaux
```

## Build

### Linux

Le build nécessite un environnement Linux avec les dépendances de compilation,
GRUB/xorriso, ext4 et QEMU/OVMF pour le test. La chaîne actuelle ne copie pas
les binaires userspace d’une distribution hôte.

```sh
MANTLE_PROFILE=personal sh ./build.sh
```

Profils disponibles : `personal`, `education`, `government`.

```sh
MANTLE_PROFILE=personal MANTLE_COMPAT=core sh ./build.sh
MANTLE_PROFILE=personal MANTLE_COMPAT=full MANTLE_SDK=1 sh ./build.sh
```

`core` évite Python pour accélérer les itérations. `full` ajoute Python. Le
SDK ajoute GCC/C++, LLVM/Clang et CMake compilés depuis les sources.

### Windows

Le build Windows passe par Docker Desktop Linux :

```powershell
.\build.ps1 -Profile personal
```

Docker Desktop doit être installé et démarré. Le résultat se trouve dans
`build/out/`, pas dans la racine du dépôt.

### Sorties

```text
build/out/mantleos-amd64.iso
build/out/mantleos-amd64.iso.sha256
build/out/mantleos-root.ext4
build/out/mantleos-disk.img
build/out/mantle-initramfs.cpio.gz
build/out/build-info.txt
```

## Test en VM

La validation automatisée utilise QEMU en UEFI avec OVMF, une console série
dédiée et un disque root persistant :

```sh
sh ./tests/verify-image.sh
sh ./tests/qemu-boot.sh
```

Le test privilégie KVM quand `/dev/kvm` est disponible et utilise TCG sinon.
Il échoue si la VM ne produit pas les marqueurs suivants avant le timeout :

```text
MANTLE_KERNEL_OK
MANTLE_INIT_OK
MANTLE_ROOTFS_OK
MANTLE_SERVICES_OK
MANTLE_NETWORK_OK
MANTLE_SHELL_OK
MANTLE_MT_OK
MANTLE_MTC_OK
```

Lorsque la CI réussit, l’ISO, sa somme, `build-info.txt` et le log série sont
disponibles dans les artefacts du workflow GitHub Actions `MantleOS build`.

## Scripts MantleOS : `.mt`

`.mt` est un langage Mantle Script interprété par `mantle-script`. Il ne s’agit
pas d’un fichier shell renommé : les commandes système doivent être explicites
avec `run` et sont exécutées directement sans concaténation dans un shell.

```mt
#!/usr/bin/mantle
let project = "mantle-test"
print "Installation de ${project}"
if command.exists("git") {
    run git --version
}
require admin {
    run mantle update
}
```

La grammaire actuellement implémentée couvre variables texte, interpolation,
`print`, `run`, `if command.exists`, `require admin` et `exit`. Les fonctions,
boucles, imports, pipes et redirections restent explicitement non implémentés.
Voir [docs/mantle-script.md](docs/mantle-script.md).

## Commandes MantleOS : `.mtc`

`.mtc` est volontairement plus simple : une commande directe par ligne, sans
variables, fonctions ni blocs.

```mtc
mantle update
mantle upgrade
mantle clean
```

```sh
mantle run install.mt
mantle exec update.mtc
```

Les fichiers téléchargés ne deviennent pas exécutables automatiquement.

## Gestion des paquets

`.mtpkg` est le format expérimental de paquet MantleOS. Il contient un payload,
des métadonnées, un manifeste SHA-256, un identifiant de clé, un document signé
et une signature OpenSSL. Les clés publiques approuvées sont locales et les
clés révoquées sont refusées.

```sh
mantle verify package.mtpkg
sudo mantle install package.mtpkg
sudo mantle remove package-name
```

Les chemins absolus, traversals, liens dangereux, fichiers critiques et
permissions privilégiées sont refusés. L’installation utilise une staging area,
des sauvegardes et un rollback best-effort. Aucun catalogue distant n’est
présenté comme disponible sans dépôt configuré. Voir
[docs/mantle-package.md](docs/mantle-package.md).

## Sécurité

La fondation applique déjà la séparation root/utilisateur, des builds statiques
durcis, la validation de chemins de paquets, la vérification cryptographique,
des journaux locaux et un réseau activé par service. Les protections futures
incluent Secure Boot, TPM 2.0, chiffrement complet, isolation d’applications,
AppArmor/SELinux, signatures de mises à jour et rollback vérifié.

Ces mécanismes ne constituent pas une certification. Leur niveau réel doit être
mesuré par des tests, une revue de code et des audits indépendants.

## Confidentialité

Le système ne contient pas de télémétrie obligatoire, de publicité, de profilage
publicitaire, d’identifiant publicitaire, de compte en ligne obligatoire ou de
service cloud requis pour le socle. Les logs et diagnostics sont locaux. Toute
fonction de diagnostic distante future devra être facultative, minimisée,
documentée et désactivable.

## Profils

- **MantleOS Personal** : poste local quotidien, simplicité, confidentialité et compatibilité.
- **MantleOS Education** : base pour politiques d’établissement, comptes, déploiement et mode examen contrôlé.
- **MantleOS Government** : base pour dépôts internes, réseaux isolés, mises à jour hors ligne et durcissement renforcé.

Education et Government sont des profils d’architecture et de politiques en
développement. Ils ne sont pas homologués et ne doivent pas être présentés
comme tels.

## Contribution

Les contributions doivent préserver l’indépendance de la chaîne MantleOS,
l’absence de télémétrie obligatoire, la compatibilité Unix et la lisibilité du
code.

Avant une proposition :

1. lire [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) et la documentation concernée ;
2. ajouter un test adapté à tout nouveau composant ;
3. exécuter les contrôles statiques disponibles ;
4. ne pas annoncer un boot ou une fonctionnalité sans test réel ;
5. documenter les dépendances, licences et limites matérielles.

## Licence

Le code original MantleOS est distribué sous [licence MIT](LICENSE). Les
composants tiers conservent leurs licences respectives ; leurs sources et
versions sont référencées dans `compat/sources.lock` et la documentation de
build.

## Avertissement

MantleOS est encore en développement. L’ISO, la session graphique,
l’installateur, la gestion des paquets et les protections de production ne sont
pas au niveau d’un système audité. N’utilisez pas MantleOS dans un
environnement critique ou avec des données irremplaçables tant que le projet
n’a pas été stabilisé, testé sur le matériel cible et audité.
