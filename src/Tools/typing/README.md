# Python Binding Stubs

This directory contains the generated-stub workflow for FreeCAD's Python API:
- the helper script that regenerates local stub output
- the smoke-check inputs used by Pyright and Pyrefly
- the remaining manual overlay inputs
- the documentation for curated source-adjacent stub inputs

Use the helper to regenerate discovery output and run the smoke checks:

```sh
src/Tools/typing/check-stubs.sh
```

Run the complete Python test suite for the Tools packages with:

```sh
pixi run python src/Tools/run_tests.py
```

The helper runs the stub generator:

```sh
python3 src/Tools/typing/generate_stubs.py --root . --out-dir src/Tools/typing/generated
```

The shared public API layer lives under `src/Tools/python_api_model/`:
`model.py` defines normalized API declarations, `signatures.py` defines
structured callable semantics, `normalize.py` canonicalizes source-side type
spelling, and `resolve.py` applies deterministic precedence and diagnostics.
This package has no dependency on StubGen.

The source and binding pipeline lives under `src/Tools/typing/stubgen/`:
`api_extract.py` reads curated `.pyi` inputs, `binding_adapter.py` converts
discovered C++ registrations, `render.py` emits public stubs, and `generator.py`
coordinates the output. `discovery.py`, `source_inputs.py`, `module_merge.py`,
and `stub_support.py` provide the remaining input and output-specific helpers.
`type_context_rules.py` holds manual PyCXX context classifications that are not
derivable yet.

That command writes under `src/Tools/typing/generated/`:

- `stubs/`: import-shaped public stubs with overlays applied, plus mapped
  PyCXX type method tables where the runtime type can be tied to a public or
  private stub name. This tree is suitable for a type-checker search path and
  is the output used by the smoke checks. It is disposable local output; the
  repository only keeps `generated/.gitignore`, so regenerate it instead of
  editing it directly.
- `stubs/<Module>/py.typed`: a PEP 561 marker emitted into each top-level
  module directory so type checkers (Pyright, MyPy, Pyrefly) discover the
  stubs when the package is installed.
- `pyproject.toml`: a packaging manifest for distributing the stubs on PyPI
  as `freecad-typings`. It uses the `hatchling` build backend and the FreeCAD
  version from `version.json`, and copies the `stubs/` tree to the wheel root
  so the top-level modules import directly (e.g. `import FreeCAD`).

Build and publish the stub package with `uv` from the generated directory:

```sh
cd src/Tools/typing/generated
uv build                 # creates dist/freecad_typings-<version>.tar.gz and .whl
uv publish               # uploads to PyPI (set UV_PUBLISH_TOKEN or log in first)
```

To push a release candidate or development snapshot instead, use the alias
`uv publish --publish-url https://test.pypi.org/legacy/` (test PyPI) or pass
an explicit index URL. Regenerate before publishing whenever the bindings
change so the wheel reflects the current API.

Keep residual hand-written public overlays under `src/Tools/typing/inputs/overlays/`. Keep
source-adjacent PyCXX type signature inputs in plain `.pyi` files such as
`src/Gui/FreeCADGui._MainWindow.pyi` when curated type signatures should live
next to the wrapper source. Keep stub-only module function signatures and
support nodes in source-adjacent `*.module.pyi` files such as
`src/App/FreeCAD.module.pyi` or `src/Base/FreeCAD.Console.module.pyi`.

Plain source-adjacent type-stub `.pyi` files are consumed by `stubgen` only.
Source-adjacent `*.module.pyi` files can also be registered through CMake with
`generate_module_from_py(...)` when they are the source of generated module
bindings. Plain type-stub `.pyi` files can also contribute top-level support
nodes such as imports, helper aliases, helper protocols, and non-method class
members to the merged public stub output. Do not edit generated output directly;
edit the curated source inputs and regenerate.

### Core Property Contracts

