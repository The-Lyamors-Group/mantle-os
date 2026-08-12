# Mantle Shell

`mantle-shell` est un shell MantleOS minimal écrit en C. Il possède son propre lexer, parseur de commandes et exécuteur : mots quotés, expansion `${VAR}`, built-ins `cd`, `export`, `unset`, `pwd`, `true`, `false`, `exit`, pipelines et redirections `>`, `>>`, `<` sont exécutés sans fabriquer une chaîne passée à `system()`.

Dash reste disponible comme `/bin/sh` POSIX. Il n’est pas renommé en `mantle-shell`.
