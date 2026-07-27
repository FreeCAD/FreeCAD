# MbDFEM Development Documentation

These documents describe how to reproduce and work in the current MbDFEM
FreeCAD development workspace on a new Windows computer.

The current workspace is:

```text
C:\Users\askoh\Documents\GitHub\aiksiongkoh\FreeCAD\feature-mbdfem
```

The active branch is:

```text
feature/mbdfem
```

The fork remote is:

```text
origin    https://github.com/aiksiongkoh/FreeCAD.git
upstream  https://github.com/FreeCAD/FreeCAD.git
```

## Documents

- [Setup](Setup.md): install tools, clone the fork, configure, build, and launch
  the workspace from VS Code.
- [Debugging](Debugging.md): use the Visual Studio debugger from VS Code and
  verify breakpoints and PDB symbols.
- [Architecture](Architecture.md): current MbDFEM module structure and design
  direction.
- [Development Log](DevelopmentLog.md): important project milestones and known
  decisions.
- [Windows FreeCAD Development Setup](Windows-FreeCAD-Development-Setup.md):
  extended notes and troubleshooting for this exact Windows setup.

## Current Reproducible State

This branch includes an initial built-in `src/Mod/MbDFEM` module, documentation,
and a Windows Debug PDB-name fix for the FreeCAD executable/module symbol clash.

The branch has been merged with `upstream/main` and pushed to:

```text
https://github.com/aiksiongkoh/FreeCAD/tree/feature/mbdfem
```

Use `feature/mbdfem` as the branch to reproduce this workspace. The fork's
`main` branch is not required for MbDFEM development.
