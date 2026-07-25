# Debugging Log

## Objective

Configure VS Code so breakpoints work correctly while debugging FreeCAD.

---

# Current Problem

FreeCAD starts successfully.

Breakpoints inside

src/Main/MainGui.cpp

remain hollow.

Tooltip:

No symbols have been loaded for this document.

---

# What Has Been Verified

## Build

✔ Debug build

✔ No build errors

---

## Symbols

FreeCADGui_d.dll

Symbols loaded.

FreeCADApp_d.dll

Symbols loaded.

FreeCADBase_d.dll

Symbols loaded.

---

## PDB

Verified

build/debug/bin/FreeCAD_d.pdb

exists.

---

## VS Code

launch.json was created.

launch.json later removed.

Using

CMake: Debug

produces the same behavior.

---

## CMake Tools

Installed

Commands available.

---

## Current Hypothesis

Symbols for

FreeCAD_d.exe

are not being associated with

MainGui.cpp.

Need to determine why.

---

# Remaining Investigation

- Verify generated debugger launch configuration.

- Verify debugger type.

- Verify EXE/PDB GUID match.

- Verify linker uses /DEBUG.

- Determine why only executable symbols fail to bind.