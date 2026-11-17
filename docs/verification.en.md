# Verification — November 17, 2026

[Français](verification.md) · [Home](../README.md)

Version 0.1.0, Windows x64. The local full `scripts/build.ps1` run passed. These results describe the measured scenarios; they do not certify every host or device.

| Check | Observed result |
| --- | --- |
| Original WASM in Node | No imports; 88,200 stereo frames, peak 0.170440673828125. |
| Native/reference audio | Maximum error 0 on both channels, using the same startup prerender. |
| Independent engines/plugins | Unplayed instance peak 0. |
| Live engine state | 1,602,672 bytes in the test; identical audio after recall. |
| Pending OCT+ gesture | Preserved when saved before firmware consumption. |
| Truncated/corrupt state | Rejected without replacing the current state. |
| 44.1/48/88.2/96 kHz | Variable blocks (1, 31, 128, 511, 2048, 4096) match continuous rendering; released tail reaches silence. |
| Actual VST3 loading | Factory, interfaces, stereo/MIDI buses, lifecycle, timed notes and parameters pass. |
| Double precision | Explicit rejection; 32-bit float processing is supported. |
| VST3 state | 1,504,480 bytes in the scenario; parameter recall and truncation rejection pass. |
| Output automation | Gain ramp checked against rendered reference samples. |
| MIDI channels | Separate mappings; channel-2 sustain unaffected by channel-1 pedal-up, released by its own pedal-up. |
| Zero-frame/no-output call | MIDI/parameter flush succeeds. |
| Offline processing | Setup and audio render pass without an editor window. |
| Concurrent saving | No artificial silent block in the test; note-off retained and final tail released. |
| Panel geometry | Control counts/reachability and black-key hit priority pass. |

Original local benchmark: six notes, 48 kHz, 256 blocks of 4096 frames; 21.845 seconds of audio rendered in 0.241 seconds (about 90.66x realtime). This offline number does not measure device latency or worst-case callback deadlines. State capture copies 32 MB once; compression runs outside the audio callback.

The plugin imports Windows system libraries only (USER32, GDI+, KERNEL32 and UCRT). It does not require a browser or network at runtime.

## Manual observation

SAVIHost VST3 x64 loaded the initial plugin build. The tester confirmed the panel was visible and sound worked from both computer-keyboard and mouse input. Later changes to state saving, automation, MIDI routing and startup prerender were checked by the automated tests. Automated native-window capture timed out; no native-panel screenshot or pixel-by-pixel visual comparison is claimed. Not every knob, menu, sequence or window lifecycle has been checked manually.

Reason is not installed and has not been tested. Host tempo/position synchronization, MIDI output, SysEx and hardware connections are outside this version.

## Review and CI scope

An independent code review identified saving contention, missing pending button gestures, stepped continuous automation and lost MIDI channels. These were corrected; the first three had reproduced failures before the final passing tests. Channel-specific sustain is also covered.

GitHub workflow syntax and packaging can be checked locally. A hosted Actions result is only established after a run in the uploaded repository. Package source/binary hashes are recorded by `scripts/package.ps1`.