The source-adjacent `src/App/PropertyPythonContracts.pyi` contains the Python
getter/setter behavior at `App::Property*` conversion roots and override
points. `stubgen` parses that file and renders its aliases into the generated
`FreeCAD` module stub, so `src/App/FreeCAD.module.pyi` does not duplicate the
property vocabulary. Draft and BIM protocols may still narrow these generic
contracts when their workbench invariants are stronger.

`stubgen/type_hierarchy.py` discovers FreeCAD's C++ TypeId graph.
`stubgen/property_hierarchy.py` projects that graph onto `App::Property*`
classes and discovers Python conversion override declarations.
`property_contracts.py` combines those structural facts with
`PropertyPythonContracts.pyi`, resolving getter and setter contracts
independently so descendants such as `PropertyLength`, `PropertyDistance`,
`PropertyDirection`, and `PropertyLinkHidden` do not need separate metadata
entries. The generator checks that conversion overrides in covered families
have adjacent contracts; it does not infer Python types from C++ method bodies.

The metadata file is a stubgen input only and is excluded from the public stub
merge. C++ remains authoritative for inheritance and override boundaries; the
adjacent input is authoritative for the Python conversion shape at those
boundaries. Focused tests should accompany each new conversion family.

### C++-Registered Properties

Properties registered through C++ `ADD_PROPERTY(...)` calls are exposed by
the Python wrapper's dynamic property lookup rather than by direct PyCXX
binding members. `stubgen/cpp_properties.py` discovers those registrations,
matches them with the C++ property members, resolves their Python conversion
contracts, and adds the resulting getter/setter pairs to the generated public
class stubs. The source-adjacent binding stubs therefore stay focused on
directly bound members; a manually declared dynamic property should be removed
once this pipeline covers it.

Property conversion metadata remains source-adjacent to the owning module in
`PropertyPythonContracts.pyi` files. The App catalog supplies shared
conversion aliases, while module catalogs describe workbench-specific wrapper
types such as Part shapes, Mesh objects, and Sketcher constraints. Properties
whose owner has no public binding, or whose conversion depends on optional
external support such as VTK's Python wrappers, are intentionally left out
instead of being guessed. Generation reports discovered and emitted property
counts together with categorized diagnostics and a few source examples for
skipped cases. Existing declarations are treated as conflicts rather than
silently masking generated properties.

### Python Bootstrap Exports

The Python-defined bootstrap API is kept beside the compatibility-sensitive
startup code in `src/App/FreeCADInit.py`. Its `QUANTITY_CONSTANTS` and
`UNIT_CONSTANTS` tables are both the runtime declarations and the input for
`stubgen/init_exports.py`. The generator also recognizes class assignments
such as `App.Logger = FCADLogger`, `App.ScaleType = ScaleType`, and
`units.Scheme = Scheme`, so public bootstrap exports are sourced from their
actual installation sites rather than duplicated in the generator. It resolves
all of them to an intermediate `ModuleExport` model and merges typed members
into the public `FreeCAD` and `FreeCAD.Units` stubs. The adjacent
`src/App/FreeCADInit.pyi` contains only dynamically assigned logger methods.
Do not edit the generated member list directly; change the structured runtime
declaration or its typing supplement instead.

The init-export parser intentionally understands only literal declaration
tables, typed records, and direct class assignments to known bootstrap
receivers. It does not execute `FreeCADInit.py` or inspect a running FreeCAD
process. Missing table documentation receives a stable generated description;
explicit `doc` fields remain the preferred form for user-facing details.
Irregular exports remain curated until they have a stable structured source
representation.
Use package-shaped overlay paths that mirror the public import tree, such as
`src/Tools/typing/inputs/overlays/PySide/QtCore.pyi`. Third-party packages such as Pivy should
stay out of this tree until their stubs are ready to be maintained or
generated at the package source.

Public module overlays merge top-level symbols into generated modules instead of
replacing the whole file. Keep overlays focused on aliases, helper types, and
manual APIs that the generator still cannot model. Use source-adjacent
`*.module.pyi` files for module function signatures, helper support nodes, and
small explicit module functions that are still missing from the discovered
inventory. At the moment the remaining overlays should be small, mostly around
compatibility shims or other APIs that do not fit the structured source inputs
yet.

