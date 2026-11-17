# FMini SMK-37 VST3

[![Windows build and test](https://github.com/kederiku/fmini-smk37-vst3/actions/workflows/ci.yml/badge.svg)](https://github.com/kederiku/fmini-smk37-vst3/actions/workflows/ci.yml)
[![License: GPL-3.0-only](https://img.shields.io/badge/license-GPL--3.0--only-blue)](LICENSE)

**English | [Français](README.fr.md)**

A Windows x64 VST3 instrument based on the [FMini-1 SMK-37 browser emulator](https://ruppert.github.io/fmini-1/emu/smk/). It runs the original synth firmware natively and recreates the SMK-37 panel, including the firmware's own LCD display. No browser or internet connection is needed to play.

This is an independent community port, not an official M-VAVE, Steinberg or Reason Studios product.

## Features

- Original FMini-1 sound engine, menus, presets, effects, arpeggiator and sequencer.
- Native panel with 37 keys, 16 pads, eight encoders, four faders, 18 buttons, pitch/modulation wheels and six finishes.
- Mouse, computer-keyboard and host MIDI input; stereo audio output.
- MIDI notes, controllers, sustain, pitch bend and pressure across 16 channels, subject to firmware routing.
- Host automation for output level and four faders; host-managed state saving.
- Verified processing at 44.1, 48, 88.2 and 96 kHz with 32-bit float audio.

## Install

1. Download the Windows x64 ZIP from [Releases](https://github.com/kederiku/fmini-smk37-vst3/releases) and extract it. If no release exists yet, use a successful [Windows build and test](https://github.com/kederiku/fmini-smk37-vst3/actions/workflows/ci.yml) run, or [build from source](docs/en/building.md).
2. Copy the entire `VST3/FMini SMK-37.vst3` bundle into a VST3 folder recognized by your host, normally `C:\Program Files\Common Files\VST3`.
3. Rescan plugins and load **FMini SMK-37** as an instrument.

For use without a DAW, see the [SAVIHost instructions](docs/en/usage.md#savihost). Public release archives contain the VST3 and its complete corresponding source; SAVIHost is downloaded separately under its own private-use license.

## Documentation

| Topic | English | Français |
| --- | --- | --- |
| Playing, saving and troubleshooting | [User guide](docs/en/usage.md) | [Guide utilisateur](docs/fr/utilisation.md) |
| Building and testing | [Build guide](docs/en/building.md) | [Compilation](docs/fr/compilation.md) |
| GitHub Actions and releases | [Publishing guide](docs/en/github.md) | [Publication](docs/fr/github.md) |
| Code structure and threading | [Architecture](docs/en/architecture.md) | [Architecture](docs/fr/architecture.md) |
| Contributing | [Contributing](CONTRIBUTING.md) | [Contribuer](CONTRIBUTING.fr.md) |
| Validation results | [Verification](docs/verification.en.md) | [Vérification](docs/verification.md) |

## Current limitations

Windows x64 only. Reason 13+ is a compatibility target, **not a tested host**. Panel visibility and computer-keyboard/mouse sound were confirmed in SAVIHost; exhaustive manual control testing is still pending.

The arpeggiator/sequencer use their internal clock; host tempo/position synchronization is not implemented. There is no VST3 MIDI output, SysEx, Bluetooth connection or hardware firmware flashing. State files are tied to the exact embedded engine version. The plugin does not process 64-bit float audio.

## License and credits

The port is **GPL-3.0-only**. Every public binary package includes the complete corresponding source, including the original firmware and its Felucca-Engines submodule.

FMini-1 is by Guilherme Ruppert, based on Felucca by Leo Kuroshita / Hügelton Instruments. The SMK panel derives from Chance Roth's work; the hardware port is credited to M. Geisler. See [third-party notices](THIRD_PARTY_NOTICES.md), [LICENSE](LICENSE) and the retained [license texts](licenses).
