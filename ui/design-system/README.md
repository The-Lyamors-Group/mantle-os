# Design System MantleOS

Le Design System définit les primitives communes de la future session graphique
MantleOS et de ses applications. Il privilégie une hiérarchie compacte,
des surfaces lisibles, des états explicites et un focus visible.

`tokens.css` contient les valeurs primitives et sémantiques : couleurs, modes
clair/sombre, typographie, espacements, rayons, élévation, focus et mouvement.
`components.css` expose les premiers composants de référence pour les prototypes
et la documentation hors ligne. Une application native devra reprendre les
mêmes valeurs, états et règles d’accessibilité ; elle ne doit pas inventer son
propre thème.

La direction d’interface retenue pour Mantle Shell est une composition dense et
calme : barre système discrète, panneau de lancement orienté recherche,
notifications lisibles et surfaces sans effet de verre décoratif. Le panneau
latéral sert à la navigation et aux espaces de travail ; il ne remplace pas la
gestion réelle des fenêtres du compositeur.
