# Windows FreeCAD Development Setup Notes

This file contains extended notes for reproducing this MbDFEM workspace on
Windows. For the shortest setup path, start with [Setup](Setup.md).

## Preferred Stack

Use a Microsoft-centered workflow:

- Windows 11.
- Visual Studio Community 2026 or current Visual Studio with MSVC v145 x64.
- VS Code.
- VS Code C/C++ extension.
- VS Code CMake Tools extension.
- PowerShell or x64 Native Tools Command Prompt.
- Visual Studio debugger through `cppvsdbg`.

Required non-Microsoft components:

- Git for Windows.
- CMake.
- FreeCAD Debug LibPack.
- 7-Zip if needed for LibPack extraction.

## Workspace Layout

The current checkout uses:

```text
C:\Users\askoh\Documents\GitHub\aiksiongkoh\FreeCAD\feature-mbdfem
```

Recommended new-machine equivalent:

```text
C:\Users\<WindowsUser>\Documents\GitHub\aiksiongkoh\FreeCAD\feature-mbdfem
```

The Debug LibPack is outside the source checkout:

```text
C:\Users\<WindowsUser>\Documents\GitHub\FreeCAD\FreeCAD-LibPack\LibPack-26.3.0-v3.5.2-x64-Debug
```

Keeping the LibPack outside the Git checkout avoids accidental commits of
third-party binaries.

## Branch Model

Use this branch for MbDFEM development:

```text
feature/mbdfem
```

Remotes:

```text
origin    https://github.com/aiksiongkoh/FreeCAD.git
upstream  https://github.com/FreeCAD/FreeCAD.git
```

The fork's `main` branch does not need to change for MbDFEM collaboration.
Collaborators can work directly on `origin/feature/mbdfem`.

Because this is a shared branch, merge upstream instead of rebasing unless the
team agrees to rewrite history:

```powershell
git fetch upstream main
git merge upstream/main
git submodule update --init --recursive
git push
```

## Visual Studio Environment

Open VS Code from **x64 Native Tools Command Prompt for VS 2026**:

```cmd
code "%USERPROFILE%\Documents\GitHub\aiksiongkoh\FreeCAD\feature-mbdfem"
```

Verify:

```powershell
where.exe cl
```

Expected:

```text
...\Hostx64\x64\cl.exe
```

If VS Code is opened without the MSVC environment, CMake may configure with the
wrong compiler or fail to find compiler headers and Windows SDK files.

## CMake Version Notes

The current workspace has used CMake successfully with the Visual Studio
generator and the Debug LibPack.

If CMake reports Python development components missing from the Debug LibPack,
verify the LibPack first:

```powershell
$LibPack = "$HOME\Documents\GitHub\FreeCAD\FreeCAD-LibPack\LibPack-26.3.0-v3.5.2-x64-Debug"
Test-Path "$LibPack\bin\python.exe"
Test-Path "$LibPack\bin\Include\Python.h"
Test-Path "$LibPack\bin\libs"
```

Earlier setup notes observed that one newer CMake release failed to detect the
Debug LibPack Python components. If this happens again, use the CMake version
known to work with the current FreeCAD LibPack, then rerun configure from a
clean `build/debug` directory.

## Configure and Build Commands

Configure:

```powershell
cmake --preset debug
```

Build the GUI executable:

```powershell
cmake --build build\debug --config Debug --target FreeCADMain
```

Build the MbDFEM module:

```powershell
cmake --build build\debug --config Debug --target MbDFEM
```

Build the Python main module if PDB names or startup symbols change:

```powershell
cmake --build build\debug --config Debug --target FreeCADMainPy
```

## Expected Artifacts

Main GUI executable:

```text
build/debug/bin/FreeCAD_d.exe
```

Command-line executable:

```text
build/debug/bin/FreeCADCmd_d.exe
```

Main GUI symbols:

```text
build/debug/bin/FreeCAD_d.pdb
```

MbDFEM Python extension:

```text
build/debug/Mod/MbDFEM/MbDFEM_d.pyd
```

MbDFEM generated runtime scripts:

```text
build/debug/Mod/MbDFEM/Init.py
build/debug/Mod/MbDFEM/InitGui.py
build/debug/Mod/MbDFEM/Examples/CreateMbDFEMModel.py
```

## Debugger Notes

Use VS Code `cppvsdbg` to debug `FreeCAD_d.exe`.

If breakpoints in `src/Main/MainGui.cpp` are hollow, check:

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\bin\llvm-pdbutil.exe" `
    dump -files build\debug\bin\FreeCAD_d.pdb |
    Select-String -Pattern "MainGui.cpp" -SimpleMatch
```

If no match is returned, rebuild `FreeCADMainPy` and `FreeCADMain`. The
workspace includes a fix that prevents `FreeCADMainPy` from overwriting the GUI
executable PDB.

## MbDFEM Smoke Test

In FreeCAD's Python console:

```python
import MbDFEM
from pathlib import Path

script = Path(MbDFEM.__file__).parent / "Examples" / "CreateMbDFEMModel.py"
exec(script.read_text(encoding="utf-8"))
```

Run tests:

```python
import Test
import TestMbDFEMApp

Test.runTestsFromModule(TestMbDFEMApp)
```

## Files to Commit

Commit source, CMake registration, and documentation:

```text
src/Mod/MbDFEM
src/Mod/CMakeLists.txt
cMake/FreeCAD_Helpers/InitializeFreeCADBuildOptions.cmake
cMake/FreeCAD_Helpers/PrintFinalReport.cmake
src/Main/CMakeLists.txt
docs
```

Avoid committing generated or local files:

```text
build
.vscode
CMakeUserPresets.json
*.log
*.FCStd
```

Review before committing:

```powershell
git status --short
git diff --check
git diff --stat
```
