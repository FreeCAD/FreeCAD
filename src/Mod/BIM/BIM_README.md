<!--
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2026 FreeCAD
SPDX-FileNotice: Part of the FreeCAD project.
-->

# FreeCAD BIM

## Welcome to the FreeCAD BIM Module!

The BIM Module is FreeCAD's core Building Information Modeling (BIM) functionality, exposed to users as the BIM Workbench.
It implements BIM-specific parametric objects, tools, workflows, Industry Foundation Classes (IFC) interoperability, BIM project organization, quantity extraction, scheduling, reporting, and much more, useful for projects in architecture, engineering, and construction (AEC) of the build environment.

This document provides a high-level overview of the Module structure, organization, coding conventions, and development guidelines for contributors. Please feel free to improve this document while discovering the Module and contributing to it, so it stays relevant and effective.

Creation Date: July 2026, 26.3 development cycle

Last Update: July 2026

---

## 1. What is the BIM Module?

The BIM Module is FreeCAD's integrated Building Information Modeling framework and Workbench. It provides:

- Parametric architectural and construction objects (e.g. Wall, Slab, Window, Roof, Space)
- BIM project organization tools (e.g. Project, Site, Building, Storey)
- Documentation tools (e.g. Section Plan, Axis, Views, Dimensions)
- IFC-based workflows and Native IFC document support (e.g. Classes, Types, Property Sets)
- Import and export capabilities for common AEC formats (e.g. OBJ, DAE, SH3D, 3DS)
- Quantity surveying and reporting tools (e.g. Schedule, Report)
- Many misc utilities and tools to edit and manage project objects and data

Since FreeCAD 1.0, the BIM Module integrates and extends the historical `Arch` Workbench, Draft tools, and NativeIFC functionality into a unified BIM workbench. Much of the underlying implementation still resides in the legacy `Arch*` modules and packages that provide the foundation of the BIM object model.

The BIM Module is integrated with the rest of FreeCAD and relies heavily on existing FreeCAD core concepts, other important Modules, and API such as:

- App and Gui document architecture
- Part geometry
- Draft tools and utilities
- Sketcher
- TechDraw
- Link objects

The BIM Module is therefore a layer built on top of FreeCAD's parametric document and geometric infrastructure, and extends it with BIM semantics and workflows.

---

## 2. Project Organization and Maintainers

The BIM Module is maintained by the FreeCAD BIM contributors community as part of the main FreeCAD project.

Similarly to other modules in FreeCAD, the development, discussions, bug reports and feature proposals take place through:

- FreeCAD git repository
- Pull requests and code reviews
- Issue tracker
- FreeCAD user and developers forum

New contributors should consult the repository documentation, discussions, and reach out to current contributors before implementing major architectural changes.

---

## 3. Architecture and Dependencies

The BIM Module is almost entirely Python3-based and follows the architectural conventions used throughout FreeCAD.
It builds upon FreeCAD's existing document, property, and geometric infrastructure.

Its main components are:

```
src/Mod/BIM/

Workbench Integration
├── Init.py
├── InitGui.py

Core BIM Objects
├── Arch*.py

Workbench Commands
├── bimcommands/
│   └── Bim*.py

IFC and file formats Infrastructure
├── nativeifc/
│   └── ifc_*.py
├── importers/
│   └── import/export*.py

Resources
├── Resources/
│   ├── icons/
│   |   └── *.svg
│   ├── templates/
│   ├── translations/
│   |   └── *.ts
│   └── ui/
│       └── *.ui

Presets and Schemas
├── Presets/
│   └── *.json/csv

Unit Tests
├── bimtests/
│   └── Test*.py

Utilities
└── utils/
    └── *.py
```

Main dependencies include:

- FreeCAD App API
- FreeCAD Gui API for both Qt (UI) and Coin3D (viewport)
- Part
- Draft
- Sketcher
- TechDraw

External libraries:

- Qt / PySide
- Coin3D / Pivy
- IfcOpenShell (when available)

---

## 4. Core BIM Object Model

The core BIM object system is implemented primarily through the `Arch*.py` modules.

BIM objects are implemented as parametric FreeCAD document objects using the Property system and recompute framework.
Their geometry is typically generated from underlying geometric primitives, profiles, sketches, or other supporting objects referenced through properties such as `Base`.

Examples:

```
ArchWall.py
ArchStructure.py
ArchWindow.py
ArchRoof.py
ArchSpace.py
ArchSite.py
ArchProject.py
```

Most BIM objects share common behavior through base classes such as:

```
ArchComponent.py
```

These base classes provide:

- Property management
- IFC classification support
- Placement handling
- Shape generation
- Material support
- Quantity extraction
- View integration

Many BIM objects reuse functionality and utilities from the `Draft` module.

Most BIM objects also follow FreeCAD's App object / View Provider pattern, where document data and behavior are separated from visualization and user-interface interaction.

---

## 5. Command and GUI Layer

User-facing BIM tools commands are primarily implemented in:

```
bimcommands/
```

Examples:

```
BimWall.py
BimWindow.py
BimRoof.py
BimProject.py
BimMaterial.py
```

These modules typically contain:

- FreeCAD commands (e.g. menu, toolbar, shortcuts)
- Task panels
- Interactive workflows (e.g. 3D view)
- Selection handling

---

## 6. Document Structure and BIM Logic

