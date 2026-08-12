# Sécurité

Le noyau active ASLR, PIE possible pour les binaires MantleOS, stack protector, W^X strict, namespaces, seccomp, AppArmor et la signature des modules comme cibles de durcissement. La première image ne revendique aucune certification.

Le PID 1 est compilé statiquement avec les protections de compilation disponibles. Les prochaines étapes de sécurité sont la vérification de l’initramfs, la chaîne Secure Boot, la séparation utilisateur/admin et les tests de sandbox.
