# Run after git add. This validates the index (the next commit), not unsaved work.
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$projectRoot = Split-Path $PSScriptRoot -Parent
Push-Location $projectRoot
try {
    $gitRoot = & git rev-parse --show-toplevel
    if ($LASTEXITCODE -ne 0 -or [IO.Path]::GetFullPath($gitRoot) -ne $projectRoot) {
        throw 'Git root must be this project directory.'
    }
    $entries = @(& git -c core.quotepath=false ls-files --stage)
    if ($LASTEXITCODE -ne 0) { throw 'Cannot read the Git index.' }
    if ($entries.Count -eq 0) { throw 'The index is empty. Stage the intended files first.' }
    $files = @{}
    foreach ($entry in $entries) {
        if ($entry -notmatch '^\d+ ([0-9a-f]+) 0\t(.+)$') { throw "Unmerged index entry: $entry" }
        $files[$Matches[2]] = $Matches[1]
    }
    $forbidden = @($files.Keys | Where-Object {
        $_ -match '(^|/)(Binaries|Intermediate|Saved|DerivedDataCache|\.vs|\.idea)/' -or
        $_ -match '\.(sln|slnx|suo|user|VC\.db|VC\.opendb)$' -or
        ($_ -match '(^|/)\.env($|\.)' -and $_ -notmatch '(^|/)\.env\.example$')
    })
    if ($forbidden.Count) { throw "Generated/local files are tracked:`n$($forbidden -join "`n")" }

    & git grep --cached -n -I -E '^(<<<<<<< |=======$|>>>>>>> )' -- Source Config Scripts Docs '*.uproject'
    if ($LASTEXITCODE -eq 0) { throw 'Possible unresolved conflict markers found above.' }
    if ($LASTEXITCODE -ne 1) { throw 'Conflict marker check failed.' }

    # Windows PowerShell sends CRLF to native stdin; check-attr treats the CR
    # as part of a filename. Pass small argument batches instead.
    $paths = @($files.Keys)
    $attributes = @(for ($offset = 0; $offset -lt $paths.Count; $offset += 64) {
        $last = [Math]::Min($offset + 63, $paths.Count - 1)
        $batch = @($paths[$offset..$last])
        & git -c core.quotepath=false check-attr --cached filter -- @batch
        if ($LASTEXITCODE -ne 0) { throw 'Cannot read staged LFS attributes.' }
    })
    $lfsOids = @{}
    foreach ($attribute in $attributes) {
        if ($attribute -notmatch '^(.*): filter: (.*)$') { throw "Unexpected attribute: $attribute" }
        $path = $Matches[1]
        $filter = $Matches[2]
        if (-not $files.ContainsKey($path)) { throw "Cannot match attribute to an indexed path: $path" }
        if ($path -match '\.(uasset|umap)$' -and $filter -ne 'lfs') {
            throw "UE asset is not configured for LFS: $path"
        }
        if ($filter -eq 'lfs') { $lfsOids[$files[$path]] = $true }
    }
    $oids = @($files.Values | Sort-Object -Unique)
    $sizes = @($oids | & git cat-file '--batch-check=%(objectname) %(objecttype) %(objectsize)')
    if ($LASTEXITCODE -ne 0) { throw 'Cannot inspect staged objects.' }
    foreach ($line in $sizes) {
        if ($line -notmatch '^([0-9a-f]+) blob (\d+)$') { throw "Unexpected staged object: $line" }
        $oid = $Matches[1]
        $size = [long]$Matches[2]
        if ($lfsOids.ContainsKey($oid) -and $size -gt 1024) { throw "Raw asset staged instead of LFS pointer: $oid" }
        if (-not $lfsOids.ContainsKey($oid) -and $size -gt 10MB) { throw "Non-LFS object exceeds 10 MiB: $oid. Review its path and LFS rules." }
    }
    if ($lfsOids.Count) {
        $pointerLines = @($lfsOids.Keys | & git cat-file --batch)
        if ($LASTEXITCODE -ne 0) { throw 'Cannot read staged LFS pointers.' }
        $pointerText = ($pointerLines -join "`n") + "`n"
        $pattern = '(?m)^[0-9a-f]+ blob \d+\nversion https://git-lfs.github.com/spec/v1\noid sha256:[0-9a-f]{64}\nsize \d+\n\n'
        $validPointers = [regex]::Matches($pointerText, $pattern)
        if ($validPointers.Count -ne $lfsOids.Count -or [regex]::Replace($pointerText, $pattern, '').Length -ne 0) {
            throw 'Invalid/noncanonical LFS pointer in the index. Re-add affected files after installing LFS.'
        }
    }
    Write-Host "PASS: $($files.Count) staged/tracked files; $($lfsOids.Count) unique LFS pointers."
} finally {
    Pop-Location
}
