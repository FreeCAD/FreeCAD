# Link Array External Geometry Import Bug - Development Notes

## Overview
When importing external geometry from a Draft Link Array element into a Sketch, the geometry was being imported from the **original object's location** instead of the **transformed array element's location**.

**Date Started**: 2026-01-14
**Status**: Fix implemented and tested

---

## Problem Description

### Steps to Reproduce
1. Create a sketch with geometry in Part workbench
2. Extrude it
3. Create a Draft Polar Array (Link) from the extrusion - this creates rotated copies
4. Create a new sketch
5. Try to import external geometry from one of the rotated array copies (not element 0)

### Expected Behavior
The geometry should be imported at the transformed location of the selected array element.

### Actual Behavior (Before Fix)
The geometry was imported from the **original object's location** (element 0 position).

---

## Root Cause Analysis

The bug had **two parts**:

### Part 1: GUI Handler Resolution
In `DrawSketchHandlerExternal.h`, the selection system was resolving the selection before passing it to `addExternal()`:

- **Original selection**: `("Array", "2.;#14:1;:G0;XTR;:H1130:8,F.Face6")` - with mapped TNP names
- **Resolved selection**: `("Extrude", "Face6")` - loses array element path!

The `SelectionChanges` message was being resolved through the Link Array to the base Extrude object.

### Part 2: addExternal() Path Handling
Even when receiving correct input like `("Array", "2.Face6")`, the `addExternal()` function in `SketchObject.cpp` was:

1. Resolving through the Array to get the base shape
2. Finding element indices in the resolved shape
3. Building element names **without** the array element path prefix

---

## Fix Implementation

The fix required changes in **three files**:

### 1. DrawSketchHandlerExternal.h (GUI)

**Added include:**
```cpp
#include <App/ElementNamingUtils.h>
```

**Modified `onSelectionChanged()`:**
```cpp
// Use original (non-resolved) selection if available.
// This is important for Link Arrays where msg may have resolved
// "Array" + "2.Face6" to "Extrude" + "Face6", losing the array element path.
// For mapped element names (TNP), convert to simple names using oldElementName()
// which transforms "2.;#14:...Face6" back to "2.Face6".
const char* objName = msg.pObjectName;
std::string subElemStr(msg.pSubName);
if (msg.pOriginalMsg) {
    objName = msg.pOriginalMsg->pObjectName;
    // Convert mapped element name to simple element name, preserving path prefix
    subElemStr = Data::oldElementName(msg.pOriginalMsg->pSubName);
}
Gui::cmdAppObjectArgs(
    sketchgui->getObject(),
    "addExternal(\"%s\",\"%s\", %s, %s)",
    objName,
    subElemStr.c_str(),
    ...
);
```

**Key insight:** `SelectionChanges::pOriginalMsg` contains the original (non-resolved) selection. `Data::oldElementName()` converts mapped TNP names like `"2.;#14:1;:G0;XTR;:H1130:8,F.Face6"` back to simple names `"2.Face6"`.

### 2. SketchObject.cpp - addExternal()

**Modified to preserve path prefix:**
```cpp
// For Link Arrays and similar, SubName may contain a path prefix like "2.Face6"
// where "2." identifies the array element and "Face6" is the actual geometry.
// We need to preserve this path prefix when storing the reference.
std::string subPath;
const char* elementName = Data::findElementName(SubName);
if (elementName && elementName != SubName) {
    // There's a path prefix before the element name (e.g., "2." in "2.Face6")
    subPath = std::string(SubName, elementName - SubName);
}

// Get the shape to search within. For arrays, we want the element's shape,
// not the whole resolved object, so we pass the subPath.
auto wholeShape = Part::Feature::getTopoShape(
    Obj,
    Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform,
    subPath.empty() ? nullptr : subPath.c_str()
);

// Later, when building element names:
std::string element = subPath + Part::TopoShape::shapeName(shapeType);  // "2.Face"
```

### 3. SketchObject.cpp - rebuildExternalGeometry()

**Same path-splitting logic for rebuild:**
```cpp
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
```

---

## Relationship to App::Link Fix

This bug is **separate** from the App::Link external geometry fix (issue #15663):

| Aspect | App::Link Fix | Link Array Fix |
|--------|---------------|----------------|
| Function | rebuildExternalGeometry() | addExternal() + GUI handler |
| Problem | App::Link not recognized as valid source | Array element path lost |
| Result | "not yet supported" error | Geometry at wrong location |
| GitHub Issue | #15663 | New issue needed |

Both fixes work together: the App::Link fix allows Link objects to be used as external geometry sources, and this fix ensures Link Array elements are referenced correctly.

---

## Test Results

### Now Working:
1. **Regular Part::Feature** - Edge/Face import ✅
2. **Simple App::Link** - Edge/Face import ✅
3. **Cross-document App::Link** - Edge/Face import ✅
4. **Regular Draft Array (non-link)** - Import from element N ✅
5. **Draft Link Array** - Import from element N ✅ (was broken, now fixed)

### Verified Debug Output:
```
addExternal: Obj=Array, SubName=2.Face6
addExternal: subPath='2.', elementName='Face6'
addExternal: Obj=Array, SubName=1.Face6
addExternal: subPath='1.', elementName='Face6'
addExternal: Obj=Array, SubName=0.Face6
addExternal: subPath='0.', elementName='Face6'
```

---

## Files Modified

```
src/Mod/Sketcher/Gui/DrawSketchHandlerExternal.h - Use pOriginalMsg + oldElementName()
src/Mod/Sketcher/App/SketchObject.cpp - addExternal() and rebuildExternalGeometry() path handling
```

---

## Technical Notes

### Why Data::oldElementName()?

FreeCAD uses Topological Naming Problem (TNP) mapped element names like:
```
2.;#14:1;:G0;XTR;:H1130:8,F.Face6
```

`Data::oldElementName()` converts these back to simple element names while preserving path prefixes:
- Input: `"2.;#14:1;:G0;XTR;:H1130:8,F.Face6"`
- Output: `"2.Face6"`

This allows our path-splitting code to work correctly.

### Why pOriginalMsg?

The selection system resolves selections through Links automatically. When you click on Array element 2:
- `msg` (resolved): object=Extrude, subname=Face6
- `msg.pOriginalMsg` (original): object=Array, subname=2.;#14:...Face6

We need the original to preserve the Array reference and element index.

### Why pass subPath to getTopoShape()?

`Part::Feature::getTopoShape(obj, options, subname)` with a subname like "2." retrieves the shape of array element 2 with its correct placement transformation applied. This ensures the geometry is imported at the correct location.

---

## Build & Test

```bash
# Build
export PATH="$HOME/.pixi/bin:$PATH"
pixi run build-debug

# Launch FreeCAD GUI for manual testing
./build/debug/bin/FreeCAD
```

---

## Next Steps

1. Create a new GitHub issue for this bug
2. Submit PR separately from the App::Link fix (issue #15663)
3. Run full Sketcher test suite

---

## Related Work

- **App::Link Fix**: `/Users/theosib/FreeCAD/NOTES-app-link-external-geometry.md` (issue #15663)
- **Part::Extrusion PropertyLinkSub**: `/Users/theosib/FreeCAD/NOTES-partial-sketch-extrusion.md` (issue #12820)
- **Part Loft Multi-Wire**: `/Users/theosib/FreeCAD/DRAFT-github-issue-part-loft-multiwire.md`
