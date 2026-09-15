# Shared by Build.ps1 and Test.ps1. No machine-specific paths are committed.
function Get-ShooterEngineRoot {
    param([string]$EngineRoot)
    if (-not $EngineRoot) { $EngineRoot = $env:UE_ROOT }
    if (-not $EngineRoot) {
        $project = Get-Content (Join-Path (Split-Path $PSScriptRoot -Parent) 'ShooterSamProject.uproject') -Raw | ConvertFrom-Json
        $manifest = Join-Path $env:ProgramData 'Epic/UnrealEngineLauncher/LauncherInstalled.dat'
        if (Test-Path -LiteralPath $manifest) {
            $installations = (Get-Content $manifest -Raw | ConvertFrom-Json).InstallationList
            $installation = $installations | Where-Object { $_.AppName -eq "UE_$($project.EngineAssociation)" } | Select-Object -First 1
            if ($installation) { $EngineRoot = $installation.InstallLocation }
        }
    }
    if (-not $EngineRoot) { throw 'UE installation not found. Pass -EngineRoot or set UE_ROOT to the UE_5.7 directory.' }
    $EngineRoot = [IO.Path]::GetFullPath($EngineRoot)
    foreach ($relative in @('Engine/Build/BatchFiles/Build.bat', 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe')) {
        if (-not (Test-Path -LiteralPath (Join-Path $EngineRoot $relative))) { throw "Missing UE tool: $relative in $EngineRoot" }
    }
    return $EngineRoot
}
