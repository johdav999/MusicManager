$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$bundledNode = Join-Path $env:USERPROFILE ".cache\codex-runtimes\codex-primary-runtime\dependencies\node\bin\node.exe"
$node = if (Test-Path -LiteralPath $bundledNode) { $bundledNode } else { "node" }
$server = Join-Path $repoRoot "EditorServer\server.js"

if (-not (Test-Path -LiteralPath $server)) {
    throw "Editor server was not found at $server"
}

Write-Host "Starting MusicManager editor at http://127.0.0.1:5177"
& $node $server
