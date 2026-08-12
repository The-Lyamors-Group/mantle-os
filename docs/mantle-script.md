# Mantle Script (`.mt`)

Mantle Script est interprété par `/usr/bin/mantle-script`. Il ne transforme pas le fichier en une chaîne passée à un shell : les commandes `run` sont tokenisées en argv puis lancées directement par `execvp`.

## Grammaire réellement implémentée

```ebnf
file        = { statement } ;
statement   = let | print | run | if_exists | require_admin | exit ;
let         = "let" identifier "=" ( string | word ) newline ;
print       = "print" ( string | word ) newline ;
run         = "run" word { word | string } newline ;
if_exists   = "if command.exists" "(" ( string | word ) ")" block ;
require_admin = "require admin" block ;
exit        = "exit" word newline ;
block       = "{" { statement } "}" ;
```

Les chaînes peuvent contenir `${nom}`. `let` crée une variable de texte ; `print` l’interpole ; `$0`, `$1`, etc. lisent les arguments passés au script. `command.exists("git")` vérifie le `PATH`. `require admin` n’exécute son bloc que si l’UID effectif est zéro.

Les commandes système doivent être précédées de `run` :

```mt
let project = "mantle-test"
print "Installation de ${project}"
if command.exists("git") {
    run git clone https://example.org/project.git
}
require admin {
    run mantle install curl
}
```

La première implémentation ne prend pas encore en charge les fonctions, les boucles, les imports, les pipes ou les redirections dans `.mt`. Ces constructions sont des erreurs explicites, pas des conversions silencieuses vers POSIX shell. Pour ces besoins, utiliser `sh script.sh` ou `mantle-shell -c`.

## `.mtc`

`.mtc` est interprété par `/usr/bin/mantle-command`. Chaque ligne non vide et non commentée est une commande séparée, exécutée dans l’ordre. Il n’y a ni variables, ni blocs, ni fonctions :

```mtc
mantle update
mantle upgrade
mantle clean
```

L’exécution est explicite avec `mantle run file.mt` ou `mantle exec file.mtc`. Un fichier téléchargé n’est pas rendu exécutable par MantleOS.
