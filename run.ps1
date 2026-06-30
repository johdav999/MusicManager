$ErrorActionPreference = "Stop"

$EngineRoot = "C:\Program Files\Epic Games\UE_5.6\Engine"
$ProjectPath = Join-Path $PSScriptRoot "MusicManager.uproject"
$BuildTool = Join-Path $EngineRoot "Build\BatchFiles\Build.bat"
$Editor = Join-Path $EngineRoot "Binaries\Win64\UnrealEditor.exe"

if (-not (Test-Path -LiteralPath $ProjectPath)) {
    throw "Project file not found: $ProjectPath"
}

if (-not (Test-Path -LiteralPath $BuildTool)) {
    throw "Unreal build tool not found: $BuildTool"
}

if (-not (Test-Path -LiteralPath $Editor)) {
    throw "Unreal editor not found: $Editor"
}

Write-Host "Building MusicManagerEditor..."
& $BuildTool "MusicManagerEditor" "Win64" "Development" "-Project=$ProjectPath" "-WaitMutex" "-NoHotReloadFromIDE"

if ($LASTEXITCODE -ne 0) {
    throw "Build failed with exit code $LASTEXITCODE"
}

Write-Host "Starting Unreal Editor..."
& $Editor $ProjectPath
