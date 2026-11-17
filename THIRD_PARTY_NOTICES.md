# Third-party notices

[Français](THIRD_PARTY_NOTICES.fr.md)

This independent port is not an official M-VAVE, Reason Studios or Steinberg product. Trademarks belong to their respective owners. Original license texts are retained without translation in [LICENSE](LICENSE) and [licenses](licenses).

## FMini-1 and SMK-37 panel

FMini-1 is by Guilherme Ruppert, based on Felucca by Leo Kuroshita / Hügelton Instruments and the contributors listed in upstream `CREDITS.md` and `LICENSING.md`. The SMK panel derives from Chance Roth (ChanceTheMaker), adapted by Guilherme Ruppert. The hardware SMK-37 port is credited to M. Geisler (genkigenki).

- Source: https://github.com/ruppert/fmini-1
- Revision: `46ff9987680b01d12522779448e8b7f07625093f`.
- Firmware, derived panel and VST3 port license: **GPL-3.0-only**.
- Module: `docs/emu/smk/felucca.wasm`, SHA256 `DCBFA908BD9213C454238A7769160F00634A22C1EFA8628D7958688C7F76862B`.
- Adaptations dated November 17, 2026: native wasm2c build, VST3 interface, Win32/GDI+ panel, sample-rate conversion and state management. The original WASM module is unchanged.

The MIT-licensed **Felucca-Engines** submodule is included in the release source archive under `firmware/engines`, revision `c46e0086f69bdc26df1cd144dca9e683cc4531dd`, from https://github.com/hugelton/Felucca-Engines. Its original author notices are retained.

DSP/resource licenses retained in `licenses` and upstream `LICENSES` cover DaisySP (MIT), Rings (MIT), msfa (Apache-2.0), Felucca-Engines (MIT), Fukiai (MIT), Inter Tight and DotGothic16 (SIL OFL). Refer to those original notices for the precise authors and scope.

## VST3 interfaces

Copyright Steinberg Media Technologies GmbH. Source: https://github.com/steinbergmedia/vst3_pluginterfaces, revision `4f547e8e102b47de4a8b8aaf343c73b700786372`. The MIT license is retained in `licenses/MIT-Steinberg-VST3.txt` and the interface sources. VST is a trademark of Steinberg Media Technologies GmbH.

## WABT / wasm2c

Copyright WebAssembly Community Group participants. WABT 1.0.42: https://github.com/WebAssembly/wabt/tree/1.0.42. The linked runtime and its source are included under Apache-2.0; see `licenses/Apache-2.0-WABT.txt`. The generated engine C derives from the GPL-3.0-only FMini-1 module.

## Compiler libraries

LLVM-MinGW 20261104 UCRT: https://github.com/mstorsjo/llvm-mingw/releases/tag/20261104. Notices for the LLVM and MinGW libraries are retained in `licenses`. The compiler/toolchain binaries are not included in public distribution archives; setup downloads them separately.

## SAVIHost

SAVIHost 1.43 VST3 x64, copyright Hermann Seib: https://www.hermannseib.com/english/savihost.htm. It is **not included in public GitHub packages**. The optional local `-IncludeSAVIHost` package preserves the complete original package, `license.txt` and `history.txt`, and adds an executable copy named `FMini SMK-37.exe`. The host binary is unchanged.

SAVIHost remains under its separate **free private-use license**, not the plugin's GPL license. Complete corresponding plugin source is included in every public binary package and supplied as a separate release source archive.
