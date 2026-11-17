# FMini SMK-37 VST3

[![Compilation et tests Windows](https://github.com/kederiku/fmini-smk37-vst3/actions/workflows/ci.yml/badge.svg)](https://github.com/kederiku/fmini-smk37-vst3/actions/workflows/ci.yml)
[![Licence : GPL-3.0-only](https://img.shields.io/badge/licence-GPL--3.0--only-blue)](LICENSE)

**[English](README.md) | Français**

Un instrument VST3 Windows x64 basé sur [l'émulateur SMK-37 de FMini-1](https://ruppert.github.io/fmini-1/emu/smk/). Il exécute le firmware d'origine en natif et reproduit le panneau du SMK-37, avec son écran LCD. Aucun navigateur ni accès Internet n'est nécessaire pour jouer.

Il s'agit d'un portage communautaire indépendant, sans affiliation officielle avec M-VAVE, Steinberg ou Reason Studios.

## Fonctionnalités

- Moteur sonore, menus, presets, effets, arpégiateur et séquenceur FMini-1 d'origine.
- Panneau natif : 37 touches, 16 pads, huit encodeurs, quatre faders, 18 boutons, molettes de pitch/modulation et six finitions.
- Jeu à la souris, au clavier de l'ordinateur et par MIDI ; sortie audio stéréo.
- Notes, contrôleurs, sustain, pitch bend et pression sur 16 canaux MIDI, selon le routage du firmware.
- Automation du niveau de sortie et des quatre faders ; sauvegarde de l'état par l'hôte.
- Traitement vérifié à 44,1 / 48 / 88,2 / 96 kHz en audio 32 bits flottants.

## Installation

1. Télécharger le ZIP Windows x64 depuis [Releases](https://github.com/kederiku/fmini-smk37-vst3/releases) et l'extraire. S'il n'y a pas encore de release, récupérer les fichiers d'une exécution réussie de [Windows build and test](https://github.com/kederiku/fmini-smk37-vst3/actions/workflows/ci.yml), ou [compiler les sources](docs/fr/compilation.md).
2. Copier le dossier complet `VST3/FMini SMK-37.vst3` dans un dossier VST3 reconnu par l'hôte, généralement `C:\Program Files\Common Files\VST3`.
3. Relancer la recherche de plugins puis charger **FMini SMK-37** comme instrument.

Pour jouer sans logiciel de musique, suivre les [instructions SAVIHost](docs/fr/utilisation.md#savihost). Les archives publiques contiennent le VST3 et ses sources complètes ; SAVIHost se télécharge séparément et conserve sa licence d'usage privé.

## Documentation

| Sujet | Français | English |
| --- | --- | --- |
| Jeu, sauvegarde et dépannage | [Guide utilisateur](docs/fr/utilisation.md) | [User guide](docs/en/usage.md) |
| Compilation et tests | [Compilation](docs/fr/compilation.md) | [Build guide](docs/en/building.md) |
| GitHub Actions et releases | [Publication](docs/fr/github.md) | [Publishing guide](docs/en/github.md) |
| Organisation du code et des threads | [Architecture](docs/fr/architecture.md) | [Architecture](docs/en/architecture.md) |
| Contributions | [Contribuer](CONTRIBUTING.fr.md) | [Contributing](CONTRIBUTING.md) |
| Résultats de validation | [Vérification](docs/verification.md) | [Verification](docs/verification.en.md) |

## Limites actuelles

Windows x64 uniquement. Reason 13+ reste une cible de compatibilité : **aucun essai dans Reason n'a été effectué**. L'affichage du panneau et le son au clavier et à la souris ont été confirmés dans SAVIHost ; l'essai manuel exhaustif des commandes reste à faire.

L'arpégiateur et le séquenceur utilisent leur horloge interne ; la synchronisation au tempo et à la position de l'hôte n'est pas implémentée. Pas de sortie MIDI VST3, de SysEx, de connexion Bluetooth ni de mise à jour matérielle. Les états sauvegardés sont liés à la version exacte du moteur. Le plugin ne traite pas l'audio 64 bits flottants.

## Licence et crédits

Le portage est sous **GPL-3.0-only**. Chaque paquet binaire public inclut les sources correspondantes complètes, avec le firmware d'origine et son sous-module Felucca-Engines.

FMini-1 est créé par Guilherme Ruppert, à partir de Felucca de Leo Kuroshita / Hügelton Instruments. Le panneau SMK reprend le travail de Chance Roth ; le port matériel est crédité à M. Geisler. Voir les [mentions des composants tiers](THIRD_PARTY_NOTICES.fr.md), la [licence](LICENSE) et les [textes conservés](licenses).
