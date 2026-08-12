# Userspace MantleOS

Le userspace actif est construit par `userspace/Makefile` sans libc de
distribution, puis regroupé dans un module rootfs MantleOS par
`build-rootfs.py`. La première session contient `/sbin/init`, `/bin/hello`,
`/bin/mantle` et `/bin/mantle-shell`.

`/sbin/init` est chargé par le chargeur ELF64, exécuté en ring 3 et lance le
shell par `exec`. Le shell utilise les syscalls MantleOS pour la console,
`pwd`, `ls`, `cd /`, `echo MantleOS`, `uname`, `mantle --version` et
`/bin/hello`. L'entrée clavier initiale est le contrôleur PS/2 en mode polling.

Les anciens composants C conservés dans `init/`, `system/`, `services/` et
`shell/` ne sont pas utilisés par cette chaîne.
