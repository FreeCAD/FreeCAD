## Answers to Questions

### 1. Is this bug present in LinkStage3?

**The bug was NOT introduced by realthunder.** Looking at the git history:

1. **Commit `8e91aad2f0`** (Feb 22, 2024, by Zheng Lei/realthunder): Added `makEMirror` declaration to TopoShape.h, but **no implementation** was added to TopoShapeExpansion.cpp - only `makESlice` and `makESlices` were implemented.

2. **Commit `dbe7c9d372`** (Feb 22, 2024, by bgbsww): Added the `makeElementMirror` implementation with the buggy placement code:
   ```cpp
   TopLoc_Location loc = shape.getShape().Location();
   gp_Trsf placement = loc.Transformation();
   mat = placement * mat;  // <-- BUG introduced here
   ```

So the bug was introduced during the "cleanups and tests" commit when the implementation was written, not in realthunder's original transfer. It's unclear whether LinkStage3 has a different implementation of `makEMirror` - the declaration was transferred but the implementation appears to be original to the FreeCAD main branch.

### 2. Do we need compatibility handling for 1.0 files?

This is an important question. There are two contexts where `makeElementMirror()` is called:

#### A. Python scripting: `shape.mirror(base, normal)`

This is a **one-shot operation**. The user runs it, sees the result immediately, and if they want to keep it, they use it in their model. Nothing is persisted about the mirror operation itself - only the resulting shape (if used). If the result was wrong in 1.0, the user would have seen that immediately and either:
- Adjusted their approach
- Worked around it manually
- Not used the result

**No compatibility issue here** - there's nothing stored that references the buggy behavior.

#### B. Part::Mirroring feature (GUI: Part → Mirror)

This IS parametric. Looking at `FeatureMirroring.cpp` line 317:
```cpp
this->Shape.setValue(TopoShape(0).makeElementMirror(shape, ax2));
```

The feature stores:
- `Source` - the object to mirror
- `Base` / `Normal` or `MirrorPlane` - the mirror plane definition

On recompute, it calls `makeElementMirror()` with these stored parameters.

**This is where compatibility matters.** If a user created a Part::Mirroring feature in 1.0:
1. The buggy `makeElementMirror()` computed the wrong result
2. The user saw the wrong position but may have adjusted their mirror plane parameters to compensate
3. Their file stores those adjusted parameters
4. After the fix, recomputing with those same parameters will now give a *different* result

### What the bug actually does

The bug double-applies the source shape's placement. Here's what happens:

```
Source shape at position P (has non-identity Placement)
Mirror plane M

CORRECT behavior:
  result = mirror(shape_at_P, M)
  → Shape appears at mirror of P across M

BUGGY behavior:
  result = mirror(shape_at_P, P * M)  // mirror plane shifted by P
  → Shape appears at wrong location
```

For shapes with identity placement (Placement at origin, no rotation), the bug has no effect because `P` is identity.

### Recommendation

For shapes created with **identity Placement**, the bug never manifested, so no compatibility issue.

For shapes created with **non-identity Placement**:
- Python scripting: User would have seen wrong result immediately, no stored state
- Part::Mirroring: User may have compensated by adjusting mirror plane

The question is: how many users actually used Part::Mirroring with non-identity Placement sources and compensated for the bug?

**Options:**
1. **Just fix it** - Accept that rare Part::Mirroring features using non-identity Placement sources may shift. Users can adjust their mirror plane parameters.
2. **Add migration code** - Detect 1.0 files and... do what exactly? We can't know if the user compensated for the bug or not.

I lean toward option 1. The bug is clearly wrong, and adding migration complexity for an edge case (Part::Mirroring + non-identity Placement + user compensation) seems like overkill. Users who hit this can simply adjust their mirror plane parameters.

### The Fix

The fix simply removes the incorrect placement extraction and multiplication:

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
    // Note: Do NOT extract and pre-multiply the shape's Location/Placement here.
    // BRepBuilderAPI_Transform correctly handles shapes with Location already.
    // Pre-multiplying would double-apply the placement, causing incorrect results
    // for shapes with non-identity Placement. See GitHub issue #20834.
    BRepBuilderAPI_Transform mkTrf(shape.getShape(), mat);
    return makeElementShape(mkTrf, shape, op);
}
```

This matches the behavior of the original `TopoShape::mirror()` function (TopoShape.cpp line 3331) which works correctly and doesn't manipulate the placement.