The helper also runs the smoke checks from this directory:

```sh
python3 src/Tools/typing/generate_stubs.py check --root . --out-dir src/Tools/typing/generated
```

`check-stubs.sh` also runs the focused `stubgen` unit tests, including the
structured property-contract catalog checks, before generating the disposable
stubs and invoking Pyright and Pyrefly.

Use the documentation linter to audit the curated source-adjacent stub files:

```sh
python3 src/Tools/typing/generate_stubs.py lint-docs --root .
```

This lint checks the curated source files that now carry hand-written typing
documentation, not the entire generated public stub tree. It requires module
docstrings plus docstrings on curated top-level functions, curated classes, and
their methods. Pass file or directory paths after `lint-docs` to audit a
smaller slice while documentation coverage is still being filled in.

## Recommended Direction

Prefer generated stubs for classes that already have binding `.pyi` specs.
Those files are close to the C++ wrapper source of truth and can be improved
without creating a second hand-written API surface.

Binding stubs should use the keyword-only `Metadata.deprecated(...)` decorator with
`deprecated_in` and `removed_in` releases. The public stub generator converts that
metadata to `typing_extensions.deprecated(...)` for binding classes,
source-adjacent type stubs, and `*.module.pyi` functions. Positional PEP 702
decorators are emitted only in the generated public stubs; source metadata is always
structured.
When binding classes use structured `@deprecated_attributes(...)` metadata, the
public stub generator rewrites those members as deprecated properties in the emitted
stubs so the lifecycle remains visible in the standard public typing surface.

When the same binding class is exported through multiple public module paths,
the merged public stubs keep one canonical class body and make the other
symbols re-export aliases. `FreeCAD.Base` is canonical for classes sourced from
`src/Base/`, which preserves type identity for APIs that use paths such as
`FreeCAD.Vector`, `FreeCAD.Base.Vector`, or `Part.Precision`.

Use source-adjacent plain `.pyi` files for PyCXX type method tables that the
inventory can map to a public class. These fragments are source inputs to the
generator, not the published stub tree. Use `@typing_only` on methods inside a
binding `.pyi` class when extra typing-only methods belong to that class and
should stay next to the binding source. Use class-body `if TYPE_CHECKING:`
blocks for typing-only attributes that should stay next to the binding source.
Use curated overlays for APIs that still need hand-written public module stubs,
including manual `PyMethodDef`, Boost.Python, or pybind code that is not
represented in the binding `.pyi` generator model. Keep these files focused on
public Python signatures. Avoid moving raw generated skeletons into the tree
without reviewing the signatures against the implementation.

When a manual API is large or actively changing, prefer adding generator input
for it instead of growing a large overlay. When it is small, stable, or hard to
model in the generator, a maintained overlay is the lower-risk option.

### Typing-only Members

Prefer source-side typing additions when they naturally belong to an existing
binding class.

- Use `@typing_only` for methods.
- Use class-body `if TYPE_CHECKING:` blocks for attributes.

This split matches the current binding parser behavior:

- the legacy method parser still walks class-body `if` blocks, so
  `if TYPE_CHECKING:` is not enough to hide methods from binding generation
- the legacy attribute parser only consumes top-level class attributes, so
  attributes inside `if TYPE_CHECKING:` stay stub-only

The public stub generator flattens class-body `if TYPE_CHECKING:` attribute
blocks into ordinary class members in the emitted stubs, so the published stub
surface stays clean.

## Maintenance Notes

Use `src/Tools/typing/generate_stubs.py` in scripts and documentation. Do not
introduce another entrypoint name for the same pipeline.

When a PyCXX type context still needs a manual rule, add it in
`src/Tools/typing/stubgen/type_context_rules.py`. Use an internal reason
for helper types that should not surface publicly, and use public targets only
when the current discovery path cannot map the context automatically.
