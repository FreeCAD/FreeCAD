# Architecture and integration

[Guide index](README.md) · [Code reuse](code-reuse.md) · [Documents and geometry](documents-and-geometry.md)

Place behavior with the layer that owns its invariants. This makes it available to the right
callers, keeps headless operation possible, and limits the impact of changes.

## Identify the owner

| Area | Responsibilities | Dependencies to avoid introducing |
| --- | --- | --- |
| `src/Base` | General facilities independent of documents and workbenches | App, Gui, workbench or OpenCASCADE-specific concepts |
| `src/App` | Documents, properties, links, dependency evaluation, persistence | Gui and workbench internals |
| `src/Gui` | Common commands, views, selection, widgets, workbench infrastructure | Knowledge of particular workbench feature classes |
| `src/Mod/<Module>/App` | Domain features, calculations, module APIs and bindings | Its Gui layer or assumptions about an active view |
| `src/Mod/<Module>/Gui` | Commands, view providers, interaction and presentation | Unnecessary dependencies on other workbenches' implementation details |

This describes the direction to preserve when adding code. It is not a claim that every
historical file already follows it. Qt types do not by themselves make an API GUI-dependent;
examine which Qt module and runtime services are required.

Before adding a dependency, inspect the module's CMake linkage, optional build settings, import
paths, and initialization order. A call that works after every workbench has been activated
can still fail at startup, during restore, or in `FreeCADCmd`.

## Separate computation, presentation, and interaction

A feature's geometric result should come from its App-side properties and dependencies. A view
provider presents the result. A task panel manages interactive editing, input, and preview
lifetime. A command establishes an operation's context and transaction.

Keep a calculation callable without opening the panel that edits its inputs. A task-widget
address stored in an App feature prevents scripted and headless use. A view provider that
reaches into a specific panel also makes presentation depend on whether that panel exists.
Instead, expose the domain state through the appropriate existing property or API and let the
panel invoke it through the established command/transaction flow.

Use a generic hook when a module needs to specialize a core mechanism. The core contract must
make sense without naming that module. Prefer overriding the right responsibility to adding
type switches in core for each workbench's concrete class. A hook with only vague parameters
and no clear invariant is not an improvement; use a real caller to define it.

## Let modules contribute commands

The core may own a common menu while a module owns its entries. Do not hardcode a Part command
in core Gui and assume PartGui has already registered it. The existing
[Gui::WorkbenchManipulator](../../src/Gui/WorkbenchManipulator.h) provides an extension point.
Part's [implementation](../../src/Mod/Part/Gui/WorkbenchManipulator.cpp) contributes its Datums
command to the common menu and toolbar after the owning module loads.

Check the whole lifecycle: before loading the module, after loading it, after switching
workbenches, and with the module disabled. An action renamed to `Std_*` still has the same
implementation dependency. Startup availability needs a deliberate loading contract, not a
new prefix or an incidental import from an unrelated module.

## Keep geometric and rendering contracts intact

For operations on mapped Part shapes, inspect the existing `TopoShape::makeElement...` and
Part wrappers before calling the kernel directly. A raw OpenCASCADE result alone does not
retain FreeCAD's topological naming metadata. Low-level kernel calls can belong inside a Part
implementation, but their results must participate in the established mapping and error flow.

Use Coin nodes and the existing scenegraph abstractions for view rendering. A direct drawing
call from a task panel can bypass selection, export, and render-action state. Before introducing
another rendering path, establish why the existing path cannot express the requirement.

For dialogs and task panels, prefer the module's established `.ui` and widget/resource pattern.
Dynamic construction may be appropriate for genuinely variable content; it should not recreate
the layout and translation responsibilities already handled by the standard pattern.

## Design shared APIs around neutral inputs

Suppose two workbenches need the same geometric computation. Passing one workbench's task
panel to a shared helper couples the computation to that workbench's interaction. Passing
geometry, an explicit tolerance, and the needed mode can make a genuinely reusable API.
First inspect [existing facilities](code-reuse.md) and validate numerical and mapping semantics.

Separate performance, policy, and presentation decisions. For example, a build-provenance
choice is not inherently the same choice as debug information or compiler optimization.
Coupling independent settings because they happen to correlate on one machine makes other
supported workflows impossible. Prefer an explicit contract and measure the tradeoff.

## Integrate a feature completely

Follow a neighboring feature all the way through the repository. Typical registration points
are shown below; not every feature needs every item.

| Part of the feature | Registration to inspect |
| --- | --- |
| New App sources and types | `App/CMakeLists.txt`, module initialization, type initialization |
| Python exposure | Binding specs, handwritten implementation, exported names and maintained declarations |
| GUI sources and forms | `Gui/CMakeLists.txt`, `.ui` sources, GUI initialization |
| Commands | Command construction and the module's command registration function |
| Workbench integration | Menu, toolbar, task watcher or existing shared command group |
| Resources | SVG icon and resource collection entries |
| Tests | Source lists, suite/aggregator imports, registration only for genuinely new suites |

The handbook's [feature integration checklist](https://freecad.github.io/DevelopersHandbook/technical/ChecklistForNewFeatureC%2B%2B)
provides an overview. Verify filenames and registration mechanisms in this checkout, especially
when working on a newer binding generator or a Python workbench.

Test the new entry through the workflow users actually take. A compiled source file is not
proof that a command is registered, an icon is packaged, or the command works before another
workbench is loaded.

## Review the resulting dependency direction

Can the domain behavior run without the GUI? Do optional modules remain optional? Are shared
callers using one authoritative mechanism? Can ownership and failure behavior be explained
without knowing a particular dialog's implementation? Resolve these questions before expanding
the patch with another abstraction. Significant new contracts belong in
[design discussion](design-proposals.md) before a broad implementation.
