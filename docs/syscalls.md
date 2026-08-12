# ABI syscall MantleOS

La première ABI userspace est spécifique à MantleOS et utilise l'instruction
x86_64 `syscall`. Les numéros et conventions ci-dessous sont ceux réellement
utilisés par les programmes de `userspace/`.

Les arguments suivent l'ABI System V x86_64 : `rdi`, `rsi`, `rdx`, `r10`, `r8`,
`r9`. Le numéro est dans `rax` et le résultat signé revient dans `rax`. Une
erreur renvoie une valeur négative. Les pointeurs userspace sont acceptés
uniquement dans la fenêtre actuellement mappée `0x400000..0x600000`.

| Numéro | Nom | Arguments | État |
| ---: | --- | --- | --- |
| 0 | `read` | fd, buffer, taille | clavier PS/2 pour fd 0 |
| 1 | `write` | fd, buffer, taille | console pour fd 1 et 2 |
| 2 | `open` | chemin | fichiers du module rootfs |
| 3 | `close` | fd | descripteurs rootfs |
| 6 | `exec` | chemin | ELF64 `PT_LOAD` |
| 8 | `getpid` | — | PID courant |
| 9 | `chdir` | chemin | cwd du processus courant |
| 10 | `getcwd` | buffer, taille | cwd du processus courant |
| 11 | `exit` | code | arrêt du processus courant |
| 12 | `reboot` | — | contrôleur clavier, expérimental |
| 13 | `readdir` | chemin, buffer, taille | entrées du rootfs monté |
| 14 | `uname` | buffer, taille | identité du noyau MantleOS |

`stat`, `fstat`, `lseek`, `wait`, `mmap` et `brk` ne sont pas annoncés comme
disponibles tant qu'ils n'ont pas une implémentation complète. Le modèle de
processus est encore mono-processus : `exec` remplace l'image courante et le
scheduler round-robin viendra après le premier shell console.

Le passage initial vers ring 3 utilise une GDT avec segments utilisateur,
un TSS et `iretq`. Les appels userspace utilisent une entrée `syscall` dédiée,
une pile kernel séparée et une validation systématique des adresses.
