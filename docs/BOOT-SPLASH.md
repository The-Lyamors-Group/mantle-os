# Splash MantleOS

La vidéo de référence conservée dans `public/statics/medias/boot-screen.mov` est en 1920×1080, H.264, 30 i/s, 4,783 s. Elle présente le symbole MantleOS et le mot-symbole centrés, avec une transition progressive d’un fond sombre vers un fond clair.

Le boot ne lit pas cette vidéo. L’ancien renderer `boot/splash/mantle-splash.c`
visait un framebuffer Linux et n’est pas compilé par la chaîne noyau actuelle.
Le splash sera réimplémenté après les pilotes vidéo et le mode graphique
MantleOS.
