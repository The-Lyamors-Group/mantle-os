# MantleOS

MantleOS est un système d’exploitation français open source qui cherche à
offrir un ordinateur plus privé, plus compréhensible et plus souverain.

Le projet est axé sur la confidentialité, la sécurité, la compatibilité, la
simplicité et les standards ouverts. MantleOS est indépendant de Linux comme
noyau. Il est encore expérimental.

> [!IMPORTANT]
> **MantleOS®** est un projet de [The Lyamors Group®](https://github.com/The-Lyamors-Group).

---

> [!WARNING]
> MantleOS est actuellement en développement. Il n’est ni certifié ni homologué par l’ANSSI, la DGSI ou l’État français. Ne l’utilisez pas comme système principal ou dans un environnement critique à ce stade.

## Télécharger MantleOS

### Je débute, que dois-je télécharger ?

Pour un PC Intel ou AMD classique en 64 bits, le fichier à télécharger sera :

**mantleos-amd64.iso**

Une ISO est un fichier qui contient les fichiers nécessaires pour démarrer un
système depuis une machine virtuelle ou une clé USB. Une version publique n’est
pas encore disponible : le dernier build doit d’abord passer la validation de
boot UEFI dans QEMU.

**Aucune version publique n’est encore disponible.**

Vous pouvez [voir les builds de développement](https://github.com/The-Lyamors-Group/mantle-os/actions/workflows/mantleos-build.yml), qui sont expérimentaux et ne doivent pas être confondus avec une version stable.

Quand une version sera validée, elle sera publiée dans les
[Releases MantleOS](https://github.com/The-Lyamors-Group/mantle-os/releases) avec
les fichiers suivants :

- mantleos-amd64.iso ;
- mantleos-amd64.iso.sha256 ;
- mantleos-build-info.txt.

À côté de chaque ISO publiée, son empreinte SHA-256 permettra de vérifier que
le téléchargement n’a pas été modifié.

Windows PowerShell :

~~~powershell
Get-FileHash .\mantleos-amd64.iso -Algorithm SHA256
~~~

Linux :

~~~bash
sha256sum mantleos-amd64.iso
~~~

## Essayer MantleOS sans l’installer

Le moyen prévu pour essayer MantleOS est une machine virtuelle : un ordinateur
simulé dans une fenêtre, qui ne modifie pas votre disque principal.

QEMU est l’outil utilisé par la CI du projet pour démarrer l’ISO avec un
firmware UEFI. La commande de test est documentée pour les développeurs plus
bas. Le dernier résultat public connu a validé la compilation et la génération
de l’ISO, mais le boot QEMU doit être revalidé après les corrections de
diagnostic en cours.

Aucun autre hyperviseur n’est déclaré testé à ce stade.

## Installer MantleOS

L’installateur graphique n’est pas encore fonctionnel. MantleOS ne fournit donc
pas aujourd’hui de procédure d’installation recommandée sur un disque réel.

N’écrivez pas une ISO expérimentale sur un disque contenant des données utiles.

## Configuration requise

| Élément | Minimum théorique | Configuration réellement testée |
| --- | --- | --- |
| Architecture | x86_64, processeur Intel/AMD 64 bits | x86_64 dans QEMU |
| Mémoire | 256 Mo pour le noyau minimal | 256 Mo configurés dans QEMU |
| Stockage | Aucun disque pour l’image actuelle | Aucun disque invité utilisé |
| Firmware | UEFI avec GRUB | OVMF dans QEMU, validation en cours |
| CPU | x86_64 avec mode long | CPU virtuel QEMU x86_64 |
| Virtualisation | Non nécessaire avec TCG ; KVM accélère Linux | TCG prévu sans KVM |

Ces valeurs concernent le noyau minimal actuel, pas un futur système de bureau.

## État du projet

| Composant | État réel |
| --- | --- |
| Build du kernel MantleOS | Compilé et linké dans la CI |
| Image ISO | Générée par la CI |
| Boot UEFI | Expérimental, validation QEMU en cours |
| Kernel x86_64 | Minimal, affiche un marqueur série puis reste stable |
| Console VGA/série | Expérimentale |
| Mantle Shell | Non embarqué dans l’image actuelle |
| Réseau | Non disponible dans l’image actuelle |
| Interface graphique | En développement, non embarquée |
| Installateur | Non disponible |
| Gestionnaire de paquets | Non disponible dans l’image actuelle |
| Utilisation quotidienne | Non recommandée |

Les termes « compilé » et « généré » ne signifient pas qu’une fonction est
stable. Une fonction n’est annoncée comme validée qu’après le test correspondant
dans la CI.

## Pour les utilisateurs Unix/Linux

Le dépôt conserve les travaux préparatoires d’un futur userspace MantleOS. Les
commandes curl, Git, SSH, les outils Unix, les scripts .mt, les commandes .mtc
et les paquets .mtpkg ne sont pas disponibles dans l’image noyau actuelle.
Ils seront reconnectés après l’implémentation de la libc, de l’ABI et des appels
système MantleOS.

## Pour les développeurs

### Récupérer le dépôt

~~~bash
git clone https://github.com/The-Lyamors-Group/mantle-os.git
cd mantle-os
~~~

### Construire

Sur Linux, avec GCC, binutils, Make, GRUB, xorriso et mtools :

~~~bash
MANTLE_PROFILE=personal sh ./build.sh
~~~

Sous Windows, Docker Desktop Linux est nécessaire :

~~~powershell
.\build.ps1 -Profile personal
~~~

Le résultat attendu est :

~~~text
build/out/mantleos-amd64.iso
build/out/mantleos-amd64.iso.sha256
build/out/mantleos-build-info.txt
build/out/build.log
~~~

### Vérifier et tester

~~~bash
sh ./tests/verify-image.sh
sh ./tests/qemu-boot.sh
~~~

Le test QEMU utilise OVMF, un firmware UEFI libre, et attend le marqueur
MANTLE_KERNEL_OK sur la sortie série. Il utilise KVM si disponible, sinon TCG,
un mode d’émulation sans accélération matérielle.

### CI et Releases

GitHub Actions compile le kernel depuis ce dépôt, crée l’ISO, vérifie son hash
et tente le boot UEFI dans QEMU. Les builds de branches restent des builds de
développement. Une Release n’est créée que lorsqu’un tag v* est poussé et que
toutes les étapes de compilation, vérification et boot ont réussi.

Exemple :

~~~bash
git tag v0.1.0-alpha
git push origin v0.1.0-alpha
~~~

Cette commande ne doit être utilisée qu’après revue du code et validation du
résultat attendu. Une Release publiée n’est pas automatiquement une version
stable.

### Architecture interne

~~~text
UEFI → GRUB → Multiboot2 → kernel/arch/x86_64 → VGA + COM1
~~~

Le noyau actuel initialise le mode long x86_64, une table de pages minimale,
une GDT, la console VGA et le port série. Il n’utilise ni noyau Linux, ni libc,
ni rootfs, ni initramfs. Les répertoires memory/, interrupts/, drivers/,
syscall/ et scheduler/ sont préparés pour les prochaines étapes.

Voir docs/ARCHITECTURE.md et docs/BUILDING.md pour les détails techniques.

## Sécurité et confidentialité

Le socle ne contient pas de télémétrie obligatoire, de publicité, de tracking,
de compte cloud obligatoire ou d’analytics intégrés. Cela ne constitue pas une
promesse absolue de sécurité : les protections de production, le chiffrement,
Secure Boot, les permissions et l’isolation restent à implémenter et à auditer.

## Licence

Le code original MantleOS est distribué sous licence MIT. Cela signifie que le
code peut être utilisé, étudié, modifié et redistribué selon les conditions
simples précisées dans LICENSE. Les composants tiers conservent leurs licences
respectives.

## Avertissement

MantleOS est actuellement un projet expérimental. L’ISO peut être générée sans
être encore validée comme système amorçable. Ne l’installez pas sur votre
ordinateur principal et ne l’utilisez pas avec des données irremplaçables.
