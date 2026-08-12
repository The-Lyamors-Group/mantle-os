# Mantle Compositor

Ce répertoire est réservé au compositeur Wayland propre à MantleOS. Il ne
réintroduit pas Sway comme bureau final.

Le premier jalon d’implémentation devra utiliser une bibliothèque de bas niveau
Wayland/wlroots ou un équivalent auditable pour fournir DRM/KMS, sorties,
surfaces xdg-shell, clavier, pointeur, focus et espaces de travail. Tant que ce
composant n’est pas compilé et testé sur une machine Linux équipée de DRM ou
du backend headless, il n’est pas inclus dans l’ISO et n’est pas annoncé comme
fonctionnel.

Le contrat de session prévu est explicite : le compositeur possède la gestion
des fenêtres et publie l’état ; Mantle Shell possède le panneau système, le
lanceur, les notifications et les espaces de travail ; les applications ne
contournent pas le compositeur pour simuler des fenêtres.
