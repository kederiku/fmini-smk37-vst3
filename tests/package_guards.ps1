# Exercise stale-release rejection with a disposable miniature project. No live
# VERSION file, plugin binary or user preset is modified by these checks.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$stage = Join-Path $root ('build\guard-test-' + [guid]::NewGuid().ToString('N'))
$binaryFolder = Join-Path $stage 'dist\FMini SMK-37.vst3\Contents\x86_64-win'
New-Item -ItemType Directory -Force $binaryFolder,(Join-Path $stage 'scripts'),(Join-Path $stage 'build') | Out-Null
foreach ($file in @('package.ps1','source-package.ps1','build-metadata.ps1')) { Copy-Item -LiteralPath (Join-Path $root "scripts\$file") -Destination (Join-Path $stage 'scripts') }
Set-Content -LiteralPath (Join-Path $stage 'VERSION') -Value '0.1.0'
$binary = Join-Path $binaryFolder 'FMini SMK-37.vst3'
Set-Content -LiteralPath $binary -Value 'test fixture, not an executable'
function Expect-Rejection([string]$message) {
    $rejected = $false
    try { & (Join-Path $stage 'scripts\package.ps1') }
    catch { if (!$_.Exception.Message.Contains($message)) { throw }; $rejected = $true }
    finally { Set-Location $root }
    if (!$rejected) { throw "Expected rejection: $message" }
}
Expect-Rejection 'Run the full build'
$record = @{ version='0.1.0'; binarySha256=(Get-FileHash -LiteralPath $binary).Hash; sourceSha256='unused' }
$record | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $stage 'build\verified-build.json')
# Simulate VERSION + Version.h being updated by a TestsOnly run, without a DLL rebuild.
Set-Content -LiteralPath (Join-Path $stage 'VERSION') -Value '0.2.0'
Set-Content -LiteralPath (Join-Path $stage 'build\Version.h') -Value '#define FMINI_VERSION "0.2.0"'
Expect-Rejection 'Rebuild after changing VERSION'
Set-Content -LiteralPath (Join-Path $stage 'VERSION') -Value '0.1.0'
Add-Content -LiteralPath $binary -Value 'changed after verification'
Expect-Rejection 'Binary differs from the tested build'
foreach ($directory in @('src','third_party\generated','third_party\wasm2c-runtime','third_party\sdk')) { New-Item -ItemType Directory -Force (Join-Path $stage $directory) | Out-Null }
Copy-Item -LiteralPath (Join-Path $root 'scripts\build.ps1') -Destination (Join-Path $stage 'scripts')
$record.binarySha256 = (Get-FileHash -LiteralPath $binary).Hash
$record | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $stage 'build\verified-build.json')
Expect-Rejection 'Sources changed since the tested build'
Write-Output 'PASS package guards: unverified build, stale version, changed binary and changed sources rejected'
