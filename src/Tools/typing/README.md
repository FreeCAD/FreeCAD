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

The helper runs the stub generator:

```sh
python3 src/Tools/typing/generate_stubs.py --root . --out-dir src/Tools/typing/generated
```

The implementation now lives under `src/Tools/typing/stubgen/`:
`model.py` for shared types and regexes, `parsing.py` for low-level source
parsing, `generator.py` for pipeline orchestration, and `cli.py` for argument
handling. The pipeline is further split across focused modules such as
`discovery.py`, `source_inputs.py`, `class_merge.py`, `module_merge.py`, and
`render.py`. `type_context_rules.py` holds the remaining manual PyCXX context
classifications that are not derivable yet.

That command writes under `src/Tools/typing/generated/`:

- `stubs/`: import-shaped public stubs with overlays applied, plus mapped
  PyCXX type method tables where the runtime type can be tied to a public or
  private stub name. This tree is suitable for a type-checker search path and
  is the output used by the smoke checks. It is disposable local output; the
  repository only keeps `generated/.gitignore`, so regenerate it instead of
  editing it directly.

Keep residual hand-written public overlays under `src/Tools/typing/inputs/overlays/`. Keep
source-adjacent PyCXX type signature inputs in plain `.pyi` files such as
`src/Gui/FreeCADGui._MainWindow.pyi` when curated type signatures should live
next to the wrapper source. Keep stub-only module function signatures and
support nodes in source-adjacent `*.module.pyi` files such as
`src/App/FreeCAD.module.pyi` or `src/Base/FreeCAD.Console.module.pyi`.

These source-adjacent stub files are consumed by `stubgen` only. The
legacy binding generator still reads only the binding `.pyi` files that are
registered through CMake. Plain type-stub `.pyi` files can also contribute
top-level support nodes such as imports, helper aliases, helper protocols, and
non-method class members to the merged public stub output. Do not edit
generated output directly; edit the curated source inputs and regenerate.

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
