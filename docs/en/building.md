# Building and testing

[Home](../../README.md) · [Français](../fr/compilation.md)

## Requirements

Windows x64, PowerShell 5.1 or 7, `tar.exe`, Node.js 22+, and internet access for the initial dependency download. LLVM-MinGW 20261104 UCRT and WABT 1.0.42 are downloaded into `tools`; Visual Studio is not required. Allow several GB of disk space for the extracted compiler and build/package staging.

From the repository root:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\setup.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\package.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\verify-package.ps1
```

`setup.ps1` verifies downloaded and cached archives against the committed SHA256 lock. A mismatch stops setup; review the dependency before changing its checksum. Run setup again when the lock or pinned versions change. The normal build uses the committed generated C, runtime and VST3 interface headers.

If Node is not in PATH, pass `-NodeExecutable 'C:\path\to\node.exe'` to `build.ps1`. No user-specific runtime location is assumed.

## Outputs

| Path | Contents |
| --- | --- |
| `dist/FMini SMK-37.vst3` | Standard Windows VST3 bundle. |
| `dist/FMini-SMK-37-<version>-Windows-x64.zip` | Public binary package with bilingual docs, licenses, checksums and embedded source ZIP. |
| `dist/FMini-SMK-37-<version>-sources.zip` | Complete corresponding source, including downloaded upstream firmware and engines. |
| `dist/FMini-SMK-37-<version>-SHA256SUMS.txt` | SHA256 hashes for both release ZIPs. |

The version comes from `VERSION`; `build.ps1` generates plugin metadata from it. Only a successful full build writes `build/verified-build.json`, recording the version and hashes of the binary and compile inputs. Packaging rejects stale or edited builds. After changing VERSION or source code, rebuild before packaging. Fresh staging prevents files from an earlier package entering the new archive.

For a local package with SAVIHost:

```powershell
.\scripts\setup.ps1 -IncludeSAVIHost
.\scripts\package.ps1 -IncludeSAVIHost
```

This creates a separate `-Windows-x64-SAVIHost.zip`, with separate `-SAVIHost-sources.zip` and `-SAVIHost-SHA256SUMS.txt` files. Public assets remain unchanged. Keep this package for use consistent with SAVIHost's included private-use license. GitHub Actions use the public package without SAVIHost.

## Tests

`build.ps1` runs the original WASM in Node, compiles the native engine and tests, then builds and loads the actual VST3. It checks reference audio, instance isolation, state roundtrips/corruption, button gesture recall, sample rates/block sizes, panel geometry, MIDI timing/channels, sustain, gain ramps, offline/zero-frame processing and saving during audio processing. The host tests run without a visible editor.

`build.ps1 -TestsOnly` skips the VST3 build/integration tests. `node tests/docs_test.mjs` checks local documentation links and action pin format. `tests/package_guards.ps1` verifies that untested, stale-version, modified-binary and edited-source builds cannot be packaged. `verify-package.ps1` checks package hashes, source completeness, workflow inclusion and absence of SAVIHost in public archives.

For a clean native build, remove the disposable `build/obj` directory and rerun `build.ps1`. The compiler flags are defined in that script; cached objects must be rebuilt after changing those flags or runtime headers.

## Regenerating the engine

`build.ps1 -RegenerateEngine` converts the pinned `felucca.wasm` to C with wasm2c. This optional step needs the Microsoft Visual C++ runtime and OpenSSL 3 DLLs available to the WABT executable. The ordinary build does not run wasm2c.

When changing upstream firmware, review its licenses/submodule, regenerate the C, update the engine hash in `src/Engine.cpp`, regenerate reference audio, and rerun the full tests. State compatibility is tied to the module hash; do not silently reuse it for a different firmware.
