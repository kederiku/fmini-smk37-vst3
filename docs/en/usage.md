# User guide

[Home](../../README.md) · [Français](../fr/utilisation.md)

## SAVIHost

1. Download the **VST3 x64** edition from the [official SAVIHost page](https://www.hermannseib.com/english/savihost.htm) and extract its full package. Read its included license: the free host is for private use.
2. Copy `VST3/FMini SMK-37.vst3/Contents/x86_64-win/FMini SMK-37.vst3` from the plugin archive into the SAVIHost directory. This is the inner binary file, not the outer bundle directory.
3. Copy or rename `savihost.exe` to **`FMini SMK-37.exe`**, keeping exactly the same base name as the `.vst3` file. Keep `license.txt` and `history.txt`.
4. Launch `FMini SMK-37.exe`. Choose an audio output in **Devices > Wave** and, if needed, a MIDI input in **Devices > MIDI**.

A locally prepared `-SAVIHost.zip` already contains these files. Extract it before starting the executable. Public GitHub packages omit the host.

## Panel controls

| Control | Gesture |
| --- | --- |
| Keys / pads | Hold the left mouse button; drag to another key or pad. Click height determines velocity. |
| Encoders | Drag up/down or use the mouse wheel. Hold Shift during dragging for finer changes. |
| Faders / wheels | Click or drag. Pitch returns to center when released. |
| Buttons | Hold the mouse button. Shift-click latches a button for combinations; Shift-click again releases it. |
| Finish | Use the top-right menu to select one of six colors. |
| Output | The top slider controls plugin gain; center is unity (100%). |
| Computer keyboard | Click the panel, then use A W S E D F T G Y H U J K O L P ; (layout-dependent). |
| Octave | Z / X. |
| Panic | Escape while the panel has keyboard focus. |

The LCD is the original firmware display. Its menus and musical behavior follow [FMini-1](https://github.com/ruppert/fmini-1). The firmware startup animation is prerendered before the instrument becomes available.

## MIDI and automation

Route a MIDI track or keyboard to the instrument. The plugin preserves MIDI channels; the firmware's selected routing determines which parts respond. Sustain uses CC64; pitch bend, modulation and channel/polyphonic pressure are forwarded. The host exposes OUTPUT and Fader 1–4 for automation; additional hidden parameters carry VST3 MIDI controller assignments.

An internal arp or sequence uses its own tempo. Host transport stop releases notes, but host tempo and song-position synchronization are not available. Physical USB, TRS and Bluetooth options in the firmware do not create hardware connections from this plugin.

## Save and restore

Save an instrument preset or project using your host. The state includes firmware memory, settings/sequences, queued button gestures, output level, faders and finish. Saving only inside the firmware does not replace saving the host project. Held notes are released when a state is recalled.

Keep presets with the plugin version that created them. Invalid or incompatible engine states are rejected. Typical test snapshots are about 1.5 MB; size varies with the state.

## Troubleshooting

| Symptom | Check |
| --- | --- |
| No sound | Host audio device, active MIDI routing/channel, host track monitoring, MASTER and OUTPUT levels. Try a mouse key. |
| Typing does not play | Click inside the panel. Hosts can intercept keyboard shortcuts; try mouse input or an external MIDI keyboard. |
| Stuck note | Press Escape with panel focus; check sustain pedal and MIDI input. |
| SAVIHost cannot load the plugin | Use VST3 x64 SAVIHost; match executable and plugin filenames; use the inner `.vst3` file. |
| DAW cannot find the plugin | Install the whole bundle, use a 64-bit Windows host, and rescan its VST3 folder. |
| Crackles | Increase the host's audio buffer and check the audio driver. Benchmark results are not device-latency guarantees. |
| Preset rejected | Check that it was saved with the same embedded firmware version and that the file is complete. |

Reason has not been tested. See [observed validation](../verification.en.md) before interpreting compatibility claims.
