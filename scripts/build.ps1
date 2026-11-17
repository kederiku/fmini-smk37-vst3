param([switch]$TestsOnly, [switch]$RegenerateEngine, [string]$NodeExecutable = 'node')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
$llvm = Join-Path $root 'tools\llvm-mingw-20261104-ucrt-x86_64\bin'
$wabt = Join-Path $root 'tools\wabt-1.0.42'
$runtime = Join-Path $root 'third_party\wasm2c-runtime'
$version = (Get-Content -LiteralPath (Join-Path $root 'VERSION') -Raw).Trim()
if ($version -notmatch '^\d+\.\d+\.\d+$') { throw 'VERSION must contain major.minor.patch' }
$env:Path = $llvm + ';' + $env:Path
New-Item -ItemType Directory -Force 'third_party\generated','build\obj','dist' | Out-Null
# Keep plugin metadata and release filenames tied to the same version file.
('#define FMINI_VERSION "' + $version + '"') | Set-Content -LiteralPath 'build\Version.h' -Encoding ascii
if ($RegenerateEngine -or !(Test-Path -LiteralPath 'third_party\generated\fmini.c')) {
    & "$wabt\bin\wasm2c.exe" 'third_party\fmini-1-46ff9987680b01d12522779448e8b7f07625093f\docs\emu\smk\felucca.wasm' -n fmini -o 'third_party\generated\fmini.c'
    if ($LASTEXITCODE -ne 0) { throw 'wasm2c failed (requires VC++ runtime and OpenSSL 3 on PATH)' }
}
& $NodeExecutable .\tests\reference.mjs
if ($LASTEXITCODE -ne 0) { throw 'Reference generation failed (Node.js 22 or newer required)' }
$flags = @('-O2','-ffp-contract=off','-DWASM_RT_USE_MMAP=0','-DWASM_RT_MEMCHECK_BOUNDS_CHECK=1','-DNDEBUG','-Isrc','-Ithird_party/generated',('-I'+$runtime))
$cSources = @('third_party\generated\fmini.c',"$runtime\wasm-rt-impl.c","$runtime\wasm-rt-mem-impl.c","$runtime\wasm-rt-exceptions-impl.c")
$objects = @()
foreach($source in $cSources) {
    $out = 'build\obj\' + [IO.Path]::GetFileNameWithoutExtension($source) + '.o'
    if (!(Test-Path -LiteralPath $out) -or (Get-Item -LiteralPath $source).LastWriteTime -gt (Get-Item -LiteralPath $out).LastWriteTime) {
        Write-Host "Compiling $source"
        & "$llvm\clang.exe" -std=c11 @flags -c $source -o $out
        if ($LASTEXITCODE -ne 0) { throw "Compile failed: $source" }
    }
    $objects += $out
}
& "$llvm\clang++.exe" -std=c++17 -static @flags src/Engine.cpp tests/engine_test.cpp @objects -o build/engine_test.exe
if ($LASTEXITCODE -ne 0) { throw 'Engine test build failed' }
& .\build\engine_test.exe
if ($LASTEXITCODE -ne 0) { throw 'Engine tests failed' }
& "$llvm\clang++.exe" -std=c++17 -static @flags src/Engine.cpp src/Audio.cpp tests/audio_test.cpp @objects -o build/audio_test.exe
if ($LASTEXITCODE -ne 0) { throw 'Audio test build failed' }
& .\build\audio_test.exe
if ($LASTEXITCODE -ne 0) { throw 'Audio tests failed' }
& "$llvm\clang++.exe" -std=c++17 -O2 -static -Isrc src/Panel.cpp tests/panel_test.cpp -o build/panel_test.exe
if ($LASTEXITCODE -ne 0) { throw 'Panel test build failed' }
& .\build\panel_test.exe
if ($LASTEXITCODE -ne 0) { throw 'Panel tests failed' }
if ($TestsOnly) { exit 0 }
$bundle='dist\FMini SMK-37.vst3\Contents\x86_64-win'
New-Item -ItemType Directory -Force $bundle | Out-Null
$cppFlags=@('-std=c++17','-static','-Wno-pragma-pack','-Ithird_party/sdk','-Ibuild')
& "$llvm\clang++.exe" @cppFlags @flags -shared src/Engine.cpp src/Audio.cpp src/Plugin.cpp src/Panel.cpp src/Editor.cpp src/EditorPaint.cpp @objects -luser32 -lgdi32 -lgdiplus '-Wl,--stack,8388608' -o "$bundle\FMini SMK-37.vst3"
if ($LASTEXITCODE -ne 0) { throw 'Plugin build failed' }
& "$llvm\clang++.exe" @cppFlags -O2 tests/plugin_test.cpp -o build/plugin_test.exe
if ($LASTEXITCODE -ne 0) { throw 'Host test build failed' }
& .\build\plugin_test.exe
if ($LASTEXITCODE -ne 0) { throw 'Host tests failed' }
# Publish evidence only after the complete integration test passes. TestsOnly or
# a failed build must never certify a stale binary for a new release version.
. (Join-Path $PSScriptRoot 'build-metadata.ps1')
[ordered]@{
    version = $version
    binarySha256 = (Get-FileHash -LiteralPath "$bundle\FMini SMK-37.vst3" -Algorithm SHA256).Hash
    sourceSha256 = Get-BuildFingerprint -Root $root
} | ConvertTo-Json | Set-Content -LiteralPath 'build\verified-build.json' -Encoding utf8
