# Compatibilité Unix/Linux

Le profil `full` compile depuis les sources musl, dash, zlib, OpenSSL, curl, Git, rsync, sudo, OpenSSH, GNU Make, pkg-config et Python. Le SDK optionnel (`MANTLE_SDK=1`) ajoute GCC/C++, CMake et LLVM/Clang depuis les sources. Le profil `core` omet Python pour accélérer les itérations.

BusyBox fournit les commandes POSIX de base et `wget` jusqu’à l’intégration d’un client dédié. Ce sont des implémentations réelles. Aucun nom de commande n’est un script simulant un outil absent.

`/bin/sh` est dash 0.5.12 compilé depuis ses sources. `mantle-shell` est maintenant le moteur C MantleOS séparé, avec son propre lexer, parseur de pipelines, redirections, variables et exécution directe. `mantle` ajoute l’interprétation des extensions MantleOS :

- `.mt` : langage Mantle Script interprété par `mantle-script`, avec `let`, `print`, blocs conditionnels et commandes `run` exécutées directement ;
- `.mtc` : séquence de commandes sans construction de langage supplémentaire ;
- `mantle run script.mt` et `mantle exec commands.mtc` : autorisation explicite ;
- un dépôt cloné par `mantle source` voit ses fichiers `.mt`/`.mtc` privés du bit exécutable.

Les paquets `.mtpkg` sont des archives tar gzip contenant des métadonnées,
`META/MANIFEST`, `META/SIGNED`, `META/SIGNATURE`, `META/KEY-ID` et `payload/`.
Le manifeste est vérifié par SHA-256 puis les métadonnées sont authentifiées par
une clé publique approuvée dans `/etc/mantleos/trust/keys`. Les chemins
dangereux et les liens sont refusés, et l’installation utilise une transaction
avec rollback. En l’absence de clé de confiance, l’installation est refusée.

Exemples de test dans `tests/fixtures/` : `mantle run tests/fixtures/hello.mt` et `mantle exec tests/fixtures/update.mtc`.

`mantle docs` sert la documentation locale embarquée avec le serveur HTTP BusyBox sur `127.0.0.1:8080`. Ce service est facultatif et n’est pas démarré automatiquement.
