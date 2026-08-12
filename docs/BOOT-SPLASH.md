# Splash MantleOS

La vidéo de référence conservée dans `public/statics/medias/boot-screen.mov` est en 1920×1080, H.264, 30 i/s, 4,783 s. Elle présente le symbole MantleOS et le mot-symbole centrés, avec une transition progressive d’un fond sombre vers un fond clair.

Le boot ne lit pas cette vidéo. `boot/splash/mantle-splash.c` rend deux scènes PPM dans le framebuffer Linux, interpole les pixels pendant 150 images et signale la fin à `/run/mantle-splash.done`. Les PPM sont dérivés des SVG MantleOS pendant le build.
