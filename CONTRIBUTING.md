# Contributing

[Français](CONTRIBUTING.fr.md) · [Build guide](docs/en/building.md)

Issues and pull requests are welcome in English or French. For a bug report, include Windows version, host/version, plugin version, sample rate, buffer size, steps to reproduce, and the expected/actual result. Attach a minimal preset only if it is safe to share.

Keep source-code comments in English and update both documentation languages when behavior changes. Explain ownership, timing and format invariants instead of narrating obvious statements. Do not hand-edit generated code or alter vendored dependencies without recording the upstream revision and license.

Run `scripts/build.ps1` for code changes and `node tests/docs_test.mjs` for documentation. For packaging/CI changes, also run `scripts/package.ps1`, `scripts/verify-package.ps1` and actionlint. Describe what was tested; do not claim host compatibility without testing that host.

Keep stable VST3 IDs and state compatibility in mind. Add regression coverage for changes to DSP, event handling or state loading. Contributions to the port use GPL-3.0-only; retain all third-party notices.
