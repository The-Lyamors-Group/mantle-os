# Root filesystem MantleOS

Ce document décrit une ancienne chaîne Linux, désormais désactivée. Aucun
root filesystem n’est construit par l’image noyau indépendante actuelle.

Les artefacts ext4, disque persistant et initramfs ne sont plus produits.

La prochaine étape est de définir l’ABI mémoire et le modèle de processus
MantleOS ; aucun `switch_root` n’existe dans l’image actuelle.
