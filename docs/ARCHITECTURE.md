# Architecture MantleOS

MantleOS est construit autour d’un noyau Linux compilé depuis les sources, utilisé comme couche matérielle de compatibilité et non comme userspace distribué. Le userspace initial est produit dans `build/make-image.sh` à partir de BusyBox compilé dans le même environnement et de `init/mantle-init.c`, le PID 1 MantleOS.

```text
UEFI → GRUB configuré par MantleOS → Linux MantleOS → initramfs MantleOS
     → mantle-init (PID 1) → pseudo-filesystems → console MantleOS
```

Les composants graphiques, services, sécurité et applications seront ajoutés derrière les interfaces `init/`, `services/`, `security/`, `desktop/` et `apps/`. Aucun fichier de `config/` de l’ancienne image live n’est utilisé par la chaîne actuelle.

Le profil de build est inscrit dans `/etc/mantleos/profile` afin que les couches suivantes puissent activer des politiques sans dupliquer l’image de base.
