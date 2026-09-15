[CmdletBinding()]
param([string]$EngineRoot, [switch]$ValidateOnly)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'Unreal-Environment.ps1')
$EngineRoot = Get-ShooterEngineRoot $EngineRoot
$projectRoot = Split-Path $PSScriptRoot -Parent
$project = Join-Path $projectRoot 'ShooterSamProject.uproject'
if ($ValidateOnly) {
    Write-Host "PASS: build environment found at $EngineRoot"
    return
}
if (Get-Process UnrealEditor -ErrorAction SilentlyContinue) { throw 'Close Unreal Editor before building (avoid Live Coding conflicts).' }
$logDirectory = Join-Path $projectRoot 'Saved/Workflow'
New-Item -ItemType Directory -Force -Path $logDirectory | Out-Null
& (Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat') ShooterSamProjectEditor Win64 Development "-Project=$project" -WaitMutex -NoHotReloadFromIDE -NoEngineChanges "-Log=$(Join-Path $logDirectory 'Build.log')"
if ($LASTEXITCODE -ne 0) { throw "UE build failed (exit $LASTEXITCODE). See Saved/Workflow/Build.log." }
Write-Host 'PASS: ShooterSamProjectEditor Win64 Development build.'
