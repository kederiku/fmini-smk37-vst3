# Architecture

[Accueil](../../README.fr.md) · [English](../en/architecture.md)

Le WASM d'origine est converti en C par wasm2c puis compilé dans le plugin. Cela conserve le DSP, l'interface du firmware et son stockage émulé sans intégrer de navigateur. Chaque `Engine` possède sa mémoire linéaire et ses variables modifiables ; les instances ne partagent pas leur état musical.

| Fichiers | Rôle |
| --- | --- |
| `src/Engine.*` | Démarrage, fonctions du firmware, rendu par blocs bornés, commandes, écran/LED et état versionné. |
| `src/Audio.*` | Adaptation des 44,1 kHz du moteur vers la fréquence de l'hôte avec un filtre sinc persistant de 16 coefficients et 1 024 phases. |
| `src/Plugin.*` | Factory/composant/contrôleur VST3, MIDI, automation, traitement et flux d'état de l'hôte. |
| `src/Editor.*`, `src/EditorPaint.cpp` | Entrées et capture Win32, dessin du panneau GDI+. |
| `src/Panel.*` | Géométrie et détection des clics ; priorité aux touches noires. |
| `src/Bytes.h` | Encodage little-endian, lecture bornée et somme de contrôle. |
| `third_party/generated` | C et en-tête générés : à régénérer plutôt qu'à modifier à la main. |

## Propriété des threads

Le callback audio possède normalement le moteur. Les producteurs de l'interface sérialisent leurs écritures dans une file bornée ; le consommateur n'utilise pas leur mutex. Les événements de l'hôte sont triés par position d'échantillon et appliqués entre les segments audio. Les points d'automation du gain sont interpolés. Un débordement demande un arrêt des notes pour éviter qu'elles restent tenues.

L'affichage utilise un verrou séparé. Le thread audio l'essaie sans attendre et saute une mise à jour si nécessaire. Le remplacement d'état utilise le verrou du moteur ; s'il est occupé, le traitement émet du silence et reporte les événements de l'hôte pour conserver les relâchements de notes.

Pour sauvegarder, le thread appelant alloue un autre moteur, demande une copie en fin de bloc audio, puis compresse et écrit cette copie hors du callback. Un hôte inactif utilise une solution de repli protégée après 200 ms. La copie de 32 Mo reste une opération sur le thread audio ; le test vérifie la conservation des événements, pas le respect de chaque échéance sur tous les périphériques.

## Compatibilité des états

L'état externe contient version, finition, paramètres et données du moteur. L'état interne contient identifiant/version, empreinte exacte du WASM, taille et variables de mémoire, somme de contrôle, boutons en attente et mémoire compressée par plages de zéros. Les pointeurs natifs et tables de fonctions ne sont jamais sérialisés. Le rappel valide les données avant de remplacer le moteur, puis relâche les notes tenues.

La somme de contrôle détecte les corruptions accidentelles ; elle n'authentifie pas les données. L'identifiant de classe VST3 et les identifiants des paramètres doivent rester stables pour préserver les projets des hôtes. `VERSION` détermine la version du plugin et de la release ; elle est distincte de la version du format d'état et de l'empreinte du moteur.
