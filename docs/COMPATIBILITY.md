# Compatibilité Unix/Linux

La compatibilité Unix/Linux est un objectif userspace, pas le noyau de
MantleOS. Les profils et outils décrits par les anciens documents ne sont pas
encore disponibles dans l’image noyau indépendante.

La priorité est d’implémenter la libc MantleOS, son ABI, les appels système,
les processus et les permissions. Les outils classiques pourront ensuite être
portés ou compilés depuis leurs sources sans devenir une distribution Linux.
