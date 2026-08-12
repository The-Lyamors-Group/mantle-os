# Services MantleOS

`mantle-supervise` est le premier superviseur MantleOS. Il est lancé en PID 1 après l’initialisation des pseudo-systèmes et garde la console utilisateur disponible. Son interface sera étendue vers des unités déclaratives avec dépendances, limites de ressources et redémarrage borné ; systemd n’est pas utilisé.
