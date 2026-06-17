[CmdletBinding()]
param(
    [switch]$Clean,
    [switch]$WhatIfPath
)

$ErrorActionPreference = 'Stop'

function Get-DeduplicatedPath {
    param([string]$RawPath)

    $seen = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
    $kept = [System.Collections.Generic.List[string]]::new()

    foreach ($segment in ($RawPath -split ';')) {
        $trimmed = $segment.Trim()
        if ([string]::IsNullOrWhiteSpace($trimmed)) { continue }
        $key = $trimmed.TrimEnd('\')
        if ($seen.Add($key)) { $kept.Add($trimmed) }
    }

    return ($kept -join ';')
}

$originalPath = $env:PATH
$originalLength = $originalPath.Length
$originalCount = ($originalPath -split ';' | Where-Object { $_.Trim() -ne '' }).Count

$cleanPath = Get-DeduplicatedPath -RawPath $originalPath
$cleanLength = $cleanPath.Length
$cleanCount = ($cleanPath -split ';').Count

Write-Host ("Original PATH: {0} chars, {1} entries" -f $originalLength, $originalCount)
Write-Host ("Cleaned  PATH: {0} chars, {1} entries" -f $cleanLength, $cleanCount)
Write-Host ("Removed {0} duplicate entries, saved {1} chars" -f ($originalCount - $cleanCount), ($originalLength - $cleanLength))

if ($WhatIfPath) {
    Write-Host ""
    Write-Host "Resulting PATH entries:"
    foreach ($entry in ($cleanPath -split ';')) { Write-Host ("  " + $entry) }
    return
}

$env:PATH = $cleanPath

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
Push-Location $scriptRoot
try {
    if ($Clean) {
        Write-Host ""
        Write-Host "Running: pixi clean"
        & pixi clean
        if ($LASTEXITCODE -ne 0) { throw "pixi clean failed with exit code $LASTEXITCODE" }
    }

    Write-Host ""
    Write-Host "Running: pixi install"
    & pixi install
    if ($LASTEXITCODE -ne 0) { throw "pixi install failed with exit code $LASTEXITCODE" }

    Write-Host ""
    Write-Host "Build completed."
}
finally {
    Pop-Location
    $env:PATH = $originalPath
}