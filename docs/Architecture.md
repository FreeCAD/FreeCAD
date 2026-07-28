# MbDFEM Architecture

MbDFEM is a built-in FreeCAD module intended to combine Assembly, Multibody
Dynamics, and Finite Element Analysis workflows while preserving FreeCAD's
document model.

This document describes the current architecture and the direction for future
development.

## Current Source Layout

The authoritative source is:

```text
src/Mod/MbDFEM
```

Current files include:

```text
src/Mod/MbDFEM
├── App
│   ├── AppMbDFEM.cpp
│   ├── CMakeLists.txt
│   ├── MbDAssembly.cpp
│   ├── MbDAssembly.h
│   ├── MbDAssembly.pyi
│   ├── MbDAssemblyPyImp.cpp
│   ├── MbDMarker.cpp
│   ├── MbDMarker.h
│   ├── MbDPart.cpp
│   ├── MbDPart.h
│   ├── MbDPart.pyi
│   └── MbDPartPyImp.cpp
├── Examples
│   └── CreateMbDFEMModel.py
├── CMakeLists.txt
├── Init.py
├── InitGui.py
├── MbDFEMGlobal.h
├── README.md
└── TestMbDFEMApp.py
```

## Registration Points

The module is connected to the FreeCAD build through:

```text
src/Mod/CMakeLists.txt
cMake/FreeCAD_Helpers/InitializeFreeCADBuildOptions.cmake
cMake/FreeCAD_Helpers/PrintFinalReport.cmake
```

The build option is:

```cmake
BUILD_MBDFEM
```

## Design Principles

- Use native FreeCAD `App::DocumentObject` types for user-visible engineering
  objects.
- Keep engineering data in App classes.
- Keep GUI behavior in Gui/ViewProvider classes as the module grows.
- Use FreeCAD property links for relationships instead of a parallel ownership
  graph.
- Keep solver-specific logic behind exporter/adapter layers.
- Prefer small incremental objects and tests over large speculative frameworks.

## Current Object Model

The current C++ objects are:

```text
MbDFEM::MbDAssembly
MbDFEM::MbDPart
MbDFEM::MbDMarker
MbDFEM::MbDJoint
MbDFEM::MbDMotion
MbDFEM::MbDAction
```

The current persistent document relationships are:

```text
FreeCAD Document
├── MbDAssembly.assemblies -> [MbDAssembly, ...]
├── MbDAssembly.parts -> [MbDPart, ...]
├── MbDAssembly.markers -> [MbDMarker, ...]
├── MbDAssembly.joints -> [MbDJoint, ...]
├── MbDAssembly.motions -> [MbDMotion, ...]
├── MbDAssembly.actions -> [MbDAction, ...]
├── MbDPart.markers -> [MbDMarker, ...]
└── MbDItemIJ.markerI / markerJ -> MbDMarker
```

The example script in `Examples/CreateMbDFEMModel.py` creates this kind of
relationship graph for interactive testing. The `assemblies`, `parts`,
`markers`, `joints`, `motions`, and `actions` properties are the authoritative
model relationships.
`MbDJoint`, `MbDMotion`, and `MbDAction` derive from `MbDItemIJ`, which stores
their `markerI` and `markerJ` links.
Assemblies and parts also create lightweight App group objects for tree
presentation, so the GUI can show named `Markers`, `Assemblies`, `Parts`,
`Joints`, `Motions`, and `Actions` rows without making those folders the source
of model truth.

## Planned Object Families

Future App objects should likely include:

- `MbDForce` for force and torque definitions.
- `MbDSolver` for simulation settings.
- `MbDResult` for time-history and post-processing data.
- FEM coupling objects that link to or reuse FreeCAD FEM workbench objects.

## Solver Boundary

MbDFEM should own the FreeCAD document objects and engineering model. External
solvers should be treated as back ends.

Possible solver back ends:

- MBDyn.
- CalculiX through FreeCAD FEM conventions.
- Chrono.
- Other future solvers.

The first stable boundary should be:

```text
FreeCAD Document Object Model
    -> MbDFEM internal model validation
    -> solver input exporter
    -> external solver execution
    -> result importer
    -> visualization and post-processing
```

## Python Interface

The module exposes Python extension objects for interactive use and tests.

Current tests import:

```python
import MbDFEM
```

and create objects with TypeIds such as:

```text
MbDFEM::MbDAssembly
MbDFEM::MbDPart
MbDFEM::MbDMarker
```

The Python API should remain thin and FreeCAD-native. It should expose document
objects and convenience methods without duplicating document ownership.

## GUI Direction

Current GUI registration is minimal through:

```text
src/Mod/MbDFEM/InitGui.py
```

As the module grows, user-visible objects should receive ViewProviders for:

- tree display
- icons
- selection behavior
- 3D visualization
- task panels
- edit modes

Keep GUI code out of App classes.

## Testing Direction

Current tests live in:

```text
src/Mod/MbDFEM/TestMbDFEMApp.py
```

Tests should continue to cover:

- object creation
- TypeId registration
- property defaults
- document save and restore
- group membership
- Python API convenience methods

Add C++ tests when implementation moves beyond simple object registration into
shared algorithms or model validation.
