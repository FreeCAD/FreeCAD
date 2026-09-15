# freecad-typings

Type stubs for the [FreeCAD] Python API (version `{version}`).

This package provides [PEP 561] `py.typed` stubs generated from FreeCAD's
C++ Python bindings, so static type checkers and IDEs can understand the
FreeCAD objects, modules, and functions available at runtime.

## Installation

### pip

```sh
pip install freecad-typings
```

### uv

```sh
uv add freecad-typings
```

## Modules

The stubs cover the public FreeCAD Python modules, including `FreeCAD`,
`FreeCADGui`, `Part`, and the other core workbench modules.

## Usage

Once installed, type checkers (e.g. `pyright`, `pyre`, `mypy`) and editors
will automatically pick up the stubs for the FreeCAD modules.

These stubs are generated and intended for static analysis; they are not a
runtime replacement for the real FreeCAD binary.

[FreeCAD]: https://www.freecad.org/
[PEP 561]: https://peps.python.org/pep-0561/
