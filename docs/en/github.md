# Publishing on GitHub

[Home](../../README.md) · [Français](../fr/github.md)

## First upload

Use `scripts/export-repository.ps1` to create `dist/FMini-SMK-37-github.zip`. Extract it into the folder you will publish with Git or GitHub Desktop. It contains the source, both languages, licenses, pinned interfaces/runtime, generated engine C and `.github` workflows. Include dotfiles; exclude `tools`, `build`, `dist` and local working notes.

The repository archive is a clean source checkout. Downloaded firmware sources are obtained by `setup.ps1`; release source ZIPs include them in full, including Felucca-Engines. Do not upload the old private SAVIHost bundle as a public release asset.

Create your repository and push the extracted files to its default branch. No repository URL or account name is hardcoded, and no secret beyond GitHub's automatically provided `GITHUB_TOKEN` is required.

## Continuous integration

[ci.yml](../../.github/workflows/ci.yml) runs on branch pushes, pull requests and manual dispatch. It is also reusable by the release workflow. On `windows-2022` it:

1. Installs Node 22 and restores the pinned download cache.
2. Checks dependencies against the SHA256 lock and checks documentation links.
3. Builds the instrument and runs all native integration tests.
4. Packages binaries plus full corresponding sources and verifies the archive.
5. Uploads `windows-x64` and the `native-test-log` artifact.

Actions are pinned to full commit hashes. Pull-request builds have read-only repository permissions; they cannot create releases. Configuration details follow the official [workflow syntax](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax), [upload-artifact](https://github.com/actions/upload-artifact) and [GitHub CLI release commands](https://cli.github.com/manual/gh_release_create).

## Release a version

Set `VERSION` to `major.minor.patch`, update `CHANGELOG.md`, commit and push. Tag that same commit with the exact version, for example:

```shell
git tag -a v0.1.0 -m "FMini SMK-37 0.1.0"
git push origin v0.1.0
```

[release.yml](../../.github/workflows/release.yml) invokes the full CI and rejects a tag that does not match VERSION. On success, it creates a **draft release**, attaches the Windows ZIP, complete source ZIP and checksums. Review it in **Releases**, edit the notes and publish when ready. Reruns can update a draft; they refuse to replace an already published release.

The Windows package also embeds its source ZIP, so downloading that one file includes the corresponding source. Keep sources, notices and licenses with any distributed binaries.

The workflow files can be validated locally with [actionlint](https://github.com/rhysd/actionlint). A local successful build does not prove a hosted GitHub run has succeeded; inspect the first Actions run after uploading.
