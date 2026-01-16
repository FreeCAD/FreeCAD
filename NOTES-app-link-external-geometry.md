# App::Link External Geometry Support - Development Notes

## Overview
Adding support for importing external geometry from App::Link objects into Sketcher, fixing issue where linked sketches/features couldn't be used as external geometry references.

**Related Issue**: https://github.com/FreeCAD/FreeCAD/issues/15663

**Date Started**: 2026-01-14

---

## Problem Description

When trying to import external geometry from an App::Link object into a sketch, FreeCAD would fail with the error:

```
Sketcher: Datum feature type is not yet supported as external geometry for a sketch
```

This affected both:
1. Same-document App::Link objects pointing to Part features
2. Cross-document App::Link (XLink) objects pointing to Part features in other documents

### Root Cause

In `SketchObject.cpp`, the `rebuildExternalGeometry()` function has a chain of type checks:
1. `Part::Datum` - handled specially
2. `Part::Feature` - gets the shape directly
3. `App::Plane` - handled specially
4. Everything else - returns "not yet supported" error

App::Link objects are not derived from `Part::Feature`, so they fell through to the "not yet supported" case, even though they reference valid Part features.

---

## Changes Made

### 1. SketchObject.cpp
**File**: `src/Mod/Sketcher/App/SketchObject.cpp`

#### Added Include
```cpp
#include <App/Link.h>
```

#### Modified rebuildExternalGeometry() (around line 9292)

**Before:**
```cpp
else if (Obj->isDerivedFrom<Part::Feature>()) {
    refSubShape = static_cast<const Part::Feature*>(Obj)->Shape.getShape().getSubShape(SubElement.c_str());
}
```

**After:**
```cpp
else if (Obj->isDerivedFrom<Part::Feature>()
         || Obj->hasExtension(App::LinkBaseExtension::getExtensionClassTypeId())) {
    // Use getTopoShape with ResolveLink to handle both Part::Feature and App::Link.
    // For Link Arrays, SubElement may contain path like "0.Edge1" where "0." is
    // the array element index and "Edge1" is the actual geometry element.
    // We need to split these: pass the path part to getTopoShape for correct
    // placement resolution, then extract the element with getSubShape.
    std::string subPath;
    const char* elementName = nullptr;
    if (!SubElement.empty()) {
        elementName = Data::findElementName(SubElement.c_str());
        if (elementName && elementName != SubElement.c_str()) {
            // There's a path before the element name (e.g., "0." in "0.Edge1")
            subPath = std::string(SubElement.c_str(), elementName - SubElement.c_str());
        }
    }
    Part::TopoShape refShape = Part::Feature::getTopoShape(
        Obj,
        Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform,
        subPath.empty() ? nullptr : subPath.c_str()
    );
    if (!refShape.isNull()) {
        if (elementName && *elementName) {
            refSubShape = refShape.getSubShape(elementName);
        }
        else if (!SubElement.empty()) {
            refSubShape = refShape.getSubShape(SubElement.c_str());
        }
        else {
            refSubShape = refShape.getShape();
        }
    }
}
```

#### Key Changes Explained:

1. **Added Link check**: `Obj->hasExtension(App::LinkBaseExtension::getExtensionClassTypeId())` detects App::Link objects

2. **Use getTopoShape()**: Instead of directly casting to `Part::Feature` and accessing `Shape`, we use the utility function `Part::Feature::getTopoShape()` which can resolve through links

3. **ResolveLink option**: The `Part::ShapeOption::ResolveLink` flag tells `getTopoShape()` to follow App::Link references to get the actual shape

4. **Transform option**: `Part::ShapeOption::Transform` applies the object's placement transformation

