param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("configure", "build", "check")]
    [string]$Action
)

$ErrorActionPreference = "Stop"
$automateRoot = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $automateRoot

function Enable-MsvcEnvironment {
    $vswhereCandidates = @(
        (Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"),
        (Join-Path $env:ProgramFiles "Microsoft Visual Studio\Installer\vswhere.exe")
    )
    $vswhere = $vswhereCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
    if (-not $vswhere) {
        throw "vswhere.exe was not found. Install Visual Studio 2022 with Desktop development with C++."
    }

    $installationPath = (& $vswhere -latest -products * `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -property installationPath).Trim()
    if (-not $installationPath) {
        throw "No Visual Studio installation with the x64 C++ toolchain was found."
    }

    $vsDevCmd = Join-Path $installationPath "Common7\Tools\VsDevCmd.bat"
    if (-not (Test-Path -LiteralPath $vsDevCmd)) {
        throw "Visual Studio developer environment script was not found: $vsDevCmd"
    }

    # Import the environment produced by VsDevCmd into this PowerShell process
    # so CMake/Ninja can find cl.exe, link.exe, rc.exe, and the Windows SDK.
    $activationCommand = "`"$vsDevCmd`" -no_logo -arch=x64 -host_arch=x64 && set"
    $environmentLines = & cmd.exe /d /s /c $activationCommand
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to activate the Visual Studio x64 developer environment."
    }
    foreach ($line in $environmentLines) {
        if ($line -match "^([^=]+)=(.*)$") {
            Set-Item -Path "Env:$($matches[1])" -Value $matches[2]
        }
    }

    Write-Output "MSVC environment: $installationPath"
}

switch ($Action) {
    "configure" {
        Enable-MsvcEnvironment
        $aiPrefix = (& python -c "import sys; print(sys.prefix)").Trim()
        if ($LASTEXITCODE -ne 0 -or -not $aiPrefix) {
            throw "Failed to determine the AutoMate Pixi environment prefix."
        }

        & cmake -S . -B build-ai -G Ninja `
            -DCMAKE_BUILD_TYPE=Release `
            "-DCMAKE_PREFIX_PATH=$aiPrefix\Library" `
            "-DPython_EXECUTABLE=$aiPrefix\python.exe" `
            "-DPython3_EXECUTABLE=$aiPrefix\python.exe"
        if ($LASTEXITCODE -ne 0) {
            throw "CMake configuration failed."
        }
    }
    "build" {
        Enable-MsvcEnvironment
        & cmake --build build-ai
        if ($LASTEXITCODE -ne 0) {
            throw "automate_cpp build failed."
        }
    }
    "check" {
        $extension = Get-ChildItem -LiteralPath build-ai `
            -Filter "automate_cpp.cp310-win_amd64.pyd" -File -ErrorAction SilentlyContinue `
            | Select-Object -First 1
        if (-not $extension) {
            throw "automate_cpp was not found. Run 'pixi run build-cpp' first."
        }

        $checkCode = @'
from pathlib import Path
import torch
import automate
import automate_cpp
from automate import MateModelConfig, MatePairModel

checkpoint_path = Path("runs/mate_multitask_v2/best.pt")
checkpoint = torch.load(checkpoint_path, map_location="cpu", weights_only=False)
model = MatePairModel(MateModelConfig(**checkpoint["model_config"]))
model.load_state_dict(checkpoint["model_state"], strict=True)

print(f"automate_cpp={automate_cpp.__file__}")
print(f"torch={torch.__version__}")
print(f"checkpoint={checkpoint_path} epoch={checkpoint.get('epoch')}")
print("automate_cpp_import=OK")
print("strict_checkpoint_load=OK")
'@
        $checkCode | & python -
        if ($LASTEXITCODE -ne 0) {
            throw "automate_cpp validation failed."
        }
    }
}
