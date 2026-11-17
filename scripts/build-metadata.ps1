# Hash the compile inputs so packaging cannot combine a tested old binary with
# edited sources. Include flags/version scripts as well as C/C++ dependencies.
function Get-BuildFingerprint([string]$Root) {
    $files = @()
    foreach ($directory in @('src','third_party\generated','third_party\wasm2c-runtime','third_party\sdk')) {
        $files += Get-ChildItem -LiteralPath (Join-Path $Root $directory) -File -Recurse | Where-Object { $_.Extension -in '.cpp','.c','.h','.inc' }
    }
    foreach ($name in @('VERSION','scripts\build.ps1','scripts\build-metadata.ps1')) { $files += Get-Item -LiteralPath (Join-Path $Root $name) }
    $lines = $files | Sort-Object FullName | ForEach-Object { "$($_.FullName.Substring($Root.Length+1).Replace('\','/')) $((Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash)" }
    $sha = [Security.Cryptography.SHA256]::Create()
    try { return ([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes(($lines -join "`n"))))).Replace('-','') }
    finally { $sha.Dispose() }
}