5. **Path splitting with Data::findElementName()**: SubElement may contain a path prefix (e.g., "0.Edge1" for array element 0's Edge1). We split this into:
   - `subPath`: The path prefix ("0.") - passed to getTopoShape for correct placement
   - `elementName`: The actual element name ("Edge1") - used with getSubShape

6. **Handle empty SubElement**: The original code assumed SubElement was always populated. The fix handles both cases - getting the full shape or a sub-element.

---

## Test Results

### Passing Tests
1. **Same-document Link**:
   - Created a sketch with a square in Part workbench
   - Extruded it
   - Created App::Link to the sketch
   - In a new sketch, successfully imported external geometry from the link
   - ✅ PASS

2. **Cross-document Link (XLink)**:
   - Document1: Created a sketch with geometry
   - Document2: Created App::Link referencing Document1's sketch
   - In Document2, created a new sketch and imported external geometry from the link
   - ✅ PASS

3. **Regular Draft Array (non-link)**:
   - Created a sketch, extruded it
   - Created a Draft Polar Array (non-link mode)
   - Imported external geometry from array element 2
   - ✅ PASS - Geometry imports at correct transformed location

4. **Backward Compatibility**:
   - Importing external geometry from regular Part::Feature objects still works
   - ✅ PASS

---

## Known Limitation: Link Arrays

**NOTE**: This fix handles `rebuildExternalGeometry()` which reconstructs external geometry references when a document is loaded or recomputed. However, there is a **separate bug** in `addExternal()` that affects **Draft Link Arrays** (Draft arrays with "Use Link" enabled).

When adding external geometry from a Link Array element:
- The selection system correctly reports the Array object with subname "2.Face6"
- But `addExternal()` resolves through the link and stores a reference to the base object ("Extrude") with just "Face6"
- This causes geometry to be imported from the original location instead of the transformed array element

This is documented separately in `NOTES-link-array-external-geometry.md` and will be addressed in a separate PR.

---

## Build & Test Commands

```bash
# Build
export PATH="$HOME/.pixi/bin:$PATH"
pixi run build-debug

# Launch FreeCAD GUI for manual testing
./build/debug/bin/FreeCAD

# Run Sketcher tests (if any specific ones exist)
./build/debug/bin/FreeCADCmd -t "TestSketcherApp"
```

---

## Manual Test Procedure

### Test 1: Same-Document Link
1. Open FreeCAD
2. Create new document
3. Switch to Part workbench
4. Create a sketch with a closed shape (e.g., rectangle)
5. Close sketch and extrude it
6. Select the sketch in the model tree
7. Click the "Make Link" button (chain link icon) to create App::Link
8. Create a new sketch on a plane
9. Click "External Geometry" tool
10. Click on an edge of the linked sketch
11. **Expected**: Edge should be imported as external geometry (purple dashed line)

### Test 2: Cross-Document Link
1. Open FreeCAD
2. Create Document1, add a sketch with geometry, save it
3. Create Document2, save it
4. Enable View → TreeView actions → Multi document
5. In Document1's tree, select the sketch
6. Click on Document2's 3D view tab to make it active
7. Click "Make Link" - this creates a cross-document link
8. In Document2, create a new sketch
9. Use "External Geometry" to import from the linked sketch
10. **Expected**: Geometry should be imported successfully

### Test 3: Regular Draft Array
1. Create a sketch with geometry in Part workbench
2. Extrude it
3. Create a Draft Polar Array (NOT link mode) with multiple elements
4. Create a new sketch
5. Import external geometry from element 2 (not element 0)
6. **Expected**: Geometry imports at the rotated element's location, not the original

### Additional Tests (Suggested by Community)

The following tests were suggested to verify placement handling:

#### Test 4: Link with Non-Identity Placement
1. Create a sketch with geometry
2. Create an App::Link to the sketch
3. **Change the Link's Placement** (move/rotate it)
4. Create a new sketch and import external geometry from the link
5. **Expected**: Geometry imports at the Link's transformed location

#### Test 5: Original Object with Non-Identity Placement
1. Create a sketch with geometry
2. **Change the sketch's Placement** (move it away from origin)
3. Create an App::Link to the sketch
4. Create a new sketch and import external geometry from the link
5. **Expected**: Geometry imports at the correct combined placement

#### Test 6: Link to Part Containing Sketch
1. Create a Part container
2. Create a sketch inside the Part
3. Create an App::Link to the Part (not directly to the sketch)
4. Create a new sketch and import external geometry from the linked Part's sketch
5. **Expected**: Geometry imports correctly

#### Test 7: Placement Changes After Import
1. Create a sketch, create a link, import external geometry
2. **Change the link's placement** after import
3. Recompute
4. **Expected**: External geometry updates to the new position

---

## Files Modified (Summary)

```
src/Mod/Sketcher/App/SketchObject.cpp  - Added Link.h include, modified rebuildExternalGeometry()
```

---

## Next Steps (Pending Feedback)

1. Wait for maintainer feedback on GitHub issue #15663
2. If positive:
   - Create a proper branch for the fix
   - Run full Sketcher test suite
   - Submit PR with proper commit message
3. Address Link Array bug separately (see NOTES-link-array-external-geometry.md)

---

## Related Work

- **Part::Extrusion PropertyLinkSub**: `/Users/theosib/FreeCAD/NOTES-partial-sketch-extrusion.md`
- **Link Array Bug**: `/Users/theosib/FreeCAD/NOTES-link-array-external-geometry.md`
- **Part Loft Multi-Wire**: `/Users/theosib/FreeCAD/DRAFT-github-issue-part-loft-multiwire.md`

---

## Technical Notes

### Why hasExtension() instead of isDerivedFrom()?

App::Link uses the Extension system rather than class inheritance. An App::Link object:
- Is derived from `App::DocumentObject`
- Has `App::LinkBaseExtension` attached to it
- Is NOT derived from `Part::Feature`

So we check for the extension rather than the class type.

### Why getTopoShape() instead of direct Shape access?

`Part::Feature::getTopoShape()` is a utility function that:
1. Handles various object types uniformly
2. Can resolve through App::Link references with `ResolveLink` option
3. Applies transformations correctly with `Transform` option
4. Accepts a `subname` parameter for path-based shape retrieval
5. Returns a proper TopoShape that can be queried for sub-elements

Direct access via `static_cast<Part::Feature*>(Obj)->Shape` would fail for App::Link objects since they're not Part::Feature instances.

### Why Data::findElementName()?

`Data::findElementName()` is a utility that parses FreeCAD subnames to find where the actual element name starts. For example:
- Input: `"0.Edge1"` → Returns pointer to `"Edge1"`
- Input: `"Edge1"` → Returns pointer to `"Edge1"` (same as input)
- Input: `"Body.Sketch.Edge1"` → Returns pointer to `"Edge1"`

This allows us to correctly split compound paths from element names.
