param([string]$Archive)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$version = (Get-Content -LiteralPath (Join-Path $root 'VERSION') -Raw).Trim()
if (!$Archive) { $Archive = Join-Path $root "dist\FMini-SMK-37-$version-Windows-x64.zip" }
Add-Type -AssemblyName System.IO.Compression.FileSystem
function Read-Entry($zip, [string]$name) {
    $entry = $zip.GetEntry($name)
    if (!$entry) { throw "Missing archive entry: $name" }
    $inputStream = $entry.Open()
    $memory = New-Object IO.MemoryStream
    try { $inputStream.CopyTo($memory); return ,$memory.ToArray() }
    finally { $inputStream.Dispose(); $memory.Dispose() }
}
function Hash-Bytes([byte[]]$bytes) {
    $sha = [Security.Cryptography.SHA256]::Create()
    try { return ([BitConverter]::ToString($sha.ComputeHash($bytes))).Replace('-', '') }
    finally { $sha.Dispose() }
}
$zip = [IO.Compression.ZipFile]::OpenRead((Resolve-Path -LiteralPath $Archive).Path)
try {
    if ($zip.Entries.FullName -match '(?i)(^|/)SAVIHost/') { throw 'SAVIHost must not be in a public release' }
    $manifest = [Text.Encoding]::UTF8.GetString((Read-Entry $zip 'SHA256SUMS.txt')).TrimStart([char]0xfeff)
    $listed = @{}
    foreach ($line in ($manifest -split '\r?\n')) {
        if (!$line) { continue }
        if ($line -notmatch '^([A-F0-9]{64})  (.+)$') { throw 'Malformed checksum manifest' }
        $expected = $Matches[1]; $path = $Matches[2]
        if ((Hash-Bytes (Read-Entry $zip $path)) -ne $expected) { throw "Checksum mismatch: $path" }
        $listed[$path] = $true
    }
    foreach ($entry in $zip.Entries) {
        if ($entry.Name -and $entry.FullName -ne 'SHA256SUMS.txt' -and !$listed.ContainsKey($entry.FullName)) { throw "Unlisted archive file: $($entry.FullName)" }
    }
    $plugin = 'VST3/FMini SMK-37.vst3/Contents/x86_64-win/FMini SMK-37.vst3'
    $builtHash = (Get-FileHash -LiteralPath (Join-Path $root 'dist\FMini SMK-37.vst3\Contents\x86_64-win\FMini SMK-37.vst3')).Hash
    if ((Hash-Bytes (Read-Entry $zip $plugin)) -ne $builtHash) { throw 'Packaged binary differs from built/tested binary' }
    $sourceBytes = Read-Entry $zip "Sources/FMini-SMK-37-$version-sources.zip"
    $standaloneHash = (Get-FileHash -LiteralPath (Join-Path $root "dist\FMini-SMK-37-$version-sources.zip")).Hash
    if ((Hash-Bytes $sourceBytes) -ne $standaloneHash) { throw 'Embedded and standalone source archives differ' }
    $memory = New-Object IO.MemoryStream(,$sourceBytes)
    $sources = New-Object IO.Compression.ZipArchive($memory, [IO.Compression.ZipArchiveMode]::Read)
    try {
        $required = @('.github/workflows/ci.yml','.github/workflows/release.yml','.gitignore','VERSION',
            'README.md','README.fr.md','LICENSE','src/Plugin.cpp','scripts/build.ps1','scripts/setup.ps1',
            'scripts/package.ps1','tests/plugin_test.cpp','third_party/generated/fmini.c',
            'third_party/wasm2c-runtime/wasm-rt-impl.c','third_party/sdk/pluginterfaces/LICENSE.txt',
            'third_party/fmini-1-46ff9987680b01d12522779448e8b7f07625093f/firmware/engines/LICENSE')
        foreach ($name in $required) {
            $bytes = Read-Entry $sources $name
            $originalHash = (Get-FileHash -LiteralPath (Join-Path $root $name)).Hash
            if ((Hash-Bytes $bytes) -ne $originalHash) { throw "Missing or stale corresponding source: $name" }
        }
        if ($sources.Entries.FullName -match '^(tools|build|dist|\.git)/|^docs/superpowers/') { throw 'Source archive contains local-only files' }
    } finally { $sources.Dispose(); $memory.Dispose() }
    Write-Output 'PASS public package: checksums, tested VST3, bilingual docs, workflows, complete sources, no SAVIHost'
} finally { $zip.Dispose() }
