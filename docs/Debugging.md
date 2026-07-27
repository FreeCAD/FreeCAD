# Debugging FreeCAD and MbDFEM in VS Code

This document records the debugger setup and the resolved Windows symbol issue
for this MbDFEM workspace.

## Debug Configuration

The local VS Code launch configuration uses the Microsoft Visual Studio debugger:

```json
{
    "name": "Debug FreeCAD",
    "type": "cppvsdbg",
    "request": "launch",
    "program": "${workspaceFolder}/build/debug/bin/FreeCAD_d.exe",
    "cwd": "${workspaceFolder}/build/debug/bin",
    "stopAtEntry": true
}
```

Use **Run and Debug > Debug FreeCAD** and press `F5`.

`stopAtEntry` is useful while validating debugger setup. Once startup debugging
is no longer needed, set it back to `false`.

## Expected Breakpoint Behavior

A breakpoint in:

```text
src/Main/MainGui.cpp
```

should bind when debugging:

```text
build/debug/bin/FreeCAD_d.exe
```

The breakpoint at:

```text
MainGui.cpp:274
```

is:

```cpp
Gui::Application::initApplication();
```

This runs early in GUI startup. If you attach after FreeCAD is already open,
that line has probably already executed.

## Resolved PDB Issue

The debugger originally did not stop in `MainGui.cpp` because the loaded
executable PDB did not contain that source file.

`FreeCAD_d.exe` referenced:

```text
build/debug/bin/FreeCAD_d.pdb
```

but that PDB listed `MainPy.cpp` and not `MainGui.cpp`.

Root cause:

- `FreeCADMain` outputs the GUI executable `FreeCAD_d.exe`.
- `FreeCADMainPy` outputs the Python module `FreeCAD_d.pyd`.
- Both targets could produce a Debug PDB named `FreeCAD_d.pdb`.
- The Python module PDB could overwrite the GUI executable PDB.

The fix in `src/Main/CMakeLists.txt` gives `FreeCADMainPy` a separate PDB name:

```cmake
if(WIN32)
    # Name clash with target "FreeCADMain"
    # Must be called "FreeCADMainPy_d" and "FreeCADMainPy" to work so override default
    set_target_properties(FreeCADMainPy PROPERTIES PDB_NAME_DEBUG "FreeCADMainPy_d")
    set_target_properties(FreeCADMainPy PROPERTIES PDB_NAME_RELEASE "FreeCADMainPy")
endif(WIN32)
```

This mirrors the existing `FreeCADGuiPy` workaround.

## Rebuild After Symbol Changes

After changing CMake target or PDB properties, rebuild these targets:

```powershell
cmake --build build\debug --config Debug --target FreeCADMainPy
cmake --build build\debug --config Debug --target FreeCADMain
```

If the executable PDB remains stale, force a target rebuild from Visual Studio
or rerun CMake configure and rebuild the target.

## Verify the PDB Contains MainGui.cpp

Use `llvm-pdbutil.exe` from the Visual Studio LLVM tools:

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\bin\llvm-pdbutil.exe" `
    dump -files build\debug\bin\FreeCAD_d.pdb |
    Select-String -Pattern "MainGui.cpp" -SimpleMatch
```

Expected result includes:

```text
src\Main\MainGui.cpp
```

## Common Causes of Missed Breakpoints

- Launching `FreeCADCmd_d.exe` instead of `FreeCAD_d.exe`.
- Attaching after startup code has already run.
- Hollow breakpoint because symbols are not loaded.
- Stale PDB after target output-name changes.
- Opening VS Code outside the x64 Visual Studio developer environment before
  configuring.

## Useful Checks

Check the executable and PDB:

```powershell
Get-Item build\debug\bin\FreeCAD_d.exe
Get-Item build\debug\bin\FreeCAD_d.pdb
```

Check what PDB the executable references:

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.51.36231\bin\HostX64\x64\dumpbin.exe" `
    /headers build\debug\bin\FreeCAD_d.exe |
    Select-String -Pattern "RSDS" -Context 0,1
```

Check branch and source status before debugging a reported issue:

```powershell
git status --short --branch
git log -1 --oneline
```
