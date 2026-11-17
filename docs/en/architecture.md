# Architecture

[Home](../../README.md) · [Français](../fr/architecture.md)

The original WASM is converted to C by wasm2c and compiled into the plugin. This preserves its DSP, firmware UI and emulated storage without embedding a web browser. Each `Engine` owns its linear memory and mutable globals; separate instances do not share musical state.

| Files | Responsibility |
| --- | --- |
| `src/Engine.*` | Boot, firmware exports, bounded render chunks, controls, LCD/LED snapshots and versioned state. |
| `src/Audio.*` | Fixed 44.1 kHz engine to host-rate adaptation using a persistent 16-tap, 1024-phase sinc filter. |
| `src/Plugin.*` | VST3 factory/component/controller, MIDI mapping, automation, processing and host state streams. |
| `src/Editor.*`, `src/EditorPaint.cpp` | Win32 input/capture and GDI+ panel drawing. |
| `src/Panel.*` | Shared logical layout and hit testing; black keys take precedence. |
| `src/Bytes.h` | Little-endian state encoding, bounded reading and corruption checksum. |
| `third_party/generated` | Generated engine C and header; regenerate rather than edit manually. |

## Thread ownership

The audio callback normally owns the engine. UI producers serialize access to a bounded control queue; the consumer does not take the producer mutex. Host events are sorted by sample offset and processed between audio segments. Continuous gain points are interpolated. Queue overflow requests a panic to avoid stuck notes.

Display copies use a separate lock. The audio thread only tries that lock and skips a display update if busy. State replacement uses the engine lock; if it is busy, processing emits silence and defers host events rather than dropping note-offs.

For state saving, the calling thread allocates a separate engine, asks the callback to copy memory at a block boundary, then compresses/writes the copy outside the callback. An idle host uses a guarded fallback after 200 ms. The single 32 MB copy is still work on the audio thread; the test verifies event retention, not worst-case scheduling on every device.

## State compatibility

The outer state contains version, finish, parameter values and an engine payload. The inner state contains a magic/version, exact WASM hash, memory size/scalars, checksum, pending button masks and zero-run encoded memory. Native pointers/function tables are never serialized. Recall validates the data before swapping a fresh engine into place, then releases held notes.

The checksum detects accidental corruption; it is not authentication. The VST3 class ID and parameter IDs must remain stable to preserve host projects. `VERSION` controls release/plugin metadata and is distinct from the state format version and embedded engine hash.
