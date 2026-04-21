# Optimization: Pre-fuse tool shapes in Polar/Linear Pattern

## The Problem

`Transformed::execute()` in `FeatureTransformed.cpp` is slow for patterns with many occurrences (e.g., polar pattern with 20+ copies). The bottleneck is **element naming**, not the boolean geometry itself.

### Why element naming is expensive here

FreeCAD's topological naming system (`makeElementFuse`/`makeElementCut`) tracks the history of every face and edge from every input shape. This is what allows downstream features to reference "Face3 of PolarPattern001" and have that reference survive model edits.

Currently, `Transformed::execute()` builds a vector of **N+1 shapes** (the support shape + N transformed copies) and passes them all to a single `makeElementFuse` or `makeElementCut` call:

```cpp
// Current code (simplified)
auto getTransformedCompShape = [&](const auto& supportShape, const auto& origShape) {
    std::vector<TopoShape> shapes = {supportShape};
    TopoShape shape(origShape);
    int idx = 1;
    for (each transformation after the first) {
        shapes.emplace_back(shape.makeElementTransform(*transformIter, opName));
    }
    return shapes;
};

// Then:
supportShape.makeElementFuse(shapes);  // passes N+1 shapes
```

Each `makeElementTransform` call creates a new `TopoShape` with full element mapping. Then `makeElementFuse` must reconcile the element maps of all N+1 inputs. The element naming cost scales **with the number of input shapes**, making it O(N) where N is the occurrence count.

For a polar pattern with 60 occurrences, this means the element naming system processes 61 shapes, tracking hundreds or thousands of face/edge histories.

### Where the time goes

Profiling shows that `makeShapeWithElementMap` (called internally by `makeElementFuse`/`makeElementCut`) dominates execution time. The actual OCCT boolean operation is fast; it's the bookkeeping of "which output face came from which input face of which input shape" that's expensive.

## The Proposed Solution

**Pre-fuse the transformed tool copies using raw OCCT operations (no element naming), then pass only 2-3 shapes to the element-mapped boolean.**

Instead of:
```
makeElementFuse([support, copy1, copy2, ..., copyN])  // N+1 inputs, full element naming
```

Do:
```
rawFuse = OCCT_Fuse(copy1, copy2, ..., copyN)  // N inputs, NO element naming (fast)
makeElementFuse([support, originalTool, rawFuse])  // 3 inputs, full element naming
```

The element naming system now only sees 2-3 shapes regardless of N. The raw OCCT pre-fuse handles the geometric combination of copies without any naming overhead.

### Why this works

1. **Element naming cost is per-input-shape**: Reducing from N+1 to 2-3 inputs dramatically cuts the naming work.
2. **Raw OCCT booleans are fast**: `BRepAlgoAPI_Fuse` without element mapping is just geometry — no history tracking.
3. **The pre-fuse can run in parallel**: OCCT's `SetRunParallel(true)` enables multi-threaded boolean execution.
4. **The original tool shape retains element mapping**: By keeping the first (untransformed) tool as a proper `TopoShape`, downstream references to the original feature's faces still work.

### What we lose

Faces from the pre-fused copies (copies 2 through N) won't have individual per-copy element names. If a user references "Face X on the 5th copy of the polar pattern," that reference won't have the same naming stability as before. However:

- In practice, users rarely reference individual faces of pattern copies
- The original (first) tool instance retains full element naming
- The support shape retains full element naming
- This is a pragmatic trade-off: much faster execution for slightly reduced naming granularity on copies

## Implementation

### Overview of changes to `FeatureTransformed.cpp`

Three modifications:

1. **Add a `preFuseShapes` static helper** (before `Transformed::execute()`)
2. **Replace the `getTransformedCompShape` lambda** with `getRawTransformedShapes`
3. **Update the fuse/cut call sites** in both `Mode::Features` and `Mode::WholeShape`

### 1. Static helper: `preFuseShapes`

Insert before `Transformed::execute()` (currently at line 304). No new includes needed — `FCBRepAlgoAPI_Fuse.h` is already included at line 28.

```cpp
/// Pre-fuse multiple shapes using raw OCCT boolean operations, bypassing
/// element naming. Returns a single fused TopoDS_Shape. This is used to
/// combine transformed tool copies before the final element-mapped boolean
/// with the support shape, reducing element naming cost from O(N) to O(1).
static TopoDS_Shape preFuseShapes(const std::vector<TopoDS_Shape>& shapes)
{
    if (shapes.empty()) {
        return {};
    }
    if (shapes.size() == 1) {
        return shapes[0];
    }

    FCBRepAlgoAPI_Fuse fuser;
    fuser.SetRunParallel(Standard_True);

    TopTools_ListOfShape arguments, tools;
    arguments.Append(shapes[0]);
    for (size_t i = 1; i < shapes.size(); ++i) {
        tools.Append(shapes[i]);
    }

    fuser.SetArguments(arguments);
    fuser.SetTools(tools);
#if OCC_VERSION_HEX >= 0x070600
    fuser.Build(OCCTProgressIndicator::getAppIndicator().Start());
#else
    fuser.Build();
#endif
    if (!fuser.IsDone()) {
        throw Base::CADKernelError("Pre-fuse of transformed shapes failed");
    }

    return fuser.Shape();
}
```

**Key points:**
- Uses `FCBRepAlgoAPI_Fuse` (FreeCAD's wrapper around `BRepAlgoAPI_Fuse`) which is already included
- Returns a raw `TopoDS_Shape`, not a `Part::TopoShape` — no element naming overhead
- `SetRunParallel(Standard_True)` enables multi-threaded OCCT execution
- Handles the OCC version check for progress indication (same pattern used elsewhere in the codebase)

### 2. Replace the lambda

**Remove** the current `getTransformedCompShape` lambda (lines 367-381):

```cpp
// REMOVE THIS:
auto getTransformedCompShape = [&](const auto& supportShape, const auto& origShape) {
    std::vector<TopoShape> shapes = {supportShape};
    TopoShape shape(origShape);
    int idx = 1;
    auto transformIter = transformations.cbegin();
    transformIter++;
    for (; transformIter != transformations.end(); transformIter++) {
        if (OCCTProgressIndicator::getAppIndicator().UserBreak()) {
            return std::vector<TopoShape>();
        }
        auto opName = Data::indexSuffix(idx++);
        shapes.emplace_back(shape.makeElementTransform(*transformIter, opName.c_str()));
    }
    return shapes;
};
```

**Replace with:**

```cpp
// Build raw OCCT transformed copies of the tool shape (no element naming).
auto getRawTransformedShapes = [&](const TopoDS_Shape& origShape) {
    std::vector<TopoDS_Shape> rawShapes;
    rawShapes.reserve(transformations.size() - 1);
    auto transformIter = transformations.cbegin();
    transformIter++;  // skip identity transform
    for (; transformIter != transformations.end(); transformIter++) {
        if (OCCTProgressIndicator::getAppIndicator().UserBreak()) {
            return std::vector<TopoDS_Shape>();
        }
        BRepBuilderAPI_Transform xform(origShape, *transformIter, Standard_True);
        rawShapes.push_back(xform.Shape());
    }
    return rawShapes;
};
```

**Key differences from the old lambda:**
- Returns `std::vector<TopoDS_Shape>` instead of `std::vector<TopoShape>` — raw OCCT, no element maps
- Does NOT include the support shape in the result — that's now handled separately at the call site
- Uses `BRepBuilderAPI_Transform` directly instead of `TopoShape::makeElementTransform`
- No `Data::indexSuffix` naming — these copies don't need individual names

### 3. Update `Mode::Features` case (fuse/cut calls)

**Remove** (lines 417-430):

```cpp
// REMOVE THIS:
if (!fuseShape.isNull()) {
    auto shapes = getTransformedCompShape(supportShape, fuseShape);
    if (OCCTProgressIndicator::getAppIndicator().UserBreak()) {
        return new App::DocumentObjectExecReturn("User aborted");
    }
    supportShape.makeElementFuse(shapes);
}
if (!cutShape.isNull()) {
    auto shapes = getTransformedCompShape(supportShape, cutShape);
    if (OCCTProgressIndicator::getAppIndicator().UserBreak()) {
        return new App::DocumentObjectExecReturn("User aborted");
    }
    supportShape.makeElementCut(shapes);
}
```

**Replace with:**

```cpp
if (!fuseShape.isNull()) {
    auto rawShapes = getRawTransformedShapes(fuseShape.getShape());
    if (OCCTProgressIndicator::getAppIndicator().UserBreak()) {
        return new App::DocumentObjectExecReturn("User aborted");
    }
    TopoDS_Shape preFused = preFuseShapes(rawShapes);
    Part::TopoShape preFusedTS(preFused);
    supportShape.makeElementFuse({supportShape, preFusedTS, fuseShape});
}
if (!cutShape.isNull()) {
    auto rawShapes = getRawTransformedShapes(cutShape.getShape());
    if (OCCTProgressIndicator::getAppIndicator().UserBreak()) {
        return new App::DocumentObjectExecReturn("User aborted");
    }
    TopoDS_Shape preFused = preFuseShapes(rawShapes);
    Part::TopoShape preFusedTS(preFused);
    // Combine original tool + pre-fused copies, then cut from support
    Part::TopoShape combinedTool;
    combinedTool.makeElementFuse({cutShape, preFusedTS});
    supportShape.makeElementCut({supportShape, combinedTool});
}
```

**Why the cut path is different:** For additive (fuse), we can just fuse `support + original + preFusedCopies` in one call. For subtractive (cut), we need to first combine the original tool with all its copies (fuse them together), then cut that combined tool from the support. This ensures the cut removes all the material from all copies.

### 4. Update `Mode::WholeShape` case

**Remove** (lines 433-440):

```cpp
// REMOVE THIS:
case Mode::WholeShape: {
    auto shapes = getTransformedCompShape(supportShape, supportShape);
    if (OCCTProgressIndicator::getAppIndicator().UserBreak()) {
        return new App::DocumentObjectExecReturn("User aborted");
    }
    supportShape.makeElementFuse(shapes);
    break;
}
```

**Replace with:**

```cpp
case Mode::WholeShape: {
    auto rawShapes = getRawTransformedShapes(supportShape.getShape());
    if (OCCTProgressIndicator::getAppIndicator().UserBreak()) {
        return new App::DocumentObjectExecReturn("User aborted");
    }
    TopoDS_Shape preFused = preFuseShapes(rawShapes);
    Part::TopoShape preFusedTS(preFused);
    supportShape.makeElementFuse({supportShape, preFusedTS});
    break;
}
```

## Expected Performance

| Occurrences | Before (element-mapped inputs) | After (element-mapped inputs) | Speedup factor |
|-------------|-------------------------------|------------------------------|----------------|
| 8           | 9                             | 2-3                          | ~3-4x          |
| 20          | 21                            | 2-3                          | ~7-10x         |
| 60          | 61                            | 2-3                          | ~20-30x        |

The actual speedup depends on the complexity of the tool shape (more faces/edges = more naming overhead saved per copy).

## Testing Checklist

1. **Additive polar pattern** (e.g., 8 bosses around an axis) — verify geometry matches old behavior
2. **Subtractive polar pattern** (e.g., 8 pockets around an axis) — verify cuts are correct
3. **Linear pattern** — same code path, verify both additive and subtractive
4. **WholeShape mode** — verify the entire support is correctly patterned
5. **High occurrence count** (20+) — measure execution time improvement
6. **Overlapping tool copies** — ensure pre-fuse handles overlapping geometry
7. **Through-all cuts** — verify subtractive features that cut entirely through the support
8. **Downstream face references** — check that features referencing pattern faces still resolve
9. **MultiTransform** — verify combined transforms still work (they call `execute()` on children differently)
10. **User abort** — verify progress cancellation still works during the pre-fuse
