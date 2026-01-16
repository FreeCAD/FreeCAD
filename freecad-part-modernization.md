# FreeCAD Part Workbench Modernization Project

## Project Philosophy

This project aims to improve the **Part workbench** within its own paradigm—not to make it converge with Part Design. The goal is "feature completeness and usability improvements within the Part paradigm."

Part and Part Design represent fundamentally different modeling philosophies:

| Aspect | Part (CSG) | Part Design |
|--------|------------|-------------|
| Core model | Boolean graph of independent solids | Single Body with linear feature history |
| Multi-body | First-class, central to workflow | Discouraged, requires multiple Bodies |
| Operations | Object-level, composable | Feature-chain-level, cumulative |
| Flexibility | Reorganize anytime | Order matters, history-dependent |

**We are enhancing Part, not replacing it.**

---

## Guiding Principles

These constraints should guide all contributions:

### 1. Multi-body stays first-class
- No single-solid enforcement
- No Body container requirement
- Multiple independent solids are the norm, not an exception

### 2. Operations stay object-level
- Tools produce objects that can be freely combined
- No linear feature history assumptions
- Boolean graphs, not feature trees

### 3. Sketches are first-class citizens
- Attach sketches to faces freely
- Reuse sketches across multiple operations
- Union/cut results later as needed

### 4. Booleans are central, not incidental
- Editing boolean graphs should be easy and reversible
- Operand reordering, reassignment, extraction should be fluid
- Cut/Fuse/Common/Compound should have consistent editing UX

### 5. No forced workflows
- Avoid anything that pushes users toward Part Design patterns
- Keep Part's flexibility as a feature, not a limitation

---

## Priority Feature Wishlist

### Priority 1: Editable Boolean & Compound Membership

**Impact:** Very High  
**Part-spirit alignment:** ✓✓✓ (Core Part identity)

#### Current Pain Points
- Can drag objects into Compound/Union
- Cannot drag objects *out*
- Cannot reorder operands easily
- Cannot reassign operands between Cut/Fuse/Common
- Cut and Common feel second-class vs Union/Compound

#### Desired Enhancements

**A. Full drag-and-drop operand editing**
- Drag objects into a boolean
- Drag objects out of a boolean
- Drag objects between booleans
- Works for: Compound, Fuse, Cut, Common

**B. Operand ordering UI**
- Explicit operand list in property editor
- Up/Down buttons or drag-reorder
- Critical for Cut (Base vs Tool ordering) and multi-tool cuts

**C. Boolean "slot" semantics**
- For Cut: Clearly separate Base and Tools
- For Common: Allow N-way intersections with reorderability

**D. Non-destructive graph editing**
- Reordering operands should not invalidate downstream objects
- Avoid forced recompute cascades

#### Implementation Notes
- Look at `src/Mod/Part/App/FeaturePartBoolean.cpp`
- Compound handling in `src/Mod/Part/App/FeaturePartCompound.cpp`
- GUI operand editing likely in `src/Mod/Part/Gui/`
- Consider unified operand list widget reusable across Fuse/Cut/Common/Compound

---

### Priority 2: Hole Feature for Part

**Impact:** High  
**Part-spirit alignment:** ✓✓ (Smart primitive for CSG subtraction)

#### Current State
- Part Design has sophisticated Hole wizard (thread types, countersink/counterbore, depth modes)
- Part has nothing—must sketch circle + extrude + boolean manually
- Fasteners workbench integration is awkward

#### Desired Behavior

A Part-style Hole should:
- **Input:** Face selection + placement OR sketch with circles
- **Output:** Either a cuttable solid OR perform the cut directly (user choice)
- **Work on any solid**, not just active Body
- **Parameters:**
  - Hole type: Simple, Counterbore, Countersink
  - Threading: None, Metric (M series), UNC, UNF, etc.
  - Thread representation: None, Symbolic, Modeled
  - Depth: Through All, To Depth, To Face
  - Fit: Close, Normal, Loose (affects diameter)

#### Implementation Notes
- Part Design Hole: `src/Mod/PartDesign/App/FeatureHole.cpp`
- Thread data in JSON files: `src/Mod/PartDesign/App/Resources/Hole/`
- Could potentially share thread database with Part Design
- Key difference: output is standalone solid (for boolean) or direct cut, not Body feature

---

### Priority 3: Partial Sketch Region Selection for Extrude

**Impact:** High  
**Part-spirit alignment:** ✓✓ (Better tool, not workflow change)

#### Current State
- Part Extrude uses all closed regions in a sketch
- Part Design Pad can select specific faces/regions
- GitHub Issue #19094 tracks this request

#### Desired Behavior

**A. Region selection for Part::Extrude**
- Choose: all closed regions (current) OR selected regions only
- UI similar to face selection, not sketch constraint editing

**B. Persistent region IDs**
- Sketch sub-regions tracked parametrically
- Intelligent updates when sketch changes

**C. Multi-output extrude (optional, advanced)**
- One sketch → multiple solids (one per region)
- Very useful for later boolean work

#### Implementation Notes
- Part Extrude: `src/Mod/Part/App/FeatureExtrusion.cpp`
- Part Design Pad region handling: `src/Mod/PartDesign/App/FeaturePad.cpp`
- May need sketch face extraction utilities

---

### Priority 4: Hollow Loft / Sweep / Extrude

**Impact:** Medium-High  
**Part-spirit alignment:** ✓✓ (Feature completeness, not workflow)

#### Current State
- Part Loft creates solid or shell from profiles
- To make hollow lofted tube: must create two lofts and boolean subtract
- Part Design can do Additive Loft → Subtractive Loft within Body

