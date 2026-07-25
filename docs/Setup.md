# MbDFEM Development Environment Setup

## Purpose

This document describes how to set up a Windows development environment for the
MbDFEM FreeCAD workbench.

---

# Hardware

Development machine:
- Windows 11 Pro
- Visual Studio 2026
- VS Code

---

# Required Software

- Git
- Visual Studio 2026
    - Desktop Development with C++
- CMake
- Ninja
- VS Code

VS Code extensions:

- C/C++
- CMake Tools
- CMake
- GitHub Copilot (optional)
- Codex (optional)

---

# Source Code

Repository:

https://github.com/aiksiongkoh/FreeCAD

Branch:

feature-mbdfem

Clone into

C:\Users\<user>\Documents\GitHub\aiksiongkoh\FreeCAD

---

# LibPack

Debug LibPack:

FreeCAD-LibPack
LibPack-26.3.0-v3.5.2-x64-Debug

---

# Configure

Generator

Ninja

Build type

Debug

---

# Build

Target

FreeCADMain

---

# Verify

Successful build produces

build/debug/bin/FreeCAD_d.exe

and

build/debug/bin/FreeCAD_d.pdb

---

# Current Status

✔ Configure works

✔ Build works

✔ FreeCAD launches

Debugger investigation still in progress.