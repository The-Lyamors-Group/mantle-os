# Mantle init

`mantle-init` est le PID 1 de la première image MantleOS. Il monte les pseudo-systèmes nécessaires, prépare `/run` et `/tmp`, branche la console du noyau et lance le shell utilisateur. Les services et la supervision seront ajoutés derrière cette frontière, sans dépendance à systemd.
