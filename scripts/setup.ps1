param([switch]$IncludeSAVIHost)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$cache = Join-Path $root 'tools\downloads'
New-Item -ItemType Directory -Force $cache, (Join-Path $root 'third_party'), (Join-Path $root 'build') | Out-Null
# Verify both cached and newly downloaded files against the reviewed dependency lock.
$checksums = @{}
Get-Content -LiteralPath (Join-Path $root 'third_party\downloads.sha256.json') -Raw | ConvertFrom-Json | ForEach-Object { $checksums[$_.file] = $_.Hash }
function Fetch($url, $name) {
    $out = Join-Path $cache $name
    if (!(Test-Path -LiteralPath $out)) {
        Write-Host "Downloading $name"
        Invoke-WebRequest -Uri $url -OutFile $out -UseBasicParsing
    }
    if (!$checksums.ContainsKey($name)) { throw "Dependency is not pinned: $name" }
    if ((Get-FileHash -LiteralPath $out -Algorithm SHA256).Hash -ne $checksums[$name]) { throw "SHA256 mismatch: $name. Remove the cached download and retry; do not update the lock without reviewing the source." }
    return $out
}
function Unpack($archive, $target, $marker) {
    if (!(Test-Path -LiteralPath (Join-Path $target $marker))) {
        New-Item -ItemType Directory -Force $target | Out-Null
        & tar.exe -xf $archive -C $target
        if ($LASTEXITCODE -ne 0) { throw "Extraction failed: $archive" }
    }
}
$llvm = Fetch 'https://github.com/mstorsjo/llvm-mingw/releases/download/20261104/llvm-mingw-20261104-ucrt-x86_64.zip' 'llvm-mingw-20261104-ucrt-x86_64.zip'
Unpack $llvm (Join-Path $root 'tools') 'llvm-mingw-20261104-ucrt-x86_64\bin\clang.exe'
$wabt = Fetch 'https://github.com/WebAssembly/wabt/releases/download/1.0.42/wabt-1.0.42-windows-x64.tar.gz' 'wabt-1.0.42-windows-x64.tar.gz'
Unpack $wabt (Join-Path $root 'tools') 'wabt-1.0.42\bin\wasm2c.exe'
$upstream = Fetch 'https://codeload.github.com/ruppert/fmini-1/zip/46ff9987680b01d12522779448e8b7f07625093f' 'fmini-1-46ff9987.zip'
Unpack $upstream (Join-Path $root 'third_party') 'fmini-1-46ff9987680b01d12522779448e8b7f07625093f\LICENSE'
$sdk = Fetch 'https://codeload.github.com/steinbergmedia/vst3_pluginterfaces/zip/4f547e8e102b47de4a8b8aaf343c73b700786372' 'vst3-pluginterfaces-4f547e8e.zip'
Unpack $sdk (Join-Path $root 'third_party') 'vst3_pluginterfaces-4f547e8e102b47de4a8b8aaf343c73b700786372\base\funknown.h'
$sdkTarget = Join-Path $root 'third_party\sdk'
New-Item -ItemType Directory -Force $sdkTarget | Out-Null
if (!(Test-Path -LiteralPath (Join-Path $sdkTarget 'pluginterfaces\base\funknown.h'))) {
    Copy-Item -LiteralPath (Join-Path $root 'third_party\vst3_pluginterfaces-4f547e8e102b47de4a8b8aaf343c73b700786372') -Destination (Join-Path $sdkTarget 'pluginterfaces') -Recurse
}
$engines = Fetch 'https://codeload.github.com/hugelton/Felucca-Engines/zip/c46e0086f69bdc26df1cd144dca9e683cc4531dd' 'felucca-engines-c46e0086.zip'
Unpack $engines (Join-Path $root 'third_party') 'Felucca-Engines-c46e0086f69bdc26df1cd144dca9e683cc4531dd\LICENSE'
$engineTarget = Join-Path $root 'third_party\fmini-1-46ff9987680b01d12522779448e8b7f07625093f\firmware\engines'
if (!(Test-Path -LiteralPath (Join-Path $engineTarget 'LICENSE'))) {
    New-Item -ItemType Directory -Force $engineTarget | Out-Null
    Copy-Item -Path (Join-Path $root 'third_party\Felucca-Engines-c46e0086f69bdc26df1cd144dca9e683cc4531dd\*') -Destination $engineTarget -Recurse -Force
}
$runtime = Join-Path $root 'third_party\wasm2c-runtime'
if (!(Test-Path -LiteralPath (Join-Path $runtime 'wasm-rt-impl.c'))) {
    New-Item -ItemType Directory -Force $runtime | Out-Null
    Copy-Item -Path (Join-Path $root 'tools\wabt-1.0.42\share\wabt\wasm2c\*') -Destination $runtime
    Copy-Item -Path (Join-Path $root 'tools\wabt-1.0.42\include\wasm-rt*.h') -Destination $runtime
}
$wabtLicense = Fetch 'https://raw.githubusercontent.com/WebAssembly/wabt/1.0.42/LICENSE' 'wabt-1.0.42-LICENSE'
Copy-Item -LiteralPath $wabtLicense -Destination (Join-Path $runtime 'LICENSE') -Force
if ($IncludeSAVIHost) {
    $savi = Fetch 'https://www.hermannseib.com/programs/savihost3x64.zip' 'savihost3x64.zip'
    Unpack $savi (Join-Path $root 'tools\savihost') 'savihost.exe'
}
Write-Host 'Portable dependencies ready.'
