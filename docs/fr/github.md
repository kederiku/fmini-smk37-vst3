# Publier sur GitHub

[Accueil](../../README.fr.md) · [English](../en/github.md)

## Premier envoi

Exécuter `scripts/export-repository.ps1` pour créer `dist/FMini-SMK-37-github.zip`. Extraire l'archive dans le dossier à publier avec Git ou GitHub Desktop. Elle contient les sources, les deux langues, licences, interfaces/runtime épinglés, C généré et workflows `.github`. Inclure les fichiers commençant par un point ; exclure `tools`, `build`, `dist` et les notes de travail locales.

L'archive du dépôt constitue une copie propre des sources. `setup.ps1` récupère le firmware ; les ZIP de sources distribués avec les releases l'incluent intégralement, avec Felucca-Engines. Ne pas publier l'ancien paquet privé avec SAVIHost comme fichier de release publique.

Créer le dépôt puis envoyer les fichiers extraits sur sa branche principale. Aucun nom de compte ni URL de dépôt n'est codé en dur ; seul le `GITHUB_TOKEN` fourni automatiquement par GitHub est utilisé.

## Intégration continue

[ci.yml](../../.github/workflows/ci.yml) s'exécute lors des push de branches, pull requests et lancements manuels. La procédure de release le réutilise. Sur `windows-2022`, il :

1. Installe Node 22 et restaure le cache des téléchargements épinglés.
2. Vérifie les dépendances par SHA256 et les liens de documentation.
3. Compile le plugin et lance tous les tests natifs d'intégration.
4. Prépare le paquet binaire avec ses sources complètes puis vérifie l'archive.
5. Dépose les artefacts `windows-x64` et `native-test-log`.

Les actions sont épinglées par leur identifiant de commit complet. Les tests de pull requests ont uniquement des droits de lecture et ne peuvent pas créer de release. Références officielles : [syntaxe des workflows](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax), [upload-artifact](https://github.com/actions/upload-artifact) et [commandes de release GitHub CLI](https://cli.github.com/manual/gh_release_create).

## Créer une release

Renseigner `VERSION` au format `majeur.mineur.correctif`, actualiser `CHANGELOG.md`, créer un commit et l'envoyer. Ajouter à ce même commit le tag exact, par exemple :

```shell
git tag -a v0.1.0 -m "FMini SMK-37 0.1.0"
git push origin v0.1.0
```

[release.yml](../../.github/workflows/release.yml) relance toute la CI et refuse un tag différent de VERSION. Si tout passe, il crée une **release en brouillon**, avec le ZIP Windows, le ZIP des sources complètes et leurs empreintes. Ouvrir **Releases**, vérifier les fichiers, compléter les notes puis publier. Une nouvelle exécution peut actualiser le brouillon ; elle refuse de remplacer une release déjà publiée.

Le paquet Windows contient aussi son ZIP de sources : un seul téléchargement fournit donc les sources correspondantes. Conserver les sources, mentions et licences avec les binaires distribués.

Les fichiers de workflow peuvent être vérifiés localement avec [actionlint](https://github.com/rhysd/actionlint). Une compilation locale réussie ne prouve pas qu'une exécution sur GitHub a réussi ; consulter le premier passage dans Actions après l'envoi.
