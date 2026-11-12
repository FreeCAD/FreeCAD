# Documents, dependencies, and geometry

[Guide index](README.md) · [Architecture](architecture.md) · [Testing](testing-and-validation.md)

A feature participates in a persistent parametric model. Correctness includes the model's
dependencies, undo history, saved representation, Python API, and references to earlier geometry.
A visually correct shape after one forced recompute is not sufficient evidence.

## Work through the document lifecycle

Trace an operation through creation, property changes, recompute, undo/redo, save, restore, and
deletion. Distinguish normal execution from restore, where related objects and properties may
not yet be ready. Use the transaction mechanism already owned by the operation: GUI command
transactions, `App::AutoTransaction`, or Python document transactions as appropriate.

Do not add a transaction inside a helper just because it changes a property. Its caller may
already own the whole user operation. Inspect accept, cancel, preview, and failure paths so
the change neither leaves an open transaction nor commits half an operation.

Study [DocumentObject.h](../../src/App/DocumentObject.h), the affected feature's overrides, and
the handbook's [property-change overview](https://freecad.github.io/DevelopersHandbook/technical/PropertyChanges).
In particular, establish what `onChanged()` invalidates, what `mustExecute()` requests, and
what `execute()` consumes. Calling execution directly from a property callback can recurse
when execution changes the same property again.

## Invalidation and evaluation order are different

Consider a Boolean feature that reads external tool geometry in the receiving Body's local
coordinates. Its result depends on the tool, the receiving Body's placement, and possibly
the placements of parent Parts. A changed parent placement can invalidate the result even
though the Boolean's direct shape input did not change.

There are two separate questions:

1. Does a relevant change mark the result for recompute?
2. When it executes, have the placements and expressions it reads reached their new values?

A full recompute can mask the first problem without solving the second. Adding a conventional
dependency from a feature to its containing Body may also introduce a cycle because the Body
already depends on its features. Use the established observer/recompute mechanism for the
specific dependency, with safe disconnection and restore behavior.

Inspect [ShapeBinder](../../src/Mod/PartDesign/App/ShapeBinder.cpp) for placement observation and
the actual feature before choosing a pattern. A binder and a Boolean need not have identical
semantics. Test movement after creation, expression-driven movement, parent Parts, unrelated
objects, and restore; setting all placements before creating the feature misses the regression.

## Preserve persisted meanings

Property names, types, defaults, enum order, units, and reference flags form part of the document
schema. `PropertyEnumeration::Save()` in
[PropertyStandard.cpp](../../src/App/PropertyStandard.cpp) stores an integer index. Reordering
its fixed list can reinterpret an old document's value as a different mode. Prefer retaining a
deprecated entry; any removal or schema change needs an explicit migration strategy.

Use the affected class's existing property-rename and restore hooks, such as
`handleChangedPropertyName()` and `onDocumentRestored()`, rather than teaching every caller
multiple versions of the schema. Test documents produced by relevant earlier versions,
including development versions when their behavior is part of the report.

Replacing several mutually exclusive flags with an enum may improve a new design. In an
existing feature, the change also affects file restoration, expressions, scripts, and possibly
addons. Keep the compatibility work visible and separate from a presentation-only fix.

## Establish the schema centrally

[Draft's B-spline object](../../src/Mod/Draft/draftobjects/bspline.py) has a compatibility helper
that establishes an absent property. A shortened version of that helper is:

```python
def assureProperties(self, obj):
    if not hasattr(obj, "Parameterization"):
        tip = QT_TRANSLATE_NOOP("App::Property", "Parameterization factor")
        obj.addProperty("App::PropertyFloat", "Parameterization", "Draft", tip, locked=True)
        obj.Parameterization = 1.0
```

This is a genuine use of `hasattr`: the old saved schema can lack the property. Normal execution
can then rely on `obj.Parameterization`. A repeated `getattr(obj, "Parameterization", 1.0)` in
every calculation would conceal an incomplete migration and spread the default throughout the
code. This object calls its idempotent compatibility helper during creation and execution.
For a new migration, prefer the established restore route when it can establish the schema
before use. Inspect the affected feature's creation, execution, and restore routes rather than
assuming another object's lifecycle is identical.

## Resolve the selected instance and coordinate context

An object's `Placement` is not necessarily its global transform. A nested Part, a Body, a link,
and a linked array can introduce different contexts. The selected tree entry may be an instance
of another object, with a subelement path through containers and links.

Use [GeoFeature's placement APIs](../../src/App/GeoFeature.h) with the required target, root,
and subname context. Inspect the overload's contract. Flattening the selection immediately to
the source can lose the instance placement; keeping only the final face name can lose the
container needed to resolve it.

For example, transforming a geometric link should edit that instance's placement. The source
can be inspected to decide whether it represents geometry, but the command must not then
modify the source instead. A link to a spreadsheet is a relevant negative case. Check ordinary
links, chains, arrays, read-only placements, and the menu/keyboard entry paths affected by a
shared command filter.

The reference helpers in [DocumentObserver.h](../../src/App/DocumentObserver.h), including
`DocumentT`, `DocumentObjectT`, and `SubObjectT`, are useful places to inspect for re-resolution
and path representation. Read their actual lifetime and identity contracts before using them.

## Treat reference repair separately from dependency repair

A dress-up feature can have both a base-feature dependency and a separate link carrying selected
subelements. Inserting a feature can update one while leaving the other stale. Editing the
references later can restore the stale dependency and bypass the inserted feature. Inspect
[Body.cpp](../../src/Mod/PartDesign/App/Body.cpp) and
[FeatureDressUp.cpp](../../src/Mod/PartDesign/App/FeatureDressUp.cpp) for the rerouting context.

Synchronizing the intended base does not establish that every selected edge survives. Insertion
and deletion have different geometry available and different reasons to attempt recovery.
Preserve element-map metadata. A displayed `?Edge...` is an unresolved historical reference,
not proof that the displayed edge index identifies the intended current edge.

When geometric recovery is appropriate, establish a unique valid match. Split, changed, missing,
or ambiguous edges may need manual repair. A test should reject a plausible but wrong match,
not just demonstrate that the error marker disappeared. The related
[fillet tests](../../src/Mod/PartDesign/PartDesignTests/TestFillet.py) include distinct matching
and unresolved cases.

## Keep kernel, numerical, and API contracts explicit

For mapped shapes, prefer `TopoShape` operations that retain naming information over raw
kernel results. Validate geometry appropriate to the operation: validity, solid count, volume,
position, and orientation. Do not assume a shape is analytically circular solely because the
modeling workflow started from a circle; inspect the resulting representation and the consuming
operation's requirements.

Use named tolerances with their intended dimensions and units. An angular tolerance is not a
linear distance, and a squared-distance comparison needs a squared tolerance. Existing precision
utilities express defaults for particular purposes; copying a number from them does not justify
changing an algorithm's tolerance.

Public Python behavior includes argument conventions, return types, exceptions, units, and
names. Changing a signature while keeping the name can still break addons. Edit binding specs
and handwritten implementation sources, not generated bindings/stubs. Do not duplicate a
signature in a docstring when the binding generator derives it from the declaration.

Inspect current examples such as [TopoShape.pyi](../../src/Mod/Part/App/TopoShape.pyi) for the
deprecation mechanisms supported by this checkout. Explain the replacement and transition
when a break cannot be avoided, and verify both runtime and maintained declarations.

Preference migration needs the same care: an absent key, an explicitly saved false value, and
a legacy key are different states. Changing a widget's default does not necessarily change
the value loaded by the application. Define the default in its owning mechanism and test both
fresh and existing settings.
