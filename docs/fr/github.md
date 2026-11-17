# Publier sur GitHub

[Accueil](../../README.fr.md) · [English](../en/github.md)

## Premier envoi

Exécuter `scripts/export-repository.ps1` pour créer `dist/FMini-SMK-37-github.zip`. Extraire l'archive dans le dossier à publier avec Git ou GitHub Desktop. Elle contient les sources, les deux langues, licences, interfaces/runtime épinglés, C généré et workflows `.github`. Inclure les fichiers commençant par un point ; exclure `tools`, `build`, `dist` et les notes de travail locales.

L'archive du dépôt constitue une copie propre des sources. `setup.ps1` récupère le firmware ; les ZIP de sources distribués avec les releases l'incluent intégralement, avec Felucca-Engines. Ne pas publier l'ancien paquet privé avec SAVIHost comme fichier de release publique.

Créer le dépôt puis envoyer les fichiers extraits sur `main`. Le script de publication utilise automatiquement le dépôt courant ; seul le `GITHUB_TOKEN` fourni automatiquement par GitHub est nécessaire. Adapter les liens publics de téléchargement des README en cas de fork.

## Intégration continue

[ci.yml](../../.github/workflows/ci.yml) s'exécute lors des push hors de `main`, des pull requests et des lancements manuels. Sur `main`, la procédure de release le réutilise pour éviter une double compilation. Sur `windows-2022`, il :

1. Installe Node 22 et restaure le cache des téléchargements épinglés.
2. Vérifie les dépendances par SHA256, les liens de documentation et les tests de publication automatique.
3. Compile le plugin et lance tous les tests natifs d'intégration.
4. Prépare le paquet binaire avec ses sources complètes puis vérifie l'archive.
5. Dépose les artefacts `windows-x64` et `native-test-log`.

Les actions sont épinglées par leur identifiant de commit complet. Les tests de pull requests ont uniquement des droits de lecture et ne peuvent pas créer de release. Références officielles : [syntaxe des workflows](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax), [upload-artifact](https://github.com/actions/upload-artifact) et [API des releases GitHub](https://docs.github.com/en/rest/releases/releases).

## Publication automatique depuis main

Fusionner une pull request dans `main`, ou envoyer directement un commit sur `main`. [release.yml](../../.github/workflows/release.yml) compile et teste ce commit précis, puis **publie automatiquement une release publique**. Aucun tag à créer ni bouton Publish à cliquer. Un échec de compilation, de test ou de vérification des paquets empêche la publication.

`VERSION` conserve la version `majeur.mineur.correctif` du plugin. La modifier avec `CHANGELOG.md` quand nécessaire ; aucun changement de version n'est requis à chaque merge. Chaque exécution reçoit un tag distinct, par exemple `v0.1.0-build.12`, utilisant le numéro d'exécution GitHub Actions. Le tag désigne le commit testé. Relancer la même exécution conserve son tag.

Chaque release contient :

- `FMini-SMK-37-Windows-x64.zip` : VST3 compilé, documentation bilingue, licences et sources complètes intégrées.
- `FMini-SMK-37-sources.zip` : les mêmes sources correspondantes complètes, téléchargeables séparément.
- `SHA256SUMS.txt` : empreintes des deux ZIP publiés.

Le script vérifie les empreintes des artefacts, prépare un brouillon privé, envoie et vérifie tous les fichiers, puis le publie. Si un envoi échoue, utiliser **Actions → exécution en échec → Re-run failed jobs** pour compléter le brouillon. Relancer une release déjà publiée laisse ses fichiers inchangés. Une ancienne exécution ne peut pas remplacer une release automatique plus récente comme **Latest**. **Run workflow** sur `main` permet aussi de créer un nouveau build ; les autres branches ne peuvent pas publier.

Les publications utilisent une [file d'attente commune](https://docs.github.com/en/actions/how-tos/write-workflows/choose-when-workflows-run/control-workflow-concurrency) avec `queue: max` : les merges successifs attendent au lieu de s'annuler (limite GitHub : 100 tâches en attente). Les compilations peuvent s'exécuter en parallèle. Seule la tâche de publication possède le droit `contents: write`.

Le nom fixe du fichier permet un lien permanent vers le dernier build compilé :

[Télécharger pour Windows x64](https://github.com/kederiku/fmini-smk37-vst3/releases/latest/download/FMini-SMK-37-Windows-x64.zip)

Les paquets locaux et artefacts Actions conservent des noms versionnés ; le script renomme les deux ZIP publics et génère les empreintes correspondantes. Les anciens brouillons créés manuellement, comme `v0.1.0`, restent inchangés.

Le paquet Windows contient aussi son ZIP de sources : un seul téléchargement fournit donc les sources correspondantes. Conserver les sources, mentions et licences avec les binaires distribués.

Exécuter `node --test tests/release_test.mjs` pour tester hors ligne la publication, les reprises et la validation des fichiers. Les workflows peuvent être contrôlés avec [actionlint](https://github.com/rhysd/actionlint) ; les versions ne prenant pas encore en charge le champ GitHub `queue` le signalent comme inconnu. Une compilation locale réussie ne prouve pas qu'une exécution sur GitHub a réussi ; vérifier Actions et la release publique après l'envoi.
