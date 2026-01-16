# Issue #20834: shp.mirror() Broken with Placement - Analysis

**Issue**: https://github.com/FreeCAD/FreeCAD/issues/20834
**Status**: Analyzing
**Date**: 2026-01-15

## Problem Summary

When mirroring a shape that has a non-identity Placement, the result is incorrect. Two identical boxes positioned the same way (one via direct coordinates, one via explicit Placement) produce different mirror results.

## Root Cause

The bug is in `TopoShape::makeElementMirror()` in `src/Mod/Part/App/TopoShapeExpansion.cpp` (lines 3991-3996):

```cpp
gp_Trsf mat;
mat.SetMirror(ax2);
TopLoc_Location loc = shape.getShape().Location();
gp_Trsf placement = loc.Transformation();
mat = placement * mat;  // <-- BUG: This incorrectly combines placement with mirror
BRepBuilderAPI_Transform mkTrf(shape.getShape(), mat);
```

The problem: `shape.getShape()` already includes the Location (Placement). When passing it to `BRepBuilderAPI_Transform`, the transform is applied to the shape including its location. By pre-multiplying `placement * mat`, the code is effectively double-applying the placement.

## Working Code Comparison

The original `TopoShape::mirror()` function (still in TopoShape.cpp line 3331) works correctly:

```cpp
TopoDS_Shape TopoShape::mirror(const gp_Ax2& ax2) const
{
    gp_Trsf mat;
    mat.SetMirror(ax2);
    BRepBuilderAPI_Transform mkTrf(this->_Shape, mat);  // Just apply mirror directly
    return mkTrf.Shape();
}
```

It does NOT extract and pre-multiply the placement - it just applies the mirror transform directly.

## When Bug Was Introduced

The `makeElementMirror()` function was added in commit `dbe7c9d372` (Feb 22, 2024) as part of the TNP (Topological Naming Problem) migration. The Python binding `TopoShapePy::mirror()` was changed to call `makeElementMirror()` instead of the old `mirror()` when TNP is enabled.

Bisect results from the issue:
- **Last good**: 0.22.0dev.37302 (0e24e121eb)
- **First bad**: 0.22.0dev.38043 (561e521817)

## Proposed Fix

Remove the incorrect placement handling from `makeElementMirror()`:

```cpp
TopoShape& TopoShape::makeElementMirror(const TopoShape& shape, const gp_Ax2& ax2, const char* op)
{
    if (!op) {
        op = Part::OpCodes::Mirror;
    }

    if (shape.isNull()) {
        FC_THROWM(NullShapeException, "Null shape");
    }
    gp_Trsf mat;
    mat.SetMirror(ax2);
    // REMOVED: The placement extraction and multiplication was incorrect.
    // BRepBuilderAPI_Transform already handles the shape's Location correctly.
    BRepBuilderAPI_Transform mkTrf(shape.getShape(), mat);
    return makeElementShape(mkTrf, shape, op);
}
```

## Test Script

See `test_mirror_bug.py` for a reproduction script.

## Questions to Verify

1. Why was the placement multiplication added in the first place? Was there a specific case it was trying to handle?
2. Are there other `makeElement*` transform functions with similar issues?
3. Does the element map need any special handling after the fix?

## Related Functions to Check

Other transform functions in TopoShapeExpansion.cpp that might have similar issues:
- `makeElementTransform()`
- `makeElementCopy()`
- Any other function that extracts Location and pre-multiplies it

## Technical Details

### What is "non-identity Placement"?

Every FreeCAD object has a `Placement` property (position + rotation).

- **Identity Placement**: Position at (0,0,0), no rotation. The object's geometry is exactly where its internal coordinates define it.
- **Non-identity Placement**: The object has been moved/rotated. The visible geometry = internal geometry × Placement transform.

Two ways to position a box at (0, 30, 0):
1. `Part.makeBox(10, 20, 30, V(0, 30, 0), ...)` - geometry itself at (0,30,0), identity Placement
2. `Part.makeBox(10, 20, 30)` then set `Placement = (0,30,0)` - geometry at origin, non-identity Placement

Both appear identical visually, but the bug treats them differently.

### Mirror plane used in test

The test mirrors across the **XZ plane** (Y=0):
- Base point: origin `(0, 0, 0)`
- Normal: Y axis `(0, 1, 0)`

A point `(x, y, z)` mirrors to `(x, -y, z)`.

### The double-application bug

```
Source shape at position P (non-identity Placement)
Mirror plane M

CORRECT: result = mirror(shape_at_P, M)
BUGGY:   result = mirror(shape_at_P, P * M)  // mirror plane shifted by P
```

For identity Placement (P = identity matrix), bug has no effect.
