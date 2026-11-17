# Publishing on GitHub

[Home](../../README.md) · [Français](../fr/github.md)

## First upload

Use `scripts/export-repository.ps1` to create `dist/FMini-SMK-37-github.zip`. Extract it into the folder you will publish with Git or GitHub Desktop. It contains the source, both languages, licenses, pinned interfaces/runtime, generated engine C and `.github` workflows. Include dotfiles; exclude `tools`, `build`, `dist` and local working notes.

The repository archive is a clean source checkout. Downloaded firmware sources are obtained by `setup.ps1`; release source ZIPs include them in full, including Felucca-Engines. Do not upload the old private SAVIHost bundle as a public release asset.

Create your repository and push the extracted files to `main`. The release script uses the current repository automatically; no secret beyond GitHub's automatically provided `GITHUB_TOKEN` is required. Adapt the public download links in the READMEs if you fork this repository.

## Continuous integration

[ci.yml](../../.github/workflows/ci.yml) runs on pushes outside `main`, pull requests and manual dispatch. Updates to `main` run it through the release workflow, avoiding duplicate builds. On `windows-2022` it:

1. Installs Node 22 and restores the pinned download cache.
2. Checks dependencies against the SHA256 lock, documentation links and release automation tests.
3. Builds the instrument and runs all native integration tests.
4. Packages binaries plus full corresponding sources and verifies the archive.
5. Uploads `windows-x64` and the `native-test-log` artifact.

Actions are pinned to full commit hashes. Pull-request builds have read-only repository permissions; they cannot create releases. Configuration details follow the official [workflow syntax](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax), [upload-artifact](https://github.com/actions/upload-artifact) and [GitHub release API](https://docs.github.com/en/rest/releases/releases).

## Automatic publication from main

Merge a pull request into `main`, or push directly to `main`. [release.yml](../../.github/workflows/release.yml) builds and tests that exact commit, then **publishes a public release automatically**. There is no tag to create or Publish button to click. A failed build/test/package check prevents publication.

`VERSION` remains the plugin's `major.minor.patch` version. Update it and `CHANGELOG.md` when appropriate; no version bump is needed for every merge. Each run gets a distinct tag, for example `v0.1.0-build.12`, using GitHub's workflow run number. The tag points to the tested commit. Rerunning the same Actions run keeps its tag.

Each release contains:

- `FMini-SMK-37-Windows-x64.zip`: compiled VST3, bilingual documentation, licenses and embedded complete sources.
- `FMini-SMK-37-sources.zip`: the same complete corresponding sources as a separate download.
- `SHA256SUMS.txt`: checksums for the two published ZIPs.

The script verifies the build artifact checksums, stages a private draft, uploads and verifies every asset, then publishes it. If an upload fails, use **Actions → failed run → Re-run failed jobs** to complete the draft. A rerun of an already published release leaves its assets unchanged. Older runs cannot replace a newer automatic release as **Latest**. **Run workflow** on `main` can also create a new build; other branches cannot publish.

Publication jobs use a shared [concurrency queue](https://docs.github.com/en/actions/how-tos/write-workflows/choose-when-workflows-run/control-workflow-concurrency) with `queue: max`, so consecutive merges wait instead of canceling each other (GitHub limit: 100 waiting jobs). Builds can run concurrently. Only the publication job has `contents: write` permission.

The fixed asset name provides a permanent link to the latest compiled build:

[Download Windows x64](https://github.com/kederiku/fmini-smk37-vst3/releases/latest/download/FMini-SMK-37-Windows-x64.zip)

Local packages and Actions artifacts retain versioned filenames; the publisher renames the two public ZIPs and generates matching checksums. Existing manually created drafts, such as `v0.1.0`, remain untouched.

The Windows package also embeds its source ZIP, so downloading that one file includes the corresponding source. Keep sources, notices and licenses with any distributed binaries.

Run `node --test tests/release_test.mjs` for offline publication/retry/asset validation tests. The workflow files can be checked with [actionlint](https://github.com/rhysd/actionlint); versions without support for GitHub's `queue` field report that specific field as unknown. A local successful build does not prove a hosted GitHub run has succeeded; inspect Actions and the public release after uploading.
