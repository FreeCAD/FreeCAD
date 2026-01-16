# Draft GitHub Issue: Part::Loft Multi-Wire Profile Support

## Title
[Feature Request] Part: Support multi-wire profiles in Loft (like PartDesign::Loft)

## Labels
- Type: Feature Request
- Mod: Part
- Mod: PartDesign (refactor)

---

## Body

### Problem Description

Part::Loft cannot handle sketches or profiles with multiple closed regions (e.g., concentric circles to create hollow shapes). When attempting to loft such profiles, you get the error:

```
Profile shape is not a single vertex, edge, wire nor face.
```

In contrast, PartDesign::AdditiveLoft handles these cases correctly by lofting corresponding wires separately and sewing the results together.

**Example use case**: Creating a tapered hollow tube by lofting between two sketches, each containing two concentric circles.

### Current Behavior

Part::Loft's `prepareProfiles()` function in `TopoShapeExpansion.cpp` enforces single-wire profiles:

```cpp
if (shape.countSubShapes(TopAbs_WIRE) == 1) {
    ret.push_back(shape.getSubTopoShape(TopAbs_WIRE, 1));
    continue;
}
// ...
FC_THROWM(Base::CADKernelError,
          "Profile shape is not a single vertex, edge, wire nor face.");
```

### Expected Behavior

Part::Loft should accept profiles with multiple wires and produce hollow/complex lofted shapes, matching PartDesign::Loft's capability.

### Proposed Solution

I'd like to propose a **two-phase approach** that prioritizes code reuse and minimizes risk:

#### Phase 1: Extract Shared LoftHelper (Refactor PR)

Create a new shared utility `Part/App/LoftHelper.h|cpp` by extracting logic from PartDesign::Loft:

1. **Extract `getSectionShape()`** → `LoftHelper::extractProfileWires()`
   - Extracts all wires/vertices from a shape
   - Handles compounds, loose edges, face splitting
   - Currently in `FeatureLoft.cpp` lines 66-152

2. **Extract wire correspondence building**
   - Builds `wiresections[wireIndex][sectionIndex]` array
   - Currently in `FeatureLoft.cpp` lines 187-207

3. **Refactor PartDesign::Loft** to call the new helper
   - Zero functional change
   - All existing PartDesign Loft tests must pass

This follows the established pattern of `ExtrusionHelper.h|cpp` which is already shared between Part and PartDesign (e.g., `Part::ExtrusionHelper::makeElementDraft()` is called by `PartDesign::FeatureExtrude`).

#### Phase 2: Enhance Part::Loft (Feature PR)

1. Change `Sections` property: `PropertyLinkList` → `PropertyLinkSubList`
   - Enables sub-element selection (specific faces/wires from sketches)
   - Maintains backward compatibility (empty subs = whole object)

2. Update `Part::Loft::execute()` to use `LoftHelper`:
   - Extract wires from each profile using shared helper
   - Build wire correspondence table
   - Loft corresponding wires separately via existing `makeElementLoft()`
   - Sew shells together for solid output

3. Update Part Loft GUI (`TaskLoft.cpp`) to allow complex profile selection

### Why Two Phases?

1. **Reduced risk**: Phase 1 is pure refactoring with no functional changes
2. **Easier review**: Smaller, focused PRs are easier to review and test
3. **Validates shared code**: The helper is battle-tested via PartDesign before Part uses it
4. **Follows FreeCAD patterns**: Matches how `ExtrusionHelper`, `FaceMaker*`, and `Tools` are shared

### Technical Details

**Files to create (Phase 1):**
- `src/Mod/Part/App/LoftHelper.h`
- `src/Mod/Part/App/LoftHelper.cpp`

**Files to modify (Phase 1):**
- `src/Mod/Part/App/CMakeLists.txt` - Add new files
- `src/Mod/PartDesign/App/FeatureLoft.cpp` - Use LoftHelper

**Files to modify (Phase 2):**
- `src/Mod/Part/App/PartFeatures.h` - PropertyLinkSubList
- `src/Mod/Part/App/PartFeatures.cpp` - Multi-wire execute()
- `src/Mod/Part/Gui/TaskLoft.cpp` - UI updates
- `src/Mod/Part/App/TopoShapeExpansion.cpp` - Relax prepareProfiles() or bypass it

### Existing Code Analysis

Both Part::Loft and PartDesign::Loft already use the same underlying `TopoShape::makeElementLoft()`. The key difference is preprocessing:

| Aspect | Part::Loft | PartDesign::Loft |
|--------|-----------|-----------------|
| Property type | `PropertyLinkList` | `PropertyLinkSubList` |
| Wire extraction | Single wire enforced | All wires extracted |
| Correspondence | N/A | Per-wire indexed |
| Loft calls | Single | One per wire, then sew |

### Related Issues

- #6130 - PartDesign Loft wire ordering (related but different issue)
- #6010 - General loft improvements
- #12820 - Partial sketch region selection (related goal)

### Questions for Maintainers

1. Does this two-phase approach seem reasonable?
2. Is `Part/App/LoftHelper.h|cpp` the right location for the shared code?
3. Any concerns about changing `Sections` from `PropertyLinkList` to `PropertyLinkSubList`?
4. Should the GUI changes be a separate third PR?

I'm happy to work on this implementation if the approach looks good. I'd start with Phase 1 (the refactor) to establish the shared infrastructure.

### Environment

```
OS: macOS
FreeCAD version: 1.0dev (main branch)
```
