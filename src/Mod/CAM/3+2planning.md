# 3+2 Axis Milling Implementation

## Overview

3+2 axis milling support is implemented through a machine-agnostic orientation solver that derives all behavior from the Machine data model. The new system replaces the hardcoded C-A rotation generator with a flexible solver supporting arbitrary kinematic configurations.

## Current Implementation Status

### ✅ Completed Components

1. **Machine Data Model** (`Machine/models/machine.py`)
   - Complete `RotaryAxis` configuration with rotation vectors, limits, preferences
   - Kinematic chain support via parent/sequence relationships
   - Axis roles (`TABLE_ROTARY`, `HEAD_ROTARY`) and solution preferences

2. **Orientation Solver** (`Path/Base/Generator/rotation.py`)
   - `solve_orientation(machine, desired_tool_axis, current_state) → SolveResult`
   - **Robust dual-decomposition**: tries both axis orders to handle singularities
   - **Role-aware validation**: forward kinematics check for table/head/mixed machines
   - **Head-rotary support**: dedicated decomposition solving R.multVec(Z) = desired
   - Single-axis and multi-axis support
   - Solution space expansion (360° offsets, flips)
   - Cost-based selection with current state awareness
   - **Public APIs**: `compute_rotation_matrix()`, `build_kinematic_chain()`
   - **Comprehensive test suite**: `CAMTests/TestPathRotationGenerator.py` (13 test cases)

3. **Operation Integration** (`Path/Op/Base.py`)
   - **`Workplane` property**: `App::PropertyVector` with Z-up default
   - **Multi-axis positioning**: rotation commands emitted before toolpath
   - **Backplot support**: operation Placement set for correct 3D visualization
   - **Backward compatibility**: property added in `onDocumentRestored`

4. **GUI Support** (`Path/Op/Gui/Base.py`)
   - `Part::AttachExtension` on all operations (face attachment)
   - Face selection observer extracts face normals
   - Workplane coordinate system visualization (coin3d)
   - **Fixed**: Workplane property used instead of AttachmentSupport

## Phase 3: Operation Integration

### Implementation Summary

The multi-axis positioning system is partially integrated into the base operation class. Rotation commands are added by the base class in execute() method.

Individual operations must be modified to use transformed geometry in calculating toolpath.  The framework for this is done but must be completed in the idividaal operation classes.

## Operation Migration Status

### ✅ Completed Operations
|operation| Status | Notes |
|---|---|---|
|Deburr| ✅ DONE | Uses `self.baseShapes(obj)` for 3+2 geometry transformation. Depth transformation working correctly. |
|MillFacing| ✅ DONE | Stock-only operation - geometry transformation via stock proxy. Depth transformation skipped (uses stock-relative depths). |
|Adaptive| ✅ DONE | Special case - depth transformation skipped, geometry transformation via baseShapes() |
|Helix| ✅ DONE | Inherits from CircularHoleBase - fixed circular hole detection for transformed geometry with multiple edges |
|Custom| ✅ DONE | No geometry processing - automatically works with 3+2 transformation |
|Drilling| ✅ DONE | Inherits from CircularHoleBase - already uses `self.baseShapes(obj)` |
|Engrave| ✅ DONE | Already uses `self.baseShapes(obj)` and `self.model` - ready for 3+2 milling |
|Vcarve| ✅ DONE | Refactored for consistency and 3+2 milling support |
|PocketShape| ✅ DONE | Already uses `self.baseShapes(obj)` and `self.model`/`self.stock` (wrapped by execute()). Depth transformation skipped. |
|Pocket| ✅ DONE | Already uses `self.baseShapes(obj)` and `self.model`. Depth transformation skipped (ObjectPocket). |
|Profile| ✅ DONE | Has `execute()` override. Uses `self.baseShapes(obj)` and `self.model`. Depth transformation skipped. |


#### Direct Base Operations (PathOp.ObjectOp)
|operation| Status | Notes |
|---|---|---|
|Probe| ✅ DONE | Stock-only operation - uses transformed stock automatically. Depth transformation skipped (probing uses world coordinates). |

#### Operations requiring refactor/reimplementation (PathOp.ObjectOp)
|operation| Status | Notes |
|---|---|---|
|Waterline| Won't do.  Reimplement | Direct PathOp.ObjectOp - needs migration to `self.baseShapes(obj)` |
|Surface| Won't do.  Reimplement | Direct PathOp.ObjectOp - uses transformed job models automatically, depth transformation skipped |
|Slot| ❌ TODO | Direct PathOp.ObjectOp - needs migration to `self.baseShapes(obj)` |

#### Circular Hole Operations (PathCircularHoleBase.ObjectOp)
|operation| Status | Notes |
|---|---|---|
|Tapping| Strategy of drilling | CircularHoleBase operations need migration to `self.baseShapes(obj)` and depth transformation testing |
|ThreadMilling| ✅ DONE | Inherits from CircularHoleBase - already uses `self.baseShapes(obj)` |

#### Engrave-Based Operations (PathEngraveBase.ObjectOp)
|operation| Status | Notes |
|---|---|---|

#### Deprecated Operations
|operation| Status | Notes |
|---|---|---|
|MillFace| Deprecated | Will not be adapted |

### Migration Patterns

#### Pattern 1: Direct Base Operations
- **Inheritance**: `PathOp.ObjectOp`
- **Migration**: Replace `obj.Base` loops with `self.baseShapes(obj)`
- **Depth Handling**: Automatic depth transformation in base class

#### Pattern 2: Area-Based Operations
- **Inheritance**: `PathAreaOp.ObjectOp` → `PathOp.ObjectOp`
- **Migration**: Override `execute()` to call base class, then use inherited `opExecute()`
- **Depth Handling**: Automatic depth transformation in base class

#### Pattern 3: Circular Hole Operations
- **Inheritance**: `PathCircularHoleBase.ObjectOp` → `PathOp.ObjectOp`
- **Migration**: Replace `obj.Base` loops with `self.baseShapes(obj)`
- **Depth Handling**: Automatic depth transformation in base class

#### Pattern 4: Stock-Only Operations
- **Inheritance**: `PathOp.ObjectOp`
- **Migration**: No geometry migration needed (uses transformed stock)
- **Depth Handling**: Skip depth transformation (stock-relative depths)

#### Pattern 5: Engrave-Based Operations
- **Inheritance**: `PathEngraveBase.ObjectOp` → `PathOp.ObjectOp`
- **Migration**: Replace `obj.Base` loops with `self.baseShapes(obj)`
- **Depth Handling**: Automatic depth transformation in base class

### What's Implemented

1. **Workplane Property** (`Path/Op/Base.py`)
   - `App::PropertyVector` with default `(0, 0, 1)` (Z-up)
   - Added to `__init__()` and `onDocumentRestored()` for backward compatibility

2. **Multi-Axis Positioning** (`Path/Op/Base.py` lines 847-955)
   - Reads `obj.Workplane` and checks if rotation is needed
   - Calls `solve_orientation()` to get rotary axis angles
   - Emits G0 rotation commands before toolpath generation
   - Sets `obj.Placement` for correct backplot visualization

3. **Execution Flow**
   ```
   Base.execute()
     ├─ read obj.Workplane
     ├─ if Workplane ≈ Z → skip rotation (standard 3-axis)
     ├─ solve_orientation(machine, Workplane) → SolveResult
     ├─ emit rotation commands (G0 A.. C..)
     ├─ opExecute(obj)  ← unchanged, works on original geometry
     ├─ set obj.Placement for backplot (inverse of geometry rotation)
     └─ obj.Path = Path(commandlist)
   ```

## Depth Transformation Architecture

### The Problem
When you rotate a workpiece 90°, a 10mm depth becomes nearly 0mm in world coordinates. This breaks operations that expect meaningful depth values.

### The Solution
The base class automatically transforms depth values (StartDepth, FinalDepth, SafeHeight, ClearanceHeight) to match the rotated coordinate system.

### Special Cases (Exclusions)
Some operations need different behavior:
- **Stock-only operations** (MillFacing, Probe): Depths are relative to stock, not workpiece
- **Area operations** (Pocket, Profile): Depths come from transformed geometry, not properties
- **Hole operations** (Drilling): Holes drill along local Z-axis, not world Z
- **Special algorithms** (Adaptive, Vcarve): Have unique depth requirements

These operations are excluded from depth transformation and handle depths their own way.

### Current Limitations

- **Geometry Transformation**: Operations still compute toolpaths in world coordinates
  - Toolpaths are positioned correctly but geometry isn't transformed to Z-up frame
  - This works for positioning but may affect depth calculations for tilted faces
- **Axis Role Handling**: Current implementation assumes table-table kinematics
  - Head-rotary and mixed machines will position correctly but geometry orientation
    may not match the physical machine's kinematic behavior
