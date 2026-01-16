# FreeCAD Issues to Watch

**Date**: 2026-01-15

## High Priority / Blocker Issues

| Issue | Title | Status | Notes |
|-------|-------|--------|-------|
| [#26677](https://github.com/FreeCAD/FreeCAD/issues/26677) | PartDesign: Pad-to-face fails on filleted faces | Blocker, 1.1 | Regression from 1.0.2 |
| [#25720](https://github.com/FreeCAD/FreeCAD/issues/25720) | PartDesign: 1.1RC1 errors recomputing 1.0.X files | Blocker, 1.1 | Assigned to kadet1090 |
| [#26726](https://github.com/FreeCAD/FreeCAD/issues/26726) | Regression: Linux mouse pointer changed | Blocker, 1.1 | |

## High Priority, Potentially Approachable

| Issue | Title | Area | Notes |
|-------|-------|------|-------|
| [#20834](https://github.com/FreeCAD/FreeCAD/issues/20834) | Part: shp.mirror() broken with Placement | Part/Transforms | **INVESTIGATING** - TNP related regression |
| [#21250](https://github.com/FreeCAD/FreeCAD/issues/21250) | Sketcher: Move/Array causes hang | Sketcher/Perf | Performance issue with multiple copies |
| [#6196](https://github.com/FreeCAD/FreeCAD/issues/6196) | Part: Thickness fails on concave shapes | Part/OCC | Long-standing, OCC 7.5 regression |
| [#17689](https://github.com/FreeCAD/FreeCAD/issues/17689) | Part: Boolean common regression | Part | |

## Related to Our Work

| Issue | Title | Status | Notes |
|-------|-------|--------|-------|
| [#15663](https://github.com/FreeCAD/FreeCAD/issues/15663) | App::Link external geometry | Our fix pending | Awaiting maintainer feedback |
| [#12820](https://github.com/FreeCAD/FreeCAD/issues/12820) | PropertyLinkSub for Part::Extrusion | Our proposal pending | Awaiting maintainer feedback |

## Notes

- #20834 regression window: 0.22.0dev.37302 (good) → 0.22.0dev.38043 (bad), around May 2024 when TNP enabled
- #6196 related to OCC upgrade from 7.3.0 to 7.5.0
