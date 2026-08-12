# Root filesystem MantleOS

Le root filesystem est construit dans `build/make-image.sh` à partir de BusyBox compilé statiquement et des composants C MantleOS compilés avec musl 1.2.5 installé dans le sysroot du build.

`build/out/mantleos-root.ext4` est le rootfs de référence. `build/out/mantleos-disk.img` est une copie destinée à un disque persistant de test. L’initramfs contient uniquement les outils de découverte, `mantle-init`, le splash et les scènes du splash.

Au démarrage, `mantle-init` monte les pseudo-filesystems et cherche `mantle.root=`. Avec `/dev/vda`, le rootfs ext4 est monté en lecture-écriture. Sans disque, il tente l’image `boot/rootfs.ext4` du média ISO en lecture seule, puis conserve le mode récupération si le média n’est pas disponible.
