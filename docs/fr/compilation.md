# Compilation et tests

[Accueil](../../README.fr.md) · [English](../en/building.md)

## Prérequis

Windows x64, PowerShell 5.1 ou 7, `tar.exe`, Node.js 22+ et une connexion Internet pour le premier téléchargement. LLVM-MinGW 20261104 UCRT et WABT 1.0.42 sont téléchargés dans `tools` ; Visual Studio n'est pas requis. Prévoir plusieurs Go pour le compilateur extrait et les dossiers de compilation et de préparation des archives.

Depuis la racine du dépôt :

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\setup.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\package.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\verify-package.ps1
```

`setup.ps1` vérifie les archives téléchargées et mises en cache avec les empreintes SHA256 conservées dans le dépôt. Toute différence arrête la préparation ; vérifier la dépendance avant de modifier son empreinte. Relancer setup après un changement de versions ou d'empreintes. La compilation ordinaire utilise le C généré, le runtime et les interfaces VST3 inclus dans le dépôt.

Si Node n'est pas dans le PATH, fournir `-NodeExecutable 'C:\chemin\vers\node.exe'` à `build.ps1`. Aucun emplacement propre à un utilisateur n'est supposé.

## Résultats

| Chemin | Contenu |
| --- | --- |
| `dist/FMini SMK-37.vst3` | Bundle VST3 Windows standard. |
| `dist/FMini-SMK-37-<version>-Windows-x64.zip` | Paquet public avec documentation bilingue, licences, empreintes et archive des sources. |
| `dist/FMini-SMK-37-<version>-sources.zip` | Sources correspondantes complètes, avec le firmware et ses moteurs téléchargés. |
| `dist/FMini-SMK-37-<version>-SHA256SUMS.txt` | Empreintes SHA256 des deux ZIP de distribution. |

La version vient de `VERSION` ; `build.ps1` génère les métadonnées du plugin à partir de ce fichier. Seule une compilation complète réussie écrit `build/verified-build.json`, avec la version et les empreintes du binaire et des fichiers utilisés pour compiler. La création du paquet refuse les compilations anciennes ou modifiées. Après un changement de VERSION ou du code, recompiler. Chaque paquet utilise un nouveau dossier temporaire pour éviter de reprendre des fichiers d'une ancienne archive.

Pour préparer un paquet local avec SAVIHost :

```powershell
.\scripts\setup.ps1 -IncludeSAVIHost
.\scripts\package.ps1 -IncludeSAVIHost
```

Cela crée un ZIP séparé `-Windows-x64-SAVIHost.zip`, avec ses propres fichiers `-SAVIHost-sources.zip` et `-SAVIHost-SHA256SUMS.txt`. Les fichiers publics restent inchangés. Son utilisation reste soumise à la licence d'usage privé de SAVIHost. GitHub Actions prépare le paquet public sans SAVIHost.

## Tests

`build.ps1` exécute le WASM d'origine dans Node, compile le moteur et ses tests, puis compile et charge le véritable VST3. Il vérifie le son de référence, l'isolation des instances, les sauvegardes et états corrompus, les gestes de boutons, fréquences/tailles de blocs, géométrie du panneau, événements/canaux MIDI, sustain, automation du gain, rendu hors ligne/appels sans échantillon et sauvegarde pendant le traitement audio. Les tests d'hôte n'ouvrent pas de fenêtre d'éditeur.

`build.ps1 -TestsOnly` omet la compilation et les tests d'intégration VST3. `node tests/docs_test.mjs` vérifie les liens locaux et le format des références d'actions. `tests/package_guards.ps1` vérifie le refus des compilations non validées, anciennes ou dont le binaire ou les sources ont été modifiés. `verify-package.ps1` contrôle les empreintes, les sources complètes, la présence des workflows et l'absence de SAVIHost dans les archives publiques.

Pour une compilation native propre, supprimer le dossier de travail `build/obj` puis relancer `build.ps1`. Les options du compilateur se trouvent dans ce script ; reconstruire les objets après modification de ces options ou des en-têtes du runtime.

## Régénérer le moteur

`build.ps1 -RegenerateEngine` convertit le `felucca.wasm` épinglé en C avec wasm2c. Cette étape facultative demande les bibliothèques Visual C++ de Microsoft et les DLL OpenSSL 3 accessibles à WABT. La compilation ordinaire n'exécute pas wasm2c.

Lors d'un changement de firmware, vérifier ses licences et son sous-module, régénérer le C, actualiser l'empreinte dans `src/Engine.cpp`, recalculer l'audio de référence et relancer tous les tests. La compatibilité des sauvegardes dépend de cette empreinte ; ne pas la réutiliser pour un moteur différent.
