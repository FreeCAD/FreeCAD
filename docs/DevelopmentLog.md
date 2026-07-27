# MbDFEM Development Log

This log records important development-environment milestones and decisions for
the MbDFEM FreeCAD branch.

## Current Branch State

Active branch:

```text
feature/mbdfem
```

Tracked remote branch:

```text
origin/feature/mbdfem
```

Official FreeCAD remote:

```text
upstream/main
```

The branch was merged with `upstream/main` and pushed to the fork. At the time
of this note, the local branch and `origin/feature/mbdfem` are synchronized.

## Environment Milestones

- Windows 11 development environment selected.
- Microsoft Visual Studio C++ toolchain selected for compiler, linker, and
  debugger.
- VS Code selected as the editor and debugger front end.
- FreeCAD Debug LibPack `LibPack-26.3.0-v3.5.2-x64-Debug` selected.
- FreeCAD Debug build configured under `build/debug`.
- `FreeCAD_d.exe` builds and launches from VS Code.
- MbDFEM source lives under `src/Mod/MbDFEM`.
- MbDFEM is registered from `src/Mod/CMakeLists.txt`.
- `BUILD_MBDFEM` build option is available from
  `cMake/FreeCAD_Helpers/InitializeFreeCADBuildOptions.cmake`.

## Debugging Milestones

- Breakpoints in `src/Main/MainGui.cpp` originally did not bind.
- Investigation found that `FreeCAD_d.pdb` did not contain `MainGui.cpp`.
- Root cause was a Windows Debug PDB-name clash between `FreeCADMain` and
  `FreeCADMainPy`.
- Fixed by assigning `FreeCADMainPy` separate PDB names:
  `FreeCADMainPy_d.pdb` for Debug and `FreeCADMainPy.pdb` for Release.
- Verified that `FreeCAD_d.pdb` now contains `src\Main\MainGui.cpp`.

## Current MbDFEM Module Status

The branch contains an initial built-in MbDFEM module with:

- `MbDFEM` Python extension module.
- `MbDFEM::MbDAssembly`.
- `MbDFEM::MbDPart`.
- `MbDFEM::MbDMarker`.
- Minimal GUI workbench registration through `InitGui.py`.
- C++ ViewProviders for `MbDAssembly`, `MbDPart`, and `MbDMarker`.
- Example model script at `src/Mod/MbDFEM/Examples/CreateMbDFEMModel.py`.
- Python tests in `src/Mod/MbDFEM/TestMbDFEMApp.py`.

## Collaboration Decision

Because `feature/mbdfem` is a shared collaborator branch, prefer:

```powershell
git fetch upstream main
git merge upstream/main
git push
```

Avoid rebasing the shared branch unless all collaborators agree, because rebase
rewrites published history.

## Near-Term Development Tasks

- Keep the setup documentation accurate as the environment changes.
- Build and test the current branch after upstream merges.
- Confirm MbDFEM workbench appears in the FreeCAD GUI.
- Expand App objects for joints, forces, solver settings, and FEM coupling.
- Add icons and richer behavior for user-visible MbDFEM ViewProviders.
- Add focused tests for object creation, persistence, and document restore.