#### Desired Behavior

**Loft enhancements:**
- Loft between multiple wires per section (outer + inner profiles)
- Wall thickness parameter option
- Open loft → shell, closed loft → hollow solid

**Extrude enhancements:**
- Extrude face/sketch as thin solid (wall thickness parameter)
- Mid-plane / symmetric thickness option

**Sweep enhancements:**
- Hollow sweep (profile with inner/outer wires, or thickness param)
- Variable thickness along path (advanced, optional)

#### Implementation Notes
- Part Loft: `src/Mod/Part/App/FeatureLoft.cpp` (uses `BRepOffsetAPI_ThruSections`)
- Part Sweep: `src/Mod/Part/App/FeatureSweep.cpp`
- Thickness/offset operations: `BRepOffsetAPI_MakeThickSolid`
- May need to generate inner profile via offset, then loft both

---

### Priority 5: Compound Improvements

**Impact:** Medium  
**Part-spirit alignment:** ✓✓ (Reinforces multi-body nature)

#### Current State
- Compounds exist but are underpowered
- Less editable than Booleans
- Visibility handling is basic

#### Desired Behavior

**A. Editable compound membership**
- Same drag/reorder rules as booleans (from Priority 1)

**B. Compound visibility modes**
- Show whole compound
- Show children only
- Hybrid: ghosted container + visible children

**C. Compounds as boolean inputs**
- Treat compound as operand list when used in Cut/Fuse/Common
- Option to drill down into compound members

#### Implementation Notes
- Compound feature: `src/Mod/Part/App/FeaturePartCompound.cpp`
- Visibility likely in `src/Mod/Part/Gui/ViewProviderCompound.cpp`

---

### Priority 6: Dress-Up Tool Improvements

**Impact:** Medium  
**Part-spirit alignment:** ✓ (Quality improvement to existing tools)

#### Current State
- Part Fillet/Chamfer less robust than Part Design equivalents
- Poor handling of topological naming changes
- No multi-radius fillet support

#### Desired Behavior

**A. Multi-radius and rule-based fillets**
- Different radii per edge set
- Edge selection groups stored parametrically

**B. Stable edge referencing**
- Geometric inference (edge adjacency, angle)
- Optional naming hints
- Reduce topological naming breakage

**C. Draft tool parity**
- Draft by face set
- Draft to neutral plane
- Parametric angle references

#### Implementation Notes
- Part Fillet: `src/Mod/Part/App/FeaturePartFillet.cpp`
- Part Chamfer: `src/Mod/Part/App/FeaturePartChamfer.cpp`
- Part Design equivalents may have better edge selection code to reference

---

## Features Explicitly NOT in Scope

These would push Part toward Part Design's paradigm:

- **Body containers** - Part doesn't need them
- **Feature history/tip tracking** - Part operations are independent
- **Single-solid enforcement** - Multi-body is Part's strength
- **Pad/Pocket semantics** - We're enhancing Extrude, not replacing it
- **Feature suppression** - Makes sense for history, not CSG graphs

---

## Diagnostic/UX Improvements (Low Priority, But Welcome)

These don't change modeling but help users:

- **Boolean failure visualization:** Highlight problem faces, show non-manifold regions
- **Loft preview improvements:** Show twist, section correspondence
- **Dress-up failure explanations:** Which edge failed and why

---

## Existing Issues to Reference

- [#19094](https://github.com/FreeCAD/FreeCAD/issues/19094) - Part Extrude partial sketch support
- Part 3D Offset is buggy (separate bug-fix effort)

---

## Source Code Entry Points

```
src/Mod/Part/
├── App/
│   ├── FeatureExtrusion.cpp      # Part Extrude
│   ├── FeatureLoft.cpp           # Part Loft (BRepOffsetAPI_ThruSections)
│   ├── FeatureSweep.cpp          # Part Sweep
│   ├── FeaturePartBoolean.cpp    # Cut/Fuse/Common base
│   ├── FeaturePartFuse.cpp       # Union
│   ├── FeaturePartCut.cpp        # Subtraction
│   ├── FeaturePartCommon.cpp     # Intersection
│   ├── FeaturePartCompound.cpp   # Compound
│   ├── FeaturePartFillet.cpp     # Fillet
│   └── FeaturePartChamfer.cpp    # Chamfer
└── Gui/
    ├── TaskExtrusion.cpp         # Extrude dialog
    ├── TaskLoft.cpp              # Loft dialog
    ├── TaskSweep.cpp             # Sweep dialog
    └── ViewProvider*.cpp         # Tree/3D view handling

src/Mod/PartDesign/
├── App/
│   ├── FeatureHole.cpp           # Reference for Hole feature
│   ├── FeaturePad.cpp            # Reference for region selection
│   └── Resources/Hole/           # Thread database JSON files
```

---

## Development Approach

1. **Start small:** Pick one feature, implement minimally, test thoroughly
2. **Match existing patterns:** Follow FreeCAD's coding conventions
3. **Engage forum early:** Post WIP for feedback before large PRs
4. **Atomic PRs:** One feature per pull request
5. **Document changes:** Update wiki if adding user-facing features

---

## Success Criteria

A successful contribution:
- Works within Part's multi-body, CSG-centric paradigm
- Doesn't require Body containers or feature history
- Integrates naturally with existing Part tools
- Is upstreamable (follows FreeCAD conventions, has tests)
- Makes Part users' lives easier without pushing them to Part Design




Bugs:
- Can't import geometry from draft array duplicates
- Can't import geometry from links
- Can't select sketch to extrude after clicking the Extrude icon
