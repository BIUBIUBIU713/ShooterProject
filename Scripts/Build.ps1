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
# UBT locates linker response files under %TEMP%. Pointing it at a short
# project-local folder keeps large link commands off long user temp paths.
# (The DSH sandbox injects its own session temp dir, which this cannot override.)
$tempDirectory = Join-Path $projectRoot 'Saved\WT'
New-Item -ItemType Directory -Force -Path $tempDirectory | Out-Null
# Under StrictMode, reading an undefined environment variable throws.
$hasTemp = Test-Path Env:TEMP
$hasTmp = Test-Path Env:TMP
if ($hasTemp) { $originalTemp = $env:TEMP }
if ($hasTmp) { $originalTmp = $env:TMP }
$env:TEMP = $tempDirectory
$env:TMP = $tempDirectory
& (Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat') ShooterSamProjectEditor Win64 Development "-Project=$project" -WaitMutex -NoHotReloadFromIDE -NoEngineChanges "-Log=$(Join-Path $logDirectory 'Build.log')" -NoUBA
if ($hasTemp) { $env:TEMP = $originalTemp } else { Remove-Item Env:TEMP -ErrorAction SilentlyContinue }
if ($hasTmp) { $env:TMP = $originalTmp } else { Remove-Item Env:TMP -ErrorAction SilentlyContinue }
if ($LASTEXITCODE -ne 0) { throw "UE build failed (exit $LASTEXITCODE). See Saved/Workflow/Build.log." }
Write-Host 'PASS: ShooterSamProjectEditor Win64 Development build.'
