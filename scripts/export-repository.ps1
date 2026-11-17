$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'source-package.ps1')
$stage = Join-Path $root ('build\github-export-' + [guid]::NewGuid().ToString('N'))
Copy-RepositorySource -Root $root -Target $stage
New-Item -ItemType Directory -Force (Join-Path $root 'dist') | Out-Null
$archive = Join-Path $root 'dist\FMini-SMK-37-github.zip'
Write-Zip -Source $stage -Destination $archive
Write-Output "GitHub repository archive: $archive"
