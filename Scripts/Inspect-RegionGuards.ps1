[CmdletBinding()]
param(
    [string]$EngineRoot,
    [ValidateRange(1, 120)][int]$TimeoutMinutes = 20
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'Unreal-Environment.ps1')
$EngineRoot = Get-ShooterEngineRoot $EngineRoot
$projectRoot = Split-Path $PSScriptRoot -Parent
if (Get-Process UnrealEditor -ErrorAction SilentlyContinue) { throw 'Close Unreal Editor before running the region guard check.' }

# The check only reads assets and the level; it never saves anything.
# Both the tool and its output paths matter: the script is tracked, the report is not.
$script = Join-Path $projectRoot 'Scripts/Inspect-RegionGuards.py'
if (-not (Test-Path -LiteralPath $script)) { throw "Check script not found: $script" }

$reportDirectory = Join-Path $projectRoot 'Saved/Workflow/RegionGuardCheck'
New-Item -ItemType Directory -Force -Path $reportDirectory | Out-Null
$log = Join-Path $reportDirectory 'Inspect.log'
if (Test-Path -LiteralPath $log) { Remove-Item -LiteralPath $log -Force }

$project = Join-Path $projectRoot 'ShooterSamProject.uproject'
# The PythonScriptCommandlet reads the file from -Script, not from a bare argument.
$scriptArgument = '-Script=' + ($script -replace '\\', '/')
# The default DDC graph has no writable node in this environment; memory cache keeps
# the read-only inspection running without touching shared cache directories.
$arguments = @(
    "`"$project`"", '-run=pythonscript', "`"$scriptArgument`"",
    '-unattended', '-nop4', '-nosplash', '-nosound', '-stdout', '-FullStdOutLogOutput',
    '-DDC-ForceMemoryCache',
    "`"-abslog=$log`""
)

$process = Start-Process -FilePath (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') `
    -ArgumentList $arguments -PassThru -WindowStyle Hidden
if (-not $process.WaitForExit($TimeoutMinutes * 60000)) {
    Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
    throw "Region guard check timed out. See $log"
}
$process.WaitForExit()

if (-not (Test-Path -LiteralPath $log)) { throw "No engine log produced. Expected $log" }
$content = Get-Content -LiteralPath $log -Raw

$problems = [regex]::Matches($content, 'REGIONGUARD_PROBLEM (.+)') | ForEach-Object { $_.Groups[1].Value.Trim() }
$result = [regex]::Match($content, 'REGIONGUARD_RESULT (\S+)')

if (-not $result.Success) {
    $tail = ($content -split "`n" | Select-Object -Last 25) -join "`n"
    throw "The check never reached a result line. Log tail:`n$tail`nSee $log"
}

if ($result.Groups[1].Value -ne 'OK') {
    Write-Host "Problems found ($(@($problems).Count)):"
    $problems | ForEach-Object { Write-Host "  - $_" }
    throw "Region guard check reported problems. Report: $projectRoot/Saved/Automation/RegionGuards/region_guards_report.json"
}

Write-Host "PASS: region guard configuration looks consistent. Log: $log"