Unlike many CAD workflows that are feature-based (e.g. additions and subtraction), BIM models rely on semantic relationships in addition to geometry.

A typical BIM document structure is organized as:

```
Project
└── Site
    └── Building
        └── BuildingPart / Storey
            ├── Walls
            ├── Slabs
            ├── Structures
            ├── Spaces
            └── Equipment
```

Key concepts include:

- Hierarchical building organization
- IFC classification
- Host-child relationships
- Base or baseless objects
- Parametric generation
- Quantity extraction
- Material assignment

---

## 7. IFC and NativeIFC Architecture

The IFC support is a central aspect of the BIM Module.

Its functionality is implemented via:

```
ArchIFC.py
ArchIFCSchema.py
importIFC.py
exportIFC.py
```

The IFC layer is responsible for:

- IFC schemas
- Entity creation and mapping
- Import/export pipelines
- Property Set management
- Geometry translation

The newer NativeIFC infrastructure is located in:

```
nativeifc/
```

Key components include:

```
ifc_import.py
ifc_export.py
ifc_geometry.py
ifc_objects.py
ifc_materials.py
ifc_psets.py
```

NativeIFC enables:

- IFC-native document workflows
- Direct IFC entity representations
- Improved IFC round-tripping
- Better preservation of BIM semantics

As of 2026, NativeIFC is still under development and remains an evolving system.
Contributors should expect *architectural- changes as IFC-native workflows continue to mature.

---

## 8. Import and Export Framework

A variety of BIM, CAD and visualization formats are supported.

The import/export implementations are located primarily in:

```
importers/
```

The supported formats include:

- IFC
- OBJ
- DAE (Collada)
- JSON
- SH3D
- GBXML
- 3DS
- WebGL exports

---

## 9. Resources, Presets and Localization

The static assets are stored in:

```
Resources/
Presets/
```

Resources include:

- Icons used for UI
- UI definitions (e.g. preferences pages and dialog windows)
- Translation files
- Export Templates

Presets include:

- IFC schemas and mappings
- Property set definitions
- Quantity definitions
- Classification data
- Reporting presets
- Profile libraries

---

## 10. Testing Infrastructure

Automated tests are located in:

```
bimtests/
```

Tests cover:

- BIM object behavior
- Geometry generation
- IFC functionality
- GUI workflows
- Import/export operations
- Regression scenarios

Typical structure:

```
TestArchWall.py
TestArchStructure.py
TestArchWindow.py
TestArchRoof.py
```

When introducing new functionality:

- Add unit tests whenever possible.
- Extend existing test suites.
- Add regression tests.
- Avoid introducing GUI-only behavior without corresponding tests.

---

## 11. Coding Conventions

All BIM code should follow the [FreeCAD Development Handbook](https://freecad.github.io/DevelopersHandbook/bestpractices/pythonpractices).

Basically, this means:

- PEP 8 and PEP 257 (docstrings)
- Descriptive and standard naming
- SPDX metadata
- Clear [numpydoc](https://numpydoc.readthedocs.io/en/latest/format.html) docstrings and comments
- Double quotes for `"strings"`
- Explicit imports and no lazy import if possible
- Small focused functions
- Early returns, limited nesting and indentation
- Explicit is clearer than implicit
- Max 100 chars per line (current pylint limit)
- Manageable structure and files size
- DRY (do-not-repeat-yourself) and KISS (keep-it-super-simple)
- Appropriate licensing for code (LGPL-2.1), assets, and other resources (e.g. CC BY-SA-4.0)
- When in doubt, ask fellow contributors

### App/Gui Separation

Keep document logic independent from GUI code whenever possible.

```
App Layer
    ↓
Document Objects (Arch*.py)

Gui Layer
    ↓
Commands (bimcommands/Bim*.py)
```

### Reuse Existing Foundations

Prefer reusing and extending `ArchComponent.py`, `ArchCommands.py`, and other existing BIM utilities instead of creating duplicate implementations.

If adding new BIM objects, make sure IFC is properly handled: IFC classification and export compatibility (multiple IFC schemas), Property Sets support, quantity extraction, etc. Check with other IFC viewers or editors (e.g. buildingSMART IFC Validation Service, Bonsai, etc) everything is correct.

Also make sure icons are added, translations are supported based on FreeCAD existing tooling, menus, toolbars, tests, and documentation are updated.

### Modern Python

The BIM Module (and the `Arch` one before it) has a long legacy. Many modern Python concepts and practices (e.g. full type hinting, parallelism/concurrency) are not yet implemented. Please prefer consistency with surrounding BIM code to stylistic rewrites.

### Recommended tools

- [`pre-commit`](https://freecad.github.io/DevelopersHandbook/gettingstarted/)
- [flake8](https://flake8.pycqa.org/en/latest/)
- [codespell](https://github.com/codespell-project/codespell)

---

## 12. Contributing

Before submitting contributions:

1. Review existing BIM architecture.
2. Follow FreeCAD coding guidelines.
3. Reuse existing BIM infrastructure.
4. Add or update tests.
5. Verify IFC compatibility where applicable.
6. Update documentation and resources.
7. Keep pull requests focused and reviewable.

For major changes:

- Open a discussion first.
- Present architectural rationale.
- Consider existing workflows, backward file compatibility, and IFC implications.
- Coordinate with other FreeCAD contributors.

Thanks for reading up to this point =D

Have fun contributing and keep FreeCADing!
