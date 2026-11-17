# Vérification du portage — 17 novembre 2026

[English](verification.en.md) · [Accueil](../README.fr.md)

Version 0.1.0, Windows x86-64. Commande locale : `scripts/build.ps1`, code de sortie 0. Les nouvelles exécutions GitHub Actions conservent leur journal dans l'artefact `native-test-log`. Une vérification locale du workflow ne constitue pas une exécution hébergée sur GitHub.

## Résultats automatisés

| Vérification | Résultat observé |
| --- | --- |
| Module original exécuté dans Node | Aucun import ; 88 200 trames stéréo, pic 0,170440673828125. |
| Rendu natif face au module original | Erreur maximale 0 sur les deux canaux, après passage du démarrage du firmware. |
| Isolation de deux moteurs / plugins | Instance fraîche silencieuse : pic 0. |
| État vivant du moteur | 1 602 672 octets pour le scénario testé ; rendu identique après rappel. |
| Appui OCT+ suivi d'une sauvegarde immédiate | Geste en attente conservé après rappel. |
| État tronqué / altéré | Rejet ; état courant conservé. |
| Audio 44,1 / 48 / 88,2 / 96 kHz | Résultats identiques entre rendu continu et blocs de 1, 31, 128, 511, 2 048 et 4 096 trames ; fin de relâchement silencieuse. |
| Chargement réel du VST3 | Exports, factory, interfaces, bus stéréo et MIDI, cycle de vie, notes horodatées et paramètres : succès. |
| Double précision | Refus explicite ; traitement proposé en 32 bits flottants. |
| État VST3 | 1 504 480 octets dans le scénario testé ; rappel des paramètres et rejet d'état tronqué. |
| Automation OUTPUT | Interpolation de la rampe vérifiée par comparaison d'échantillons. |
| Canaux MIDI | Mappings distincts ; sustain canal 2 maintenu, pédale du canal 1 sans effet sur lui, relâchement par sa propre pédale. |
| Appel sans audio | Événements et paramètres acceptés dans un appel de zéro trame sans sortie. |
| Traitement hors ligne | Initialisation et rendu réussis, sans fenêtre d'éditeur. |
| Sauvegarde pendant le jeu | Pas de bloc artificiellement silencieux dans le scénario, note-off conservé, aucune note tenue à la fin. |
| Géométrie du panneau | Nombre et accessibilité des contrôles, priorité des touches noires : succès. |

Mesure locale indicative : six notes, 48 kHz, 256 blocs de 4 096 trames, 21,845 s d'audio calculées en 0,241 s (environ 90,66 fois le temps réel). Cette mesure hors ligne ne mesure ni la latence des périphériques ni les échéances de chaque callback en situation réelle. La capture d'une sauvegarde copie une fois les 32 Mo de mémoire du moteur ; sa compression est faite hors du traitement audio.

Le plugin importe uniquement des bibliothèques Windows (USER32, GDI+, KERNEL32 et UCRT). Il n'utilise ni navigateur ni réseau à l'exécution.

## Essai dans SAVIHost

L'exécutable SAVIHost VST3 x64 a chargé le plugin. L'utilisateur a confirmé que le panneau est visible, que le clavier de l'ordinateur produit du son et que les touches jouées à la souris fonctionnent également. Cette observation porte sur la première compilation ; les corrections suivantes concernent la sauvegarde, l'automation, le routage MIDI et le passage du démarrage du firmware. Le paquet final utilise la nouvelle compilation, vérifiée par les tests ci-dessus.

L'accès automatique à l'image de la fenêtre SAVIHost a expiré ; aucune capture du panneau natif n'a été obtenue. Les boutons, encodeurs, faders, menus et séquences n'ont pas tous été essayés manuellement. L'apparence a été reproduite à partir de la page d'origine, avec des tests de géométrie ; aucune comparaison visuelle pixel à pixel n'est revendiquée.

Reason n'est pas installé et n'a pas été testé. La synchronisation du tempo/position hôte, la sortie MIDI, le SysEx et les connexions matérielles restent hors de cette version.

## Relecture et corrections

Une relecture indépendante a identifié la contention pendant la sauvegarde, l'oubli des gestes de boutons en attente, l'automation continue traitée par sauts et la perte des canaux MIDI. Ces points ont été corrigés. Des tests ont reproduit les trois premières régressions avant correction ; les tests finaux couvrent aussi le routage des canaux et la pédale de sustain.

Les dépendances et révisions sont répertoriées dans `THIRD_PARTY_NOTICES.md`. Les empreintes des fichiers distribués sont enregistrées dans le manifeste produit par `scripts/package.ps1`.
