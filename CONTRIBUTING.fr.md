# Contribuer

[English](CONTRIBUTING.md) · [Compilation](docs/fr/compilation.md)

Les tickets et pull requests sont acceptés en français ou en anglais. Pour signaler un bug, préciser la version de Windows, l'hôte et sa version, la version du plugin, la fréquence audio, la taille du tampon, les étapes et le résultat attendu/obtenu. Joindre un preset minimal seulement s'il peut être partagé.

Écrire les commentaires du code en anglais et actualiser les deux langues de documentation lorsque le comportement change. Expliquer les règles de propriété, de temps et de format plutôt que les instructions évidentes. Ne pas modifier à la main le code généré ni les dépendances sans noter leur révision et leur licence.

Exécuter `scripts/build.ps1` pour le code et `node tests/docs_test.mjs` pour la documentation. Pour les paquets ou la CI, exécuter aussi `scripts/package.ps1`, `scripts/verify-package.ps1` et actionlint. Décrire les essais réalisés ; ne pas annoncer la compatibilité avec un hôte sans l'avoir essayé.

Préserver les identifiants VST3 et tenir compte des sauvegardes existantes. Ajouter une couverture de régression pour les changements du DSP, des événements ou du chargement d'état. Les contributions au portage utilisent GPL-3.0-only ; conserver les mentions des composants tiers.
