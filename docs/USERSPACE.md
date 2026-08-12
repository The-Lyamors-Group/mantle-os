# Userspace initial

Le userspace initial comprend BusyBox pour le shell et les outils POSIX, musl comme libc explicite, `mantle-init` comme PID 1 après transition, `mantle-supervise` pour les services, `mantlectl` pour l’administration, `mantle-network` pour DHCP/IP statique et `mantle-logd` pour les journaux locaux avec rotation à 1 MiB.

Le compte `mantle` possède l’UID/GID 1000 et son home persistant. Les entrées shadow sont verrouillées par défaut (`!`) : aucun mot de passe n’est livré dans l’image. Le changement de mot de passe utilise l’applet `passwd` BusyBox et écrit un hash dans `/etc/shadow` ; l’authentification interactive et la politique PAM restent à intégrer avant une déclaration de login final.
