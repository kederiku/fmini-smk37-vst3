# Origine du code et licences

[English](THIRD_PARTY_NOTICES.md)

Ce portage indépendant n'est pas un produit officiel de M-VAVE, Reason Studios ou Steinberg. Les marques appartiennent à leurs titulaires.

## FMini-1 et panneau SMK-37

FMini-1 est créé par Guilherme Ruppert, à partir de Felucca de Leo Kuroshita / Hügelton Instruments et des contributions décrites dans les fichiers `CREDITS.md` et `LICENSING.md` du projet d'origine. Le panneau SMK est issu du travail de Chance Roth (ChanceTheMaker), adapté par Guilherme Ruppert. Le port matériel SMK-37 est crédité à M. Geisler (genkigenki).

- Source : https://github.com/ruppert/fmini-1
- Révision : `46ff9987680b01d12522779448e8b7f07625093f`.
- Licence du firmware, du panneau dérivé et du portage VST3 : **GPL-3.0-only**, texte intégral dans `LICENSE`.
- Module utilisé : `docs/emu/smk/felucca.wasm`, SHA256 `DCBFA908BD9213C454238A7769160F00634A22C1EFA8628D7958688C7F76862B`.
- Adaptations du 17 novembre 2026 : compilation native du module avec wasm2c, interface VST3, panneau Win32/GDI+, adaptation de fréquence et sauvegarde d'état. Aucun changement apporté au module WebAssembly d'origine.

Le sous-module **Felucca-Engines**, MIT, est inclus dans `firmware/engines`, révision `c46e0086f69bdc26df1cd144dca9e683cc4531dd` de https://github.com/hugelton/Felucca-Engines. Ses auteurs sont crédités dans sa licence conservée.

Les licences du code DSP et des ressources intégrés au firmware sont conservées dans `licenses` et dans le répertoire `LICENSES` d'origine : DaisySP (MIT), Rings (MIT), msfa (Apache-2.0), Felucca-Engines (MIT), Fukiai (MIT), Inter Tight et DotGothic16 (SIL OFL). Voir les notices originales pour leur périmètre et leurs auteurs.

## Interfaces VST3

Copyright Steinberg Media Technologies GmbH. Source : https://github.com/steinbergmedia/vst3_pluginterfaces, révision `4f547e8e102b47de4a8b8aaf343c73b700786372`. Licence MIT conservée dans `licenses/MIT-Steinberg-VST3.txt` et dans les sources. VST est une marque de Steinberg Media Technologies GmbH.

## WABT / wasm2c

Copyright WebAssembly Community Group participants. WABT 1.0.42 : https://github.com/WebAssembly/wabt/tree/1.0.42. Le runtime lié au plugin et ses sources sont inclus, sous Apache-2.0 ; texte dans `licenses/Apache-2.0-WABT.txt`. Le C produit par wasm2c provient du module FMini-1 sous GPL-3.0-only.

## Bibliothèques de compilation

LLVM-MinGW 20261104 UCRT : https://github.com/mstorsjo/llvm-mingw/releases/tag/20261104. Les notices des bibliothèques LLVM et MinGW utilisées sont conservées dans `licenses`. Le compilateur et ses outils ne sont pas inclus dans le paquet Windows ; le script de préparation les télécharge.

## SAVIHost

SAVIHost 1.43 VST3 x64, copyright Hermann Seib : https://www.hermannseib.com/english/savihost.htm. Il n'est pas inclus dans les archives publiques GitHub. L'option locale `-IncludeSAVIHost` conserve son paquet officiel complet, avec `license.txt` et `history.txt`, et ajoute une copie de l'exécutable renommée `FMini SMK-37.exe`. Son binaire n'est pas modifié.

**SAVIHost reste soumis à sa licence distincte, pour usage privé gratuit uniquement.** Sa présence à côté du plugin ne le place pas sous GPL. Les sources du plugin, avec toutes ses dépendances sources, sont fournies séparément dans l'archive Sources du paquet Windows.
