# MbDFEM development workspace on Windows 11

This guide creates a Windows 11 development workspace for the built-in FreeCAD
`MbDFEM` module. It reproduces the layout used by this checkout:

```text
C:\Users\<WindowsUser>\Documents\FreeCAD\Development\
├── FreeCAD\
│   ├── build\
│   │   └── debug\
│   └── src\Mod\MbDFEM\
└── FreeCAD-LibPack\
    └── LibPack-26.3.0-v3.5.2-x64-Debug\
```

The source directory is a fork of
[FreeCAD/FreeCAD](https://github.com/FreeCAD/FreeCAD). The build directory follows
FreeCAD's checked-in `debug` CMake preset. The LibPack remains outside the source
checkout.

FreeCAD `main` and the LibPack evolve together. The versions above reproduce this
checkout. For a later FreeCAD revision, check the current
[FreeCAD Developers Handbook](https://freecad.github.io/DevelopersHandbook/) and
[FreeCAD-LibPack releases](https://github.com/FreeCAD/FreeCAD-LibPack/releases)
before selecting tool and LibPack versions.

## 1. Install the development tools

Open Windows PowerShell as a normal user. Install Git, GitHub CLI, Visual Studio
Code, CMake, and 7-Zip:

```powershell
winget install --id Git.Git --exact
winget install --id GitHub.cli --exact
winget install --id Microsoft.VisualStudioCode --exact
winget install --id Kitware.CMake --exact
winget install --id 7zip.7zip --exact
```

Install the current Visual Studio Community release that provides the MSVC v145
x64 toolchain. On the machine used for this guide that is Visual Studio 2026
Community. Use Visual Studio Installer to select:

- **Desktop development with C++**
- MSVC v145 x64/x86 build tools
- Windows 11 SDK
- C++ CMake tools for Windows

Restart PowerShell after installation and verify the command-line tools:

```powershell
git --version
gh --version
cmake --version
code --version
```

Install these VS Code extensions:

```powershell
code --install-extension ms-vscode.cpptools
code --install-extension ms-vscode.cmake-tools
```

## 2. Fork FreeCAD

Authenticate GitHub CLI:

```powershell
gh auth login
```

Choose `GitHub.com`, HTTPS, and browser authentication when prompted. Create a
fork under the authenticated GitHub account:

```powershell
gh repo fork FreeCAD/FreeCAD --clone=false
```

GitHub describes a fork as a repository connected to its upstream repository;
see [Fork a repository](https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/working-with-forks/fork-a-repo).

## 3. Create the directory layout and clone the fork

Set workspace variables. These values exist only in the current PowerShell
process at this stage:

```powershell
$Root = Join-Path $HOME "Documents\FreeCAD\Development"
$Source = Join-Path $Root "FreeCAD"
$LibPackRoot = Join-Path $Root "FreeCAD-LibPack"
$Build = Join-Path $Source "build\debug"

New-Item -ItemType Directory -Force -Path $Root, $LibPackRoot | Out-Null
```

Obtain the GitHub login name and clone the fork:

```powershell
$GitHubUser = gh api user --jq .login
git clone "https://github.com/$GitHubUser/FreeCAD.git" $Source
Set-Location $Source
```

Keep the fork as `origin` and register the official repository as `upstream`:

```powershell
git remote add upstream https://github.com/FreeCAD/FreeCAD.git
git remote -v
```

The expected remotes are:

```text
origin    https://github.com/<GitHubUser>/FreeCAD.git
upstream  https://github.com/FreeCAD/FreeCAD.git
```

Fetch branches and check out the branch containing MbDFEM. Replace
`<mbdfem-branch>` with the branch pushed from this development checkout:

```powershell
git fetch --all --prune
git switch --track "origin/<mbdfem-branch>"
```

Upstream FreeCAD does not contain MbDFEM until the module is accepted there, so
checking out the MbDFEM branch is required to reproduce this workspace.

## 4. Install the FreeCAD LibPack

The FreeCAD Developers Handbook recommends the prebuilt LibPack as the easiest
way to supply Windows dependencies. Download this checkout's compatible Debug
archive from the
[FreeCAD-LibPack releases page](https://github.com/FreeCAD/FreeCAD-LibPack/releases):

```text
LibPack-26.3.0-v3.5.2-x64-Debug
```

Place the downloaded archive in `$LibPackRoot`, then extract it with 7-Zip. If
the archive is a `.7z` file:

```powershell
$Archive = Get-ChildItem $LibPackRoot -Filter "LibPack-26.3.0-v3.5.2-x64-Debug*.7z" |
    Select-Object -First 1

if (-not $Archive) {
    throw "The expected LibPack archive was not found in $LibPackRoot"
}

& "${env:ProgramFiles}\7-Zip\7z.exe" x $Archive.FullName "-o$LibPackRoot"
```

Locate and validate the extracted directory:

```powershell
$LibPack = Join-Path $LibPackRoot "LibPack-26.3.0-v3.5.2-x64-Debug"

if (-not (Test-Path (Join-Path $LibPack "bin"))) {
    throw "Invalid LibPack directory: $LibPack"
}
```

Do not place the LibPack inside the `FreeCAD` Git checkout.

## 5. Create the local CMake preset

FreeCAD supplies `debug` and `release` configure presets in
`CMakePresets.json`. The `debug` preset uses `${sourceDir}/build/debug`. Add a
local preset in `$Source\CMakeUserPresets.json` that inherits it:

```powershell
Set-Location $Source
@'
{
    "version": 3,
    "configurePresets": [
        {
            "name": "mbdfem-debug",
            "displayName": "MbDFEM Debug (Windows LibPack)",
            "description": "FreeCAD debug preset with the local Windows LibPack and MbDFEM enabled",
            "inherits": "debug",
            "generator": "Ninja",
            "cacheVariables": {
                "FREECAD_LIBPACK_USE": {
                    "type": "BOOL",
                    "value": "ON"
                },
                "FREECAD_LIBPACK_DIR": {
                    "type": "PATH",
                    "value": "C:/Users/<WindowsUser>/Documents/FreeCAD/Development/FreeCAD-LibPack/LibPack-26.3.0-v3.5.2-x64-Debug"
                },
                "BUILD_MBDFEM": {
                    "type": "BOOL",
                    "value": "ON"
                }
            }
        }
    ],
    "buildPresets": [
        {
            "name": "mbdfem-debug",
            "displayName": "MbDFEM Debug",
            "configurePreset": "mbdfem-debug"
        }
    ]
}
'@ | Set-Content -Encoding utf8 CMakeUserPresets.json
```

Replace `<WindowsUser>` in that file:

```powershell
$Preset = Get-Content -Raw CMakeUserPresets.json
$Preset = $Preset.Replace("<WindowsUser>", $env:USERNAME)
$Preset | Set-Content -Encoding utf8 CMakeUserPresets.json
```

`CMakeUserPresets.json` is intentionally ignored by FreeCAD Git because it
contains machine-specific paths. Do not modify FreeCAD's checked-in
`CMakePresets.json` for this workspace.

Verify both presets:

```powershell
cmake --list-presets
cmake --list-presets=build
```

Both lists should contain `mbdfem-debug`.

## 6. Create the VS Code workspace files

FreeCAD ignores `.vscode`, so these files remain local to the machine. Create
the directory:

```powershell
New-Item -ItemType Directory -Force -Path (Join-Path $Source ".vscode") | Out-Null
```

### PowerShell workspace variables

Create `.vscode\workspace.ps1`:

```powershell
@'
# Initialize variables for PowerShell terminals opened in this VS Code workspace.

$global:Source = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$global:Root = Split-Path -Parent $global:Source
$global:Build = Join-Path $global:Source "build\debug"
$global:LibPack = Join-Path $global:Root "FreeCAD-LibPack\LibPack-26.3.0-v3.5.2-x64-Debug"

foreach ($workspacePath in @($global:Source, $global:Build, $global:LibPack)) {
    if (-not (Test-Path -LiteralPath $workspacePath)) {
        Write-Warning "FreeCAD workspace path does not exist: $workspacePath"
    }
}
'@ | Set-Content -Encoding utf8 .vscode\workspace.ps1
```

Every new integrated PowerShell terminal will dot-source this file. Persistent
terminal sessions retain their existing variables; newly created terminals
recalculate them from the workspace path.

### VS Code settings

Create `.vscode\settings.json`:

```powershell
@'
{
    "cmake.sourceDirectory": "${workspaceFolder}",
    "cmake.useCMakePresets": "always",
    "cmake.configurePreset": "mbdfem-debug",
    "cmake.buildPreset": "mbdfem-debug",
    "cmake.configureOnOpen": false,
    "C_Cpp.default.compileCommands": "${workspaceFolder}/build/debug/compile_commands.json",
    "terminal.integrated.profiles.windows": {
        "PowerShell (FreeCAD Workspace)": {
            "source": "PowerShell",
            "args": [
                "-NoLogo",
                "-NoExit",
                "-ExecutionPolicy",
                "Bypass",
                "-Command",
                ". '${workspaceFolder}\\.vscode\\workspace.ps1'"
            ],
            "overrideName": true
        }
    },
    "terminal.integrated.defaultProfile.windows": "PowerShell (FreeCAD Workspace)",
    "terminal.integrated.enablePersistentSessions": true,
    "terminal.integrated.persistentSessionReviveProcess": "onExitAndWindowClose"
}
'@ | Set-Content -Encoding utf8 .vscode\settings.json
```

### Debugger launch configuration

Create `.vscode\launch.json`. The PySide6 and Shiboken6 directories are needed
in addition to the LibPack's top-level `bin` directory for a Debug launch:

```powershell
@'
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "FreeCAD Debug",
            "type": "cppvsdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/debug/bin/FreeCAD_d.exe",
            "args": [],
            "cwd": "${workspaceFolder}/build/debug/bin",
            "stopAtEntry": false,
            "environment": [
                {
                    "name": "PATH",
                    "value": "C:/Users/<WindowsUser>/Documents/FreeCAD/Development/FreeCAD-LibPack/LibPack-26.3.0-v3.5.2-x64-Debug/bin;C:/Users/<WindowsUser>/Documents/FreeCAD/Development/FreeCAD-LibPack/LibPack-26.3.0-v3.5.2-x64-Debug/bin/Lib/site-packages/PySide6;C:/Users/<WindowsUser>/Documents/FreeCAD/Development/FreeCAD-LibPack/LibPack-26.3.0-v3.5.2-x64-Debug/bin/Lib/site-packages/shiboken6;${env:PATH}"
                }
            ],
            "console": "internalConsole"
        }
    ]
}
'@ | Set-Content -Encoding utf8 .vscode\launch.json

$Launch = Get-Content -Raw .vscode\launch.json
$Launch = $Launch.Replace("<WindowsUser>", $env:USERNAME)
$Launch | Set-Content -Encoding utf8 .vscode\launch.json
```

## 7. Start VS Code with the x64 compiler environment

The CMake Tools extension must inherit the x64 MSVC and Windows SDK environment.
Close all VS Code windows. Open **x64 Native Tools Command Prompt for VS 2026**
from the Windows Start menu, then start VS Code:

```powershell
code "$HOME\Documents\FreeCAD\Development\FreeCAD"
```

Use the x64 prompt, not the generic Developer PowerShell. A missing compiler
environment causes errors such as missing `array` or `io.h`; an x86 environment
can cause unresolved Debug runtime symbols during linking.

## 8. Configure FreeCAD

In VS Code, press `Ctrl+Shift+P` and run **CMake: Configure**. Select
`mbdfem-debug` if prompted.

The equivalent command, run from an x64 Visual Studio developer environment,
is:

```powershell
Set-Location $Source
cmake --preset mbdfem-debug
```

If a failed earlier configure cached a missing compiler, refresh only the build
configuration:

```powershell
cmake --fresh --preset mbdfem-debug
```

Successful output ends with:

```text
Configuring done
Generating done
Build files have been written to: ...\FreeCAD\build\debug
```

Verify important cache values:

```powershell
Select-String -Path "$Build\CMakeCache.txt" -Pattern @(
    '^BUILD_MBDFEM:BOOL=ON',
    '^CMAKE_BUILD_TYPE:STRING=Debug',
    '^FREECAD_LIBPACK_USE:BOOL=ON'
)
```

## 9. Build FreeCAD and MbDFEM

In VS Code:

1. Press `Ctrl+Shift+P` and run **CMake: Set Build Target**.
2. Select `all`.
3. Click **Build** on the status bar, or run **CMake: Build**.

The PowerShell equivalent is:

```powershell
cmake --build $Build
```

A first Debug build compiles several thousand targets. Ninja builds
incrementally, so rerunning the command continues from completed outputs.

Verify the principal artifacts:

```powershell
Get-Item @(
    "$Build\bin\FreeCAD_d.exe",
    "$Build\bin\FreeCADCmd_d.exe",
    "$Build\Mod\MbDFEM\MbDFEM_d.pyd",
    "$Build\Mod\MbDFEM\Init.py",
    "$Build\Mod\MbDFEM\Examples\CreateMbDFEMModel.py"
)
```

## 10. Launch and test MbDFEM

Press `F5` in VS Code and select **FreeCAD Debug** if prompted. In FreeCAD,
select **View > Workbench > MbDFEM**, then enable **View > Panels > Python
console**.

Create the example model:

```python
from pathlib import Path
import MbDFEM

script = Path(MbDFEM.__file__).parent / "Examples" / "CreateMbDFEMModel.py"
exec(script.read_text(encoding="utf-8"))
```

The example creates these persistent relationships:

```text
MbDFEM_Doc
├── MbDAssembly1.assemblies -> [MbDAssembly2]
├── MbDAssembly1.parts -> [MbDPart1, MbDPart2]
├── MbDAssembly1.markers -> [MbDMarker1, MbDMarker2]
├── MbDAssembly1.joints -> [MbDJoint1, MbDJoint2]
├── MbDAssembly1.motions -> [MbDMotion1, MbDMotion2]
├── MbDAssembly1.actions -> [MbDAction1, MbDAction2]
├── MbDPart1.markers -> [MbDMarker11, MbDMarker12]
├── MbDPart2.markers -> [MbDMarker21, MbDMarker22]
└── MbDJoint/MbDMotion/MbDAction.markerI, markerJ -> MbDMarker
```

The `assemblies`, `parts`, `markers`, `joints`, `motions`, and `actions`
properties are the authoritative model relationships. MbDFEM also creates
lightweight App folder objects for tree presentation, so the GUI is displayed
as:

```text
MbDFEM_Doc
└── MbDAssembly1
    ├── Markers
    │   ├── MbDMarker1
    │   └── MbDMarker2
    ├── Assemblies
    │   └── MbDAssembly2
    ├── Parts
    │   ├── MbDPart1
    │   │   └── Markers
    │   │       ├── MbDMarker11
    │   │       └── MbDMarker12
    │   └── MbDPart2
    │       └── Markers
    │           ├── MbDMarker21
    │           └── MbDMarker22
    ├── Joints
    │   ├── MbDJoint1
    │   │   ├── MbDMarker1
    │   │   └── MbDMarker11
    │   └── MbDJoint2
    │       ├── MbDMarker12
    │       └── MbDMarker21
    ├── Motions
    │   ├── MbDMotion1
    │   │   ├── MbDMarker1
    │   │   └── MbDMarker12
    │   └── MbDMotion2
    │       ├── MbDMarker2
    │       └── MbDMarker22
    └── Actions
        ├── MbDAction1
        │   ├── MbDMarker1
        │   └── MbDMarker21
        └── MbDAction2
            ├── MbDMarker2
            └── MbDMarker22
```

Run the module's registered Python tests with the current FreeCAD Test API:

```python
import Test
import TestMbDFEMApp

Test.runTestsFromModule(TestMbDFEMApp)
```

Expected result:

```text
Ran 2 tests
OK
```

The tests cover object creation, `TypeId`, placements, relationships, saving,
closing, and reopening.

## 11. Keep the fork synchronized

Before starting new work, update the local view of upstream:

```powershell
Set-Location $Source
git fetch upstream --prune
```

Rebase a feature branch onto current upstream `main` only when the working tree
is clean:

```powershell
git switch <mbdfem-branch>
git rebase upstream/main
```

Push the branch to the fork:

```powershell
git push --force-with-lease origin <mbdfem-branch>
```

Use `--force-with-lease` after a rebase; do not use an unrestricted force push.

## 12. Source-controlled and generated files

The MbDFEM source, tests, example, and this document belong under Git source
control:

```text
src/Mod/MbDFEM/
```

The following are local or generated and are intentionally ignored:

```text
.vscode/
CMakeUserPresets.json
build/
```

Do not edit generated copies under `build/debug/Mod/MbDFEM`. Edit the
authoritative files under `src/Mod/MbDFEM`, rebuild, and test.

Before committing, review only the intended files:

```powershell
git status --short
git diff --check
git diff
```

Stage the MbDFEM module and its three FreeCAD registration changes:

```powershell
git add src/Mod/MbDFEM
git add src/Mod/CMakeLists.txt
git add cMake/FreeCAD_Helpers/InitializeFreeCADBuildOptions.cmake
git add cMake/FreeCAD_Helpers/PrintFinalReport.cmake
```

Commit after the build and tests pass:

```powershell
git commit -m "MbDFEM: Add initial built-in module"
git push -u origin HEAD
```

Do not add unrelated files such as local logs or manually generated `.FCStd`
documents.
