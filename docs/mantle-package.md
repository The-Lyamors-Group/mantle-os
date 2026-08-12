# Format `.mtpkg`

Un paquet MantleOS est une archive `tar` compressée avec gzip. Il ne peut être
installé que si son contenu, ses chemins, son manifeste et sa signature sont
valides.

```text
META/metadata.json
META/NAME
META/MANIFEST
META/KEY-ID
META/SIGNED
META/SIGNATURE
payload/<fichiers installés>
```

`metadata.json` contient au minimum `name`, `version`, `architecture`,
`dependencies`, `description`, `license`, `maintainer` et `build`. `NAME` doit
correspondre au nom sûr utilisé par la base locale des paquets.

Chaque ligne de `MANIFEST` est une somme SHA-256 suivie d’un chemin relatif :

```text
<64 caractères hexadécimaux>  payload/usr/bin/exemple
```

Les entrées doivent rester sous `payload/`. Les chemins absolus, `..`, liens
symboliques, liens matériels et fichiers critiques du système sont refusés.
Les fichiers sont copiés individuellement après validation, afin qu’une
archive ne puisse pas déposer silencieusement un fichier arbitraire.

`SIGNED` est la représentation canonique signée des métadonnées et du
manifeste. `SIGNATURE` est une signature OpenSSL `dgst -sha256` de ce fichier.
`KEY-ID` sélectionne `/etc/mantleos/trust/keys/<KEY-ID>.pub`. Une clé présente
dans `/etc/mantleos/trust/revoked` est toujours refusée.

Le répertoire de confiance est administré localement. La rotation consiste à
installer une nouvelle clé publique approuvée, publier les paquets signés avec
son nouvel identifiant, puis révoquer l’ancienne clé. Les clés privées de
production ne sont jamais incluses dans MantleOS.

L’installation crée une transaction dans
`/var/lib/mantleos/transactions/<pid>` : les fichiers remplacés sont sauvegardés,
le manifeste est installé, puis la transaction est marquée `COMMITTED`. En cas
d’erreur, les sauvegardes sont restaurées. Une transaction conservée peut être
restaurée avec `mantle rollback <id>`.
