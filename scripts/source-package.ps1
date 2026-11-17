# Shared allowlist for the public repository and complete corresponding-source ZIP.
# Local build tools, binaries, personal configuration and working notes stay out.
function Copy-RepositorySource([string]$Root, [string]$Target) {
    New-Item -ItemType Directory -Force $Target | Out-Null
    $entries = @('.github', '.gitignore', '.gitattributes', 'VERSION', 'LICENSE',
        'README.md', 'README.fr.md', 'CONTRIBUTING.md', 'CONTRIBUTING.fr.md',
        'CHANGELOG.md', 'THIRD_PARTY_NOTICES.md', 'THIRD_PARTY_NOTICES.fr.md',
        'src', 'tests', 'scripts', 'licenses', 'docs\en', 'docs\fr',
        'docs\verification.md', 'docs\verification.en.md',
        'third_party\sdk', 'third_party\generated', 'third_party\wasm2c-runtime',
        'third_party\downloads.sha256.json')
    foreach ($entry in $entries) {
        $destination = Join-Path $Target $entry
        New-Item -ItemType Directory -Force (Split-Path -Parent $destination) | Out-Null
        Copy-Item -LiteralPath (Join-Path $Root $entry) -Destination $destination -Recurse -Force
    }
}

function Write-Zip([string]$Source, [string]$Destination) {
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    # ZipFile includes dotfiles such as .github and .gitignore on every platform.
    $temporary = $Destination + '.' + [guid]::NewGuid().ToString('N') + '.tmp'
    [IO.Compression.ZipFile]::CreateFromDirectory($Source, $temporary, [IO.Compression.CompressionLevel]::Optimal, $false)
    Move-Item -LiteralPath $temporary -Destination $Destination -Force
}
