# Finding and extending existing code

[Guide index](README.md) · [Architecture](architecture.md) · [Writing code](writing-code.md)

Reuse preserves more than code. An existing implementation may also preserve element maps,
units, exception conversion, user preferences, transaction boundaries, and difficult edge cases.
Reimplementing its visible computation can silently lose those contracts.

## Search by responsibility

Start in the affected feature and its siblings. Follow shared widgets, base classes, extensions,
and module helpers. Then inspect lower layers and libraries already linked by the module.
Search for the operation under several names and examine actual call sites.

Examples of targeted searches from the repository root:

```sh
rg -n 'setUnit|bind\(' src/Mod/Part/Gui
rg -n 'makeElementCut|makeElementFuse' src/Mod/Part/App
rg -n 'StateLocker|ScopeGuard' src/Base src/Gui
rg -n 'getGlobalPlacement|splitSubName' src/App src/Mod/Part
```

For each candidate, check its preconditions, result, ownership, failure behavior, dependencies,
and tests. A similarly named function may operate in a different coordinate system or retain a
different reference path. The shortest call is useful only if its contract matches the task.

## A map of existing facilities

This is a starting map, not a list of dependencies every module may acquire.

| Need | Places to inspect | Contract to check |
| --- | --- | --- |
| Vectors, transforms, bounds, and quantities | [Base](../../src/Base), including `Vector3D.h`, `Placement.h`, `Rotation.h`, `BoundBox.h`, and `Quantity.h` | Units, coordinate convention, numerical precision |
| Geometric tolerances | [Base/Precision.h](../../src/Base/Precision.h), OpenCASCADE precision utilities in Part | Linear versus angular, squared versus unsquared, algorithm-specific tolerance |
| Vector/point conversion | [Base/Converter.h](../../src/Base/Converter.h), [Part/App/Tools.h](../../src/Mod/Part/App/Tools.h) | Traits and dependency ownership |
| Boolean and mapped shape operations | [TopoShape.h](../../src/Mod/Part/App/TopoShape.h), [TopoShapeMapper](../../src/Mod/Part/App/TopoShapeMapper.h) | Element mapping, operation identity, result validity |
| Reference paths and global placements | [GeoFeature.h](../../src/App/GeoFeature.h), [DocumentObserver.h](../../src/App/DocumentObserver.h), [Base/Tools.h](../../src/Base/Tools.h) | Root, subname, link instance, lifetime |
| Scope cleanup and temporary state | [Base/Tools.h](../../src/Base/Tools.h), Qt scope and signal-blocking helpers | Restore previous state, ownership, exception behavior |
| Document and selection observation | [DocumentObserver.h](../../src/App/DocumentObserver.h), [Selection.h](../../src/Gui/Selection/Selection.h) | Disconnect and owner lifetime |
| Unit-aware input and task interaction | [QuantitySpinBox.h](../../src/Gui/QuantitySpinBox.h), [TaskDialog.h](../../src/Gui/TaskView/TaskDialog.h) | Unit setup, binding, accept/reject semantics |
| Plane, cylinder, or sphere fitting | [Mesh approximation](../../src/Mod/Mesh/App/Core/Approximation.h), [CylinderFit.h](../../src/Mod/Mesh/App/Core/CylinderFit.h), [SphereFit.h](../../src/Mod/Mesh/App/Core/SphereFit.h) | Input representation, degeneracy handling, module dependency |
| Python geometric utilities | [DraftGeomUtils.py](../../src/Mod/Draft/DraftGeomUtils.py), [DraftVecUtils.py](../../src/Mod/Draft/DraftVecUtils.py), [draftutils](../../src/Mod/Draft/draftutils), Part shape methods | GUI independence, tolerances, public API stability |

Use Eigen and other existing dependencies for the numerical operations they provide. A fit
helper in Mesh is evidence that the algorithm exists, not permission to make core App depend
on Mesh. First ask whether the shared computation can live in an appropriate lower layer.

## Convert through the established representation

Where Part is already a valid dependency, its traits integrate OpenCASCADE types with the Base
conversion facility. Rather than repeating coordinate extraction at each call site:

```cpp
#include <Mod/Part/App/Tools.h>

gp_Pnt asPoint(const Base::Vector3d& position)
{
    return Base::convertTo<gp_Pnt>(position);
}
```

The specialization belongs in Part because Base must not depend on OpenCASCADE. Do not move
that include into Base just to make the conversion available everywhere. Check whether an
existing conversion already expresses the direction and precision required before adding one.

## Reuse the behavior, not just the widget

Path pattern and array controls share
[PatternPathParametersWidget.cpp](../../src/Mod/Part/Gui/PatternPathParametersWidget.cpp).
Its spacing and offset controls set the length unit before binding the properties. Correcting
that shared setup covers its consumers without changing persisted property types or patching
each workbench separately. Inspect all consumers to ensure the shared behavior is intended.

Similarly, use a standard line edit's existing clear action before adding another button and
handler. Check a widget's complete API and its use in sibling panels, not just its constructor.

## Decide how much to share

| Situation | Usually appropriate |
| --- | --- |
| Existing helper has the same contract | Call it directly |
| One local operation is hard to read | Extract a named private method or small local function |
| Several callers share one invariant | Extend their common mechanism and cover all callers |
| Two modules need genuinely independent lower-layer behavior | Extract that behavior in a preparatory change and migrate the callers |
| Similar-looking code has different ownership or semantics | Keep the distinction explicit instead of forcing a shared abstraction |
| A possible future caller is the only reason for a framework | Keep the implementation local until the need is concrete |

Extraction should reduce coupling. Moving workbench-specific assumptions into Base produces a
wider problem, even if it removes duplicated lines. Identify the minimal neutral inputs and
result, keep GUI and document orchestration in their owners, and test the extracted contract.
Preserve existing APIs or migrate their callers deliberately.

For a task-panel fix, a local base class can establish ownership for the panels that currently
need it. Promote it later if another independent consumer proves the general contract. The
local `_BaseTaskPanel` in [DraftGui.py](../../src/Mod/Draft/DraftGui.py) illustrates this boundary.

## Make reuse demonstrable

Be able to explain which existing mechanisms were considered, which one was extended, and why
alternatives do not fit. This can be a short design note, not an inventory of every search.
If new code is necessary, keep it to the missing responsibility and leave one authoritative
implementation of each shared rule. Re-run the existing callers' relevant tests after changing
that implementation.
