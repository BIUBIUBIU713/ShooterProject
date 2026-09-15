[CmdletBinding()]
param(
    [string]$EngineRoot,
    [ValidatePattern('^ShooterSam\.[A-Za-z0-9_.]*$')][string]$Filter = 'ShooterSam.',
    [switch]$SkipBuild,
    [switch]$ValidateOnly,
    [ValidateRange(1, 120)][int]$TimeoutMinutes = 15
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'Unreal-Environment.ps1')
$EngineRoot = Get-ShooterEngineRoot $EngineRoot
$projectRoot = Split-Path $PSScriptRoot -Parent
if ($ValidateOnly) {
    Write-Host "PASS: test environment found at $EngineRoot; filter=$Filter"
    return
}
if (Get-Process UnrealEditor -ErrorAction SilentlyContinue) { throw 'Close Unreal Editor before running automation tests.' }
if (-not $SkipBuild) { & (Join-Path $PSScriptRoot 'Build.ps1') -EngineRoot $EngineRoot }
# A fresh directory prevents old reports from being mistaken for new results.
$runId = (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N').Substring(0, 8)
$reportDirectory = Join-Path $projectRoot "Saved/Workflow/Tests/$runId"
New-Item -ItemType Directory -Force -Path $reportDirectory | Out-Null
$project = Join-Path $projectRoot 'ShooterSamProject.uproject'
$log = Join-Path $reportDirectory 'Automation.log'
$arguments = @(
    "`"$project`"", '/Engine/Maps/Entry', '-unattended', '-nop4', '-nosplash', '-nullrhi', '-nosound',
    "`"-ExecCmds=Automation RunTests $Filter`"", '"-TestExit=Automation Test Queue Empty"',
    "`"-ReportExportPath=$reportDirectory`"", "`"-abslog=$log`""
)
$process = Start-Process -FilePath (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe') -ArgumentList $arguments -PassThru -WindowStyle Hidden
if (-not $process.WaitForExit($TimeoutMinutes * 60000)) {
    Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
    throw "Automation timed out. See $log"
}
$process.WaitForExit()
if ($process.ExitCode -ne 0) { throw "Automation exited with $($process.ExitCode). See $log" }
$reportPath = Join-Path $reportDirectory 'index.json'
if (-not (Test-Path $reportPath)) { throw "No test report produced. See $log" }
$report = Get-Content $reportPath -Raw | ConvertFrom-Json
$passed = [int]$report.succeeded + [int]$report.succeededWithWarnings
if ($passed -eq 0 -or [int]$report.failed -ne 0 -or [int]$report.notRun -ne 0 -or [int]$report.inProcess -ne 0) {
    throw "Tests failed, incomplete, or no tests matched. See $reportPath"
}
Write-Host "PASS: $passed tests ($($report.succeededWithWarnings) with warnings). Report: $reportPath"
