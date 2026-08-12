# Compatibilité Unix/Linux

Cette arborescence contient des travaux userspace historiques et des profils
préparatoires. Elle n’est pas appelée par `build.sh`, `build/make-image.sh` ou
la CI de l’image noyau indépendante.

En particulier, aucun BusyBox, dash, musl, noyau Linux ou binaire de
distribution n’est compilé, embarqué ou démarré par l’ISO actuelle. Ces
sources ne seront réactivées qu’après l’implémentation de la libc, de l’ABI et
des appels système MantleOS.
