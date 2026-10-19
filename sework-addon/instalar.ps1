# Instala o addon Sework no FreeCAD do usuario atual (Windows).
# Uso: clique com o botao direito > "Executar com o PowerShell"
#      ou: powershell -ExecutionPolicy Bypass -File .\instalar.ps1
$ErrorActionPreference = "Stop"
$origem = Join-Path $PSScriptRoot "Sework"
$base   = Join-Path $env:APPDATA "FreeCAD"

if (-not (Test-Path $base)) {
    Write-Host "Pasta $base nao encontrada. Abra o FreeCAD uma vez e rode de novo." -ForegroundColor Yellow
    exit 1
}

# FreeCAD 1.0 usa %APPDATA%\FreeCAD\Mod; versoes mais novas usam subpastas por versao (ex.: v1-1\Mod)
$destinos = @(Join-Path $base "Mod")
Get-ChildItem $base -Directory -Filter "v*" -ErrorAction SilentlyContinue | ForEach-Object {
    $destinos += (Join-Path $_.FullName "Mod")
}

foreach ($mod in $destinos) {
    $alvo = Join-Path $mod "Sework"
    New-Item -ItemType Directory -Force -Path $mod | Out-Null
    if (Test-Path $alvo) { Remove-Item $alvo -Recurse -Force }
    Copy-Item $origem $alvo -Recurse
    Write-Host "Instalado em $alvo" -ForegroundColor Green
}
Write-Host "Pronto. Abra o FreeCAD, feche e abra de novo para ver splash e tema."
