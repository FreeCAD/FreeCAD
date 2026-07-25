# Windows 11 Setup for FreeCAD C++ Development (VS Code + Ninja + Debug LibPack)

This document describes a working development environment for building and debugging FreeCAD on Windows 11.

Tested with:

- Windows 11 x64
- Visual Studio Community 2026
- VS Code
- Ninja
- Git
- FreeCAD main branch
- Debug LibPack 3.5.2
- CMake 4.2.3

---

# 1. Install Software

Install:

- Visual Studio Community 2026
    - Desktop development with C++
- VS Code
- Git
- Ninja
- CMake **4.2.3**

Do NOT use CMake 4.4.0.

CMake 4.4.0 failed to detect the Debug LibPack Python:

```
Could NOT find Python3
missing:
Development
Development.Module
Development.Embed
```

Downgrading to CMake 4.2.3 fixed the problem immediately.

---

# 2. Clone FreeCAD

```
git clone https://github.com/<your account>/FreeCAD.git
```

Add upstream

```
git remote add upstream https://github.com/FreeCAD/FreeCAD.git
```

Fetch

```
git fetch upstream
```

---

# 3. Checkout main

```
git checkout main
git pull
```

Create development branch

```
git checkout -b feature/mbdfem
```

---

# 4. Download Debug LibPack

Example

```
LibPack-26.3.0-v3.5.2-x64-Debug
```

---

# 5. Initialize submodules

VERY IMPORTANT

```
git submodule update --init --recursive
```

Without this, configuration fails with

```
The OndselSolver git submodule is not available.
```

---

# 6. Open Developer Command Prompt

Always use x64.

Verify

```
where.exe cl
```

Expected

```
Hostx64\x64\cl.exe
```

---

# 7. Open VS Code

```
code .
```

or

```
code -n <another project>
```

to open another VS Code window.

---

# 8. Configure

```
cmake ^
    -S %Source% ^
    -B %Build% ^
    -G Ninja ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ^
    -DFREECAD_LIBPACK_DIR=%LibPack% ^
    -DFREECAD_COPY_LIBPACK_BIN_TO_BUILD=ON ^
    -DFREECAD_COPY_DEPEND_DIRS_TO_BUILD=ON ^
    -DFREECAD_COPY_PLUGINS_BIN_TO_BUILD=ON
```

Should end with

```
Configuring done

Generating done
```

---

# 9. Build

```
cmake --build %Build% --parallel 8
```

---

# 10. Important Lessons Learned

## Never use CMake 4.4.0

Use

```
CMake 4.2.3
```

The newer FindPython module currently breaks detection of the Debug LibPack.

---

## Always fetch upstream

If Version.h generation prints

```
fatal:
Not a valid object name upstream/main
```

run

```
git fetch upstream
```

---

## Build variables disappear after reboot

PowerShell variables disappear after restart.

Reset

```
$Source = ...
$Build = ...
$LibPack = ...
```

before continuing.

---

## Debug LibPack Python

Correct files are

```
bin/python.exe

bin/Include/Python.h

bin/libs/python314_d.lib
```

If these exist, the LibPack is probably fine.

---

## Use Ninja

Generator

```
-G Ninja
```

is recommended.

---

## Use VS Code

VS Code works well for:

- C++
- Python
- CMake
- Git
- Debugging
- ChatGPT extension
- Codex

---

## Multiple VS Code windows

Safe:

Window #1

FreeCAD build

Window #2

Another unrelated project

No problem.

Avoid editing the same repository from multiple windows simultaneously.

---

# Recommended Workflow

Developer Command Prompt

↓

VS Code

↓

Configure once

↓

Build

↓

Debug

↓

Use Codex for code generation

↓

Commit frequently

```
git commit
```

---

# Project Goal

Develop a new FreeCAD built-in C++ workbench

```
MbDFEM
```

combining

- Assembly
- Multibody Dynamics
- Finite Element Analysis

with Python scripting support and AI-assisted development.