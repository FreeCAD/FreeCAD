# Part::Extrusion Partial Sketch Selection - Development Notes

## Overview
Adding support for selecting specific faces/regions from a sketch when using Part::Extrusion, similar to how Part Design's Pad works.

**Related Issue**: https://github.com/FreeCAD/FreeCAD/issues/12820

**Date Started**: 2026-01-14

---

## Changes Made

### 1. FeatureExtrusion.h
**File**: `src/Mod/Part/App/FeatureExtrusion.h`

Changed:
```cpp
// Line 48: Changed from PropertyLink to PropertyLinkSub
App::PropertyLinkSub Base;  // Was: App::PropertyLink Base;

// Line 110: Updated method signature
static Base::Vector3d calculateShapeNormal(const App::PropertyLinkSub& shapeLink);
// Was: static Base::Vector3d calculateShapeNormal(const App::PropertyLink& shapeLink);
```

### 2. FeatureExtrusion.cpp
**File**: `src/Mod/Part/App/FeatureExtrusion.cpp`

Changed:
- Line 280: Updated `calculateShapeNormal` signature to match header
- Lines 391-440: Rewrote `execute()` method to handle sub-element selection:

```cpp
App::DocumentObjectExecReturn* Extrusion::execute()
{
    App::DocumentObject* link = Base.getValue();
    if (!link) {
        return new App::DocumentObjectExecReturn("No object linked");
    }

    try {
        ExtrusionParameters params = computeFinalParameters();
        TopoShape result(0, getDocument()->getStringHasher());

        // Get the shape to extrude, considering sub-element selection
        TopoShape sourceShape;
        const auto& subs = Base.getSubValues();
        if (subs.empty()) {
            // No sub-elements specified - use entire shape (backward compatible)
            sourceShape = Feature::getTopoShape(link, ShapeOption::ResolveLink | ShapeOption::Transform);
        }
        else {
            // Sub-elements specified - extract and combine them
            std::vector<TopoShape> shapes;
            for (const auto& sub : subs) {
                auto subshape = Feature::getTopoShape(
                    link,
                    ShapeOption::NeedSubElement | ShapeOption::ResolveLink | ShapeOption::Transform,
                    sub.c_str()
                );
                if (subshape.isNull()) {
                    return new App::DocumentObjectExecReturn(
                        (std::string("Sub-shape not found: ") + sub).c_str()
                    );
                }
                shapes.push_back(subshape);
            }
            if (shapes.size() == 1) {
                sourceShape = shapes[0];
            }
            else {
                sourceShape.makeElementCompound(shapes);
            }
        }

        extrudeShape(result, sourceShape, params);
        this->Shape.setValue(result);
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}
```

### 3. DlgExtrusion.cpp
**File**: `src/Mod/Part/Gui/DlgExtrusion.cpp`

Changed two locations where temporary PropertyLink was created:
- Line 356: `App::PropertyLinkSub lnk;` (was `App::PropertyLink lnk;`)
- Line 759: `App::PropertyLinkSub lnk;` (was `App::PropertyLink lnk;`)

---

## Test Results

### Passing Tests
1. **All 127 existing Part tests pass** - No regressions
2. **Backward compatibility** - `extrude.Base = sketch` works
3. **Wire extrusion** - Standard use case works
4. **Sketch extrusion** - Standard use case works
5. **Face selection from solids** - `extrude.Base = (box, ['Face1'])` works

### Known Limitation
**MakeInternals faces not accessible**:
- Faces from `MakeInternals` are stored in `sketch.InternalShape`, not `sketch.Shape`
- `Feature::getTopoShape()` doesn't include internal elements
- Selecting `(sketch, ['Face1'])` fails if Face1 is only in InternalShape

This is a **pre-existing gap** - MakeInternals infrastructure exists in Sketcher but Part tools don't consume it.

---

## Test Scripts Created
- `/Users/theosib/FreeCAD/test_extrusion_sublink.py` - Comprehensive test suite
- `/Users/theosib/FreeCAD/simple_test.py` - Basic tests
- `/Users/theosib/FreeCAD/real_test.py` - Realistic use cases (PASSED)
- `/Users/theosib/FreeCAD/partial_sketch_test.py` - MakeInternals test (shows limitation)
- `/Users/theosib/FreeCAD/explore_internals.py` - Debug script for InternalShape
- `/Users/theosib/FreeCAD/debug_test.py` - Debug script

---

## Build & Test Commands
```bash
# Build
export PATH="$HOME/.pixi/bin:$PATH"
pixi run build-debug

# Run all Part tests
./build/debug/bin/FreeCADCmd -t "TestPartApp"

# Run a specific test script
./build/debug/bin/FreeCADCmd real_test.py
```

---

## Questions for Maintainers
1. Is there an existing pattern for accessing a sketch's internal faces from Part workbench?
2. Should `getTopoShape()` be extended to include internal elements?
3. Should Part::Extrusion handle sketches specially?

---

## Draft GitHub Comment
Posted to issue #12820:

```
I've been experimenting with adding partial sketch region support to Part::Extrusion. Here's what I tried and found:

**The change**: Convert `PropertyLink Base` to `PropertyLinkSub Base` in Part::Extrusion, similar to how Part Design's Pad uses `PropertyLinkSub Profile`.

**What works**:
- Backward compatibility is preserved - `extrude.Base = sketch` still works
- Face selection from any Part::Feature works: `extrude.Base = (box, ['Face1'])`
- All 127 existing Part tests pass

**What doesn't work yet**:
The faces created by `MakeInternals` are stored in the sketch's `InternalShape` property, not the main `Shape`. The `Feature::getTopoShape()` function doesn't include these internal elements, so selecting them (e.g., `extrude.Base = (sketch, ['Face1'])`) fails to find the sub-shape.

**Question for maintainers**: Is there an existing pattern for accessing a sketch's internal faces from Part workbench? Should `getTopoShape()` be extended to handle this, or should Part::Extrusion handle sketches specially?

Happy to submit a PR for the basic PropertyLinkSub change if there's interest, even if full MakeInternals support needs separate work.
```

---

## Next Steps (Pending Feedback)
1. Wait for maintainer feedback on GitHub issue
2. If positive, consider:
   - Submitting PR for basic PropertyLinkSub change
   - Separate PR/issue for InternalShape integration
3. May need to investigate how Part Design accesses sketch faces

---

## Reference: How Part Design Does It
Part Design's `ProfileBased::getTopoShapeVerifiedFace()` in `src/Mod/PartDesign/App/FeatureSketchBased.cpp:201-367`:
- Uses `PropertyLinkSub Profile`
- When subs is non-empty, extracts sub-shapes individually
- Combines with `makeElementCompound()`
- Creates faces with `makeElementFace()` using FaceMakerBullseye

Key difference: Part Design may be getting the full shape including internals through a different code path. Worth investigating if maintainers suggest this approach.

---

## Files Modified (Summary)
```
src/Mod/Part/App/FeatureExtrusion.h    - PropertyLinkSub, method signature
src/Mod/Part/App/FeatureExtrusion.cpp  - execute() rewritten, signature update
src/Mod/Part/Gui/DlgExtrusion.cpp      - Two PropertyLinkSub temporaries
```

## Files Created (Test Scripts - Don't Commit)
```
test_extrusion_sublink.py
simple_test.py
real_test.py
partial_sketch_test.py
explore_internals.py
debug_test.py
freecad-part-modernization.md (user's planning doc)
NOTES-partial-sketch-extrusion.md (this file)
```
