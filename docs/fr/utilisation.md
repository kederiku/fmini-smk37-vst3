# Guide utilisateur

[Accueil](../../README.fr.md) · [English](../en/usage.md)

## SAVIHost

1. Télécharger l'édition **VST3 x64** sur le [site officiel de SAVIHost](https://www.hermannseib.com/english/savihost.htm), puis extraire tout le paquet. Lire la licence incluse : l'hôte gratuit est réservé à l'usage privé.
2. Copier `VST3/FMini SMK-37.vst3/Contents/x86_64-win/FMini SMK-37.vst3` depuis l'archive du plugin vers le dossier SAVIHost. Il s'agit du fichier binaire intérieur, pas du dossier qui l'entoure.
3. Copier ou renommer `savihost.exe` en **`FMini SMK-37.exe`**, avec exactement le même nom de base que le fichier `.vst3`. Garder `license.txt` et `history.txt`.
4. Lancer `FMini SMK-37.exe`. Choisir la sortie audio dans **Devices > Wave**, puis si nécessaire l'entrée MIDI dans **Devices > MIDI**.

Un paquet local `-SAVIHost.zip` contient déjà ces fichiers. L'extraire avant de lancer l'exécutable. Les archives GitHub publiques n'incluent pas l'hôte.

## Commandes du panneau

| Commande | Geste |
| --- | --- |
| Touches / pads | Maintenir le clic gauche ; glisser vers une autre touche ou un autre pad. La hauteur du clic règle la vélocité. |
| Encodeurs | Glisser verticalement ou utiliser la molette. Maintenir Maj pendant le glissement pour un réglage plus fin. |
| Faders / molettes | Cliquer ou glisser. Le pitch revient au centre au relâchement. |
| Boutons | Maintenir le clic. Maj + clic verrouille un bouton pour les combinaisons ; un autre Maj + clic le libère. |
| Finition | Choisir une des six couleurs dans le menu en haut à droite. |
| Output | Le curseur supérieur règle le gain du plugin ; le centre correspond à 100 %. |
| Clavier de l'ordinateur | Cliquer dans le panneau puis utiliser A W S E D F T G Y H U J K O L P ;, selon la disposition du clavier. |
| Octave | Z / X. |
| Arrêt des notes | Échap lorsque le panneau a le focus. |

Le LCD est l'écran du firmware d'origine. Ses menus et son fonctionnement musical suivent [FMini-1](https://github.com/ruppert/fmini-1). L'animation de démarrage est calculée avant de rendre l'instrument disponible.

## MIDI et automation

Relier une piste MIDI ou un clavier à l'instrument. Le plugin conserve les canaux MIDI ; le routage choisi dans le firmware détermine les parties qui répondent. La pédale de sustain utilise CC64 ; pitch bend, modulation et pression de canal/polyphonique sont transmis. OUTPUT et Fader 1–4 sont disponibles pour l'automation ; d'autres paramètres masqués transportent les contrôleurs MIDI VST3.

L'arpégiateur et le séquenceur utilisent leur tempo interne. L'arrêt du transport relâche les notes, mais le tempo et la position du morceau ne sont pas synchronisés avec l'hôte. Les options USB, TRS et Bluetooth du firmware ne créent pas de connexion matérielle depuis le plugin.

## Sauvegarder et rappeler

Enregistrer un preset d'instrument ou un projet dans l'hôte. L'état conserve la mémoire du firmware, les réglages/séquences, les gestes de boutons en attente, le niveau de sortie, les faders et la finition. Une sauvegarde uniquement dans le firmware ne remplace pas celle du projet dans l'hôte. Le rappel relâche les notes tenues.

Conserver les presets avec la version du plugin qui les a créés. Les états invalides ou incompatibles avec le moteur sont refusés. Les instantanés de test occupent environ 1,5 Mo ; la taille dépend de l'état.

## Dépannage

| Symptôme | Vérifications |
| --- | --- |
| Aucun son | Périphérique audio, routage/canal MIDI, monitoring de la piste, niveaux MASTER et OUTPUT. Essayer une touche à la souris. |
| Le clavier de l'ordinateur ne joue pas | Cliquer dans le panneau. L'hôte peut intercepter les raccourcis ; essayer la souris ou un clavier MIDI. |
| Note bloquée | Appuyer sur Échap avec le focus dans le panneau ; vérifier la pédale et l'entrée MIDI. |
| SAVIHost ne charge pas le plugin | Utiliser SAVIHost VST3 x64, faire correspondre les noms et choisir le fichier `.vst3` intérieur. |
| Plugin absent du logiciel de musique | Installer le dossier complet, utiliser un hôte Windows 64 bits et relancer sa recherche VST3. |
| Craquements | Augmenter le tampon audio de l'hôte et vérifier le pilote. Le benchmark ne garantit pas la latence des périphériques. |
| Preset refusé | Vérifier qu'il vient de la même version du moteur et que le fichier est complet. |

Reason n'a pas été testé. Les [résultats observés](../verification.md) précisent le périmètre de validation.
