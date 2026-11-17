param([switch]$IncludeSAVIHost)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
. (Join-Path $PSScriptRoot 'source-package.ps1')
$version = (Get-Content -LiteralPath (Join-Path $root 'VERSION') -Raw).Trim()
if ($version -notmatch '^\d+\.\d+\.\d+$') { throw 'Invalid VERSION' }
$suffix = if ($IncludeSAVIHost) { '-SAVIHost' } else { '' }
$binary = Join-Path $root 'dist\FMini SMK-37.vst3\Contents\x86_64-win\FMini SMK-37.vst3'
if (!(Test-Path -LiteralPath $binary)) { throw 'Run scripts/build.ps1 first' }
if (!(Test-Path -LiteralPath 'build\verified-build.json')) { throw 'Run the full build and VST3 tests before packaging' }
$verified = Get-Content -LiteralPath 'build\verified-build.json' -Raw | ConvertFrom-Json
if ($verified.version -ne $version) { throw 'Rebuild after changing VERSION' }
if ($verified.binarySha256 -ne (Get-FileHash -LiteralPath $binary -Algorithm SHA256).Hash) { throw 'Binary differs from the tested build; rebuild before packaging' }
. (Join-Path $PSScriptRoot 'build-metadata.ps1')
if ($verified.sourceSha256 -ne (Get-BuildFingerprint -Root $root)) { throw 'Sources changed since the tested build; rebuild before packaging' }
# A fresh staging directory prevents an earlier private SAVIHost package from
# leaking into a later public release. It also preserves any running local host.
$stage = Join-Path $root ('build\package-' + [guid]::NewGuid().ToString('N'))
$source = Join-Path $stage 'source'
$release = Join-Path $stage 'release'
Copy-RepositorySource -Root $root -Target $source
$upstream = 'fmini-1-46ff9987680b01d12522779448e8b7f07625093f'
if (!(Test-Path -LiteralPath "third_party\$upstream\firmware\engines\LICENSE")) { throw 'Complete upstream sources are missing; run setup.ps1' }
Copy-Item -LiteralPath (Join-Path $root "third_party\$upstream") -Destination (Join-Path $source 'third_party') -Recurse
New-Item -ItemType Directory -Force (Join-Path $release 'VST3'),(Join-Path $release 'Sources'),(Join-Path $release 'docs') | Out-Null
$sourceArchive = Join-Path $root "dist\FMini-SMK-37-$version$suffix-sources.zip"
Write-Zip -Source $source -Destination $sourceArchive
Copy-Item -LiteralPath $sourceArchive -Destination (Join-Path $release 'Sources')
Copy-Item -LiteralPath (Join-Path $root 'dist\FMini SMK-37.vst3') -Destination (Join-Path $release 'VST3') -Recurse
foreach ($name in @('README.md','README.fr.md','THIRD_PARTY_NOTICES.md','THIRD_PARTY_NOTICES.fr.md','LICENSE','licenses','VERSION','CHANGELOG.md','CONTRIBUTING.md','CONTRIBUTING.fr.md','.github')) {
    Copy-Item -LiteralPath (Join-Path $root $name) -Destination $release -Recurse
}
foreach ($name in @('en','fr','verification.md','verification.en.md')) {
    Copy-Item -LiteralPath (Join-Path $root "docs\$name") -Destination (Join-Path $release 'docs') -Recurse
}
if ($IncludeSAVIHost) {
    if (!(Test-Path -LiteralPath 'tools\savihost\savihost.exe')) { throw 'Run setup.ps1 -IncludeSAVIHost first' }
    $hostFolder = Join-Path $release 'SAVIHost'
    New-Item -ItemType Directory -Force $hostFolder | Out-Null
    Copy-Item -Path (Join-Path $root 'tools\savihost\*') -Destination $hostFolder
    Copy-Item -LiteralPath 'tools\savihost\savihost.exe' -Destination (Join-Path $hostFolder 'FMini SMK-37.exe')
    Copy-Item -LiteralPath $binary -Destination (Join-Path $hostFolder 'FMini SMK-37.vst3')
}
$manifest = Get-ChildItem -LiteralPath $release -File -Recurse | Sort-Object FullName | ForEach-Object {
    $hash = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    "$hash  $($_.FullName.Substring($release.Length+1).Replace('\','/'))"
}
$manifest | Set-Content -LiteralPath (Join-Path $release 'SHA256SUMS.txt') -Encoding utf8
$archive = Join-Path $root "dist\FMini-SMK-37-$version-Windows-x64$suffix.zip"
Write-Zip -Source $release -Destination $archive
# Release-level hashes cover both the binary and the separately downloadable source.
@($archive,$sourceArchive) | ForEach-Object { "$( (Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash )  $([IO.Path]::GetFileName($_))" } | Set-Content -LiteralPath (Join-Path $root "dist\FMini-SMK-37-$version$suffix-SHA256SUMS.txt") -Encoding utf8
Write-Output "Windows archive: $archive"
Write-Output "Complete source archive: $sourceArchive"