- **Depth Calculations**: Computed from original geometry bounding boxes
  - For tilted faces, depths may not reflect the actual cutting conditions

### Future Enhancements (Phase 3.2+)

1. **Geometry Transformation**: Transform geometry copies to Z-up frame before `opExecute()`
   - Requires careful handling of sub-element references (`obj.Base`)
   - Stock shape must be transformed consistently
   - Depth calculations must run after transformation

2. **Head/Mixed Support**: Account for axis roles in geometry rotation
   - Table-only: `compute_rotation_matrix(chain, angles)`
   - Head-only: `compute_rotation_matrix(chain, angles).inverted()`
   - Mixed: Combined rotation based on axis roles

3. **Operation Compatibility**: Ensure all operations work with transformed geometry
   - Test with Pocket, Profile, Adaptive, Deburr, Surface operations
   - Handle edge cases (sub-element references, compound shapes)

## Testing Strategy

### Current Coverage

**Solver Tests** (`CAMTests/TestPathRotationGenerator.py`)
- 13 test cases covering table-table, head-head, mixed, single-axis machines
- Regression against legacy generator for C-A configurations
- Axis limits, solution preferences, current state awareness
- **All tests pass** ✅

**Integration Tests**
- Multi-axis positioning in running application verified ✅
- Rotation commands generated correctly for various machine configurations
- Backplot visualization shows toolpaths at correct world-space orientation

### Future Testing (Phase 3.2+)

1. **Geometry Transformation Tests**
   - Drilling on 45° tilted face: verify hole positions transform correctly
   - Pocket/Profile on angled faces: check toolpath geometry
   - Depth calculations in rotated frame

2. **Machine Configuration Tests**
   - Head-rotary machines: verify geometry rotation inverse
   - Mixed machines: test combined table/head kinematics
   - Edge cases: axis limits, singularities, solution preferences

3. **Operation Compatibility Tests**
   - All operations work with transformed geometry
   - Sub-element references resolve correctly
   - Stock transformation consistency

## Migration Status

### ✅ Phase 3.1: Multi-Axis Positioning (COMPLETED)
- Workplane property added ✅
- Solver integration ✅
- Rotation command emission ✅
- Backplot support ✅
- All machine types supported ✅

### 🔄 Phase 3.2: Geometry Transformation (NEXT)
- Transform geometry to Z-up frame before toolpath generation
- Handle sub-element references
- Stock transformation
- Depth calculations in rotated frame

### ⏳ Phase 3.3+: Extended Support
- Head-rotary and mixed machine geometry handling
- All operation compatibility
- Comprehensive integration testing

## References

- Detailed specification: `kinsspecs.md`
- Machine model: `Machine/models/machine.py`
- Solver: `Path/Base/Generator/rotation.py`
- Solver tests: `CAMTests/TestPathRotationGenerator.py`
- Base operation: `Path/Op/Base.py` (lines 847-955: multi-axis positioning implementation)
- GUI integration: `Path/Op/Gui/Base.py` (workplane property handling)

## Key Implementation Details

### Solver Enhancements

The orientation solver now handles all machine configurations robustly:

1. **Dual Decomposition**: Tries both axis orders to handle singularities where the desired vector is parallel to one axis's rotation vector
2. **Role-Aware Validation**:
   - Table rotaries: `R.multVec(desired) ≈ Z`
   - Head rotaries: `R.multVec(Z) ≈ desired`
   - Mixed machines: self-consistency check only
3. **Head-Rotary Support**: Dedicated decomposition solving `R.multVec(Z) = desired` using actual tilt angles and azimuthal differences
4. **Public APIs**: `compute_rotation_matrix()` and `build_kinematic_chain()` exported for geometry transformation

### Operation Integration

The base operation now supports multi-axis positioning:

1. **Workplane Property**: `App::PropertyVector` with Z-up default
2. **Positioning Logic**: Checks if rotation needed, calls solver, emits G-code
3. **Backplot Support**: Sets operation Placement for correct 3D visualization
4. **Backward Compatibility**: Property added to existing documents automatically

### Current Status

- ✅ All solver tests pass (13 test cases)
- ✅ Multi-axis positioning works in running application
- ✅ Rotation commands generated correctly
- ✅ Backplot shows toolpaths at correct orientation
- ⏳ Geometry transformation (Phase 3.2) - next major enhancement
