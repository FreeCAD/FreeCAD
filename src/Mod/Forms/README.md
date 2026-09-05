# Forms workbench

Forms is intended to make free-form product shapes inside a normal FreeCAD
document. The editable representation is a low-resolution quad control cage;
the generated surface or solid is an output used by downstream workbenches.

## Architecture

- Commands, feature proxies, and view providers stay in Python. Task-panel
  layouts generally use Qt Designer `.ui` files; the Sphere/Pipe parameter
  panels also construct controls in Python.
- `Forms/edit.py` owns the editor session, selection, draggers, and cleanup;
  `Forms/tool_*.py` controllers own each tool's controls, state and previews.
  `Forms/edit_tools.py` preserves the historical session API through forwarding
  descriptors. Controllers hold weak session references. Deferred callbacks
  have an explicit cancellable owner in `Forms/callbacks.py`.
- `Forms/feature.py` owns common feature persistence and parameter locking;
  `Forms/viewprovider.py` owns shared presentation. Historical imports from
  `box.py` remain aliases for saved-document compatibility.
- `Forms/model.py` reads the editable model, `Forms/capabilities.py` checks
  operation support, and `Forms/matching.py` applies support constraints without
  depending on Part Design.
- `Forms/edit_journal.py` owns cancellation snapshots, including created/deleted
  objects and inbound links. `Forms/preview.py` owns temporary motion geometry.
- Cage topology is persisted independently of the generated `Part::TopoShape`.
- Smooth BRep updates remain the default during dragging. The optional mesh
  preview avoids conversion on every mouse move for large models or slow computers.
- A native `App` object should be added only if persistence, recompute performance,
  or a dedicated property type cannot be served reliably by `FeaturePython`.
- Native document objects belong in `App`; native view providers belong in `Gui`.
- Conversion is a feature boundary so downstream Part Design operations see a
  stable shape while the originating cage remains editable.

Part Design exposes that boundary through **Additive Form** and **Subtractive
Form**. Their dropdowns create a Box, Cylinder, Quadball, Sphere, path-driven Pipe, Face, Torus, or hollow
Tube in the active Body. While editing, the preceding feature keeps its normal
appearance and the independent Form geometry uses Part Design's operation
preview color, leaving the complete cage free to move, rotate, extrude, and
refine. Finishing the edit fuses or cuts a closed intersecting Form. A disjoint
or open Form remains visible beside the base and reports why it has not yet
combined.

The **Match** edit tool associates one complete Form opening with an external
face or closed wire. It projects the boundary controls back to that support on
every recompute and offers Connected (G0) or Tangent (G1) continuity where the
support supplies a surface or planar wire. Standalone Forms and their support
remain separate objects. The Form stores the support as an associative
`PropertyLinkSub` plus normalized boundary coordinates, so changing the linked
face's position or size recomputes the matched opening. For a Part Design Form,
closing edit mode also caps the matched opening and combines it with the
preceding Body feature using the selected operation. This provides editable
material beyond one fixed face boundary.

The Box, Cylinder, Quadball, Sphere, Pipe, Face, Torus, and hollow Tube primitives establish that contract. They
store unique point and quad lists, offer segment counts, and preview through a
shared task panel. Double-clicking a primitive reopens that parameter panel.
Box, Cylinder, Quadball, Sphere, and each Pipe segment generate solids; Face generates an open BRep face or
shell. The converter makes one cubic B-spline face per original cage face and
interpolates Catmull-Clark limit samples using common clamped knot vectors, validates the fit
against a denser sample set, sews the faces, and rejects non-solid results. The
requested tolerance, measured deviation, refinement cap, and conversion status
are document properties.

The initial BRep converter supports closed solid and open surface all-quad
cages. Local refinement uses a versioned, stable-ID hierarchical T-mesh as its
authoritative data contract. Every logical leaf has four parametric sides, a
side may contain any number of T-points, and atomic edges carry knot intervals.
The implementation lives in `Forms/tmesh.py`; topology edits return a new,
validated mesh so document transactions never observe a partial edit.

The current evaluator is nested **uniform Catmull-Clark refinement with
hierarchical control overrides**. It is not a locally adaptive Dyadic T-mesh
Subdivision implementation. Logical editing is local, but evaluation cost grows
with the deepest level across the whole base cage. Seeding, fitting and motion
preview enforce a 250,000-sample-face budget before refinement; fitted root
grids are limited to 129 samples per side. An edit exceeding these limits is
rejected before its properties are written. Adaptive evaluation remains future
work, rather than a claimed capability of the current Python implementation.

`TMeshData` version 3 persists that mesh and evaluator parameter locations
independently of selectable leaf boundaries (versions 1 and 2 still load); `LocalControlPoints` is its editable FreeCAD
vector view, while old `LocalEdgeInserts` documents migrate when next edited.
Controls are evaluated on nested uniform Catmull-Clark levels. One fitted root
surface is retained per original cage face, and every logical leaf is an exact
parameter trim of that surface. Consequently each Insert Edge adds only one
selectable face and seam, repeated insertion can refine an existing child, and
neighboring evaluator cells never leak into the visible BRep. The shared
hover-tool handler gives Insert Edge, Insert Point, and Subdivide a replaceable
secondary task box and scene preview. Insert Point places arbitrary controls on
edges, previews consecutive chords as a polyline, commits a chain on right-click,
and exits on a second right-click. Polygon results are refined once into
mathematically equivalent Catmull-Clark quads for BRep fitting while retaining
their logical editable edges.
Subdivide independently accepts dyadic U and V counts (1, 2, 4, 8, or 16), so a
4 by 2 operation creates eight real selectable leaves. OCCT remains the final
surface sewing and solid-validation kernel.

The Edit Form command detaches a primitive into Editable mode, displays
large pickable control-point markers, and reuses FreeCAD's `SoTransformDragger`
for translation and rotation of one or several points. Native BRep vertices map
to one control point, edges to their two endpoints, and faces to all corner
points. Selection is synchronized through FreeCAD's selection observer, while
the dragger is shown only for a valid mapped selection and remains a fixed size
on screen while zooming. The task provides Point, Edge, Face, and All selection
filters, the full transform dragger, Global/View/Selection orientation, and
persistent symmetry about the local XY, XZ, or YZ plane. Set Pivot temporarily
moves the transform origin to a snapped CAD point without disturbing the
current selection; changing or clearing that selection restores its normal
center.
Each modeling action creates its own undo step. Accepting the task keeps those
actions; cancelling restores the primitive and cage to their state when the
editor opened, removes objects created by Unweld, and restores objects deleted
by Weld with their incoming links. When mesh preview is enabled, point motion displays a sampled Coin mesh;
release fits and sews the BRep. Whole-object rigid motion uses placement only.

## Preferences

Activate Forms, then open **Edit > Preferences > Forms > General**.
Both editing options are disabled by default:

- **Greedy selection** adds or removes elements without holding Ctrl. Empty-space
  clicks clear the selection in either selection mode.
- **Use mesh preview while dragging** trades smooth live BRep display for a faster
  faceted preview. Releasing the drag rebuilds the smooth CAD shape.

Changes apply to the current editing session when preferences are applied.

## Supported combinations and numerical guarantees

- Full edge-loop insertion, Erase and Fill, Fill Hole, Bridge, and Thicken
  require a base cage without local refinement or dissolved edges. The Python
  API enforces this before mutation, as do interactive callers.
- Local Insert Edge/Subdivide/Delete/Dissolve preserve stable evaluator controls.
  Editable Pipe uses the same evaluator as other Forms.
- Creases on newly inserted local edges are rejected: a smooth root patch cannot
  represent a sharp internal trim seam. Existing base-edge creases remain
  supported. Previously stored unsupported seam values can be uncreased.
- Semi-sharp weights decay by one per subdivision; 10 is infinite sharpness.
  Junctions use uniform OpenSubdiv parent/child rule transitions. Blender edge
  and vertex weights map as `sharpness = 10 * weight**2` (inverse square root).
- `MaximumDeviation` is a sampled fit check, not a certified continuous bound.
  A valid OCCT shape does not certify G1 continuity between patches. Match
  constrains controls in the Form's coordinate system, including parent
  placements; curved-support surface continuity requires separate assessment.
- `Forms::Surface` uses boundary-constrained OCCT filling/UV partitioning, which
  differs from ordinary subdivision. Its profile-replacement controls should
  not be interpreted as a general NURBS or periodic/holed-surface editor.
- Forms is a mandatory build dependency of Part Design in this tree.

## Tool outline

### Create

1. Box, cylinder, sphere, face, torus, and hollow tube (implemented)
2. Plane and other primitive cage layouts
3. Revolve and sweep a cage from sketches or curves
4. Pipe along a path
5. Reference canvases with calibration, opacity, and view locking

### Select and transform

1. Vertex selection and move/rotate manipulator (initial implementation)
2. Edge, face, body, loop, and ring selection filters
3. Scale manipulation, snapped Set Pivot, and world, view, selection, and
   per-entity coordinate spaces (implemented)
4. Soft selection with distance/topology falloff and symmetry
5. Grow, shrink, invert, and shortest-path selection
6. Numerical transform entry and snapping to CAD geometry

### Edit topology

1. Repeated hover-preview local Insert Edge (M changes direction), free-position polyline Insert Point, full edge-loop insertion, U/V Subdivide handler, and Alt-drag face/boundary-edge extrusion (implemented)
2. Delete Face, edge/segment Dissolve, Fill Hole, equal-loop Bridge, and minimal Erase and Fill (implemented)
3. Crease/uncrease presets and continuous semi-sharp weights (implemented)
4. Straighten edge chains and best-fit Flatten (implemented)
5. Normal/Sharp Thicken for open surfaces, with a debounced editable-solid preview (implemented)
6. Soft and axis-directed Thicken variants
7. Weld/collapse cage kernel foundation; BRep mapping still needs extraordinary-face support
8. Inset, slide, arbitrary-count subdivide, cut, and split
9. Relax, smooth, and project
10. Mirror and persistent symmetry planes

### Fit and interoperate

1. Snap or project cage points to sketches, surfaces, meshes, and point clouds
2. Retopology on reference geometry
3. Import/export a documented control-cage format
4. Convert mesh or suitable NURBS geometry into a starting cage

### Finish and validate

1. Live smooth/control-cage preview at adjustable subdivision levels
2. Boundary, non-manifold, extraordinary-vertex, and self-intersection diagnostics
3. Zebra, curvature, reflection-line, and draft analysis
4. Convert to surface, shell, or solid with an explicit tolerance
5. Preserve links between the editable form and downstream BRep features

Class-A patch construction remains a related but distinct workflow. It should
reuse Forms analysis and reference tools rather than being conflated with cage
modeling in the initial implementation.

## Blender import

**File > Import** accepts `.blend` documents when Blender 4.0 or newer is
installed. Blender is launched in background mode with automatic script
execution disabled, and exports only the editable polygon control cage. Forms
bakes modifiers before the first Catmull-Clark Subdivision Surface modifier,
leaves subdivision unapplied, and transfers vertex and edge crease weights.

Compatible objects require a manifold, all-quad control cage. A Mirror modifier
is supported, including on a mesh without a Subdivision Surface modifier.
Modifiers after subdivision, non-quad faces, loose vertices, branched boundaries,
and non-manifold edges are rejected with an explanatory message. A standard
Blender installation is discovered automatically; a nonstandard executable may
be selected through the `BlenderExecutable` string parameter under
`BaseApp/Preferences/Mod/Forms`.

## Blender export

**File > Export** writes selected objects to `.blend` through the same Blender
installation. Standalone Forms primitives and editable Forms are written as
quad control cages with a Catmull-Clark Subdivision Surface modifier and their
vertex and edge creases. A selected Part Design Body or ordinary shape is
written as its final tessellated mesh, so its visible result is preserved but
its FreeCAD feature history is not. Forms using local T-mesh refinement, local
edge insertion, or dissolved control edges are also exported as their final
mesh because those topologies have no direct Blender subdivision-cage equivalent.

General-shape tessellation uses a 0.1 mm linear deflection by default. It may be
changed through the `BlenderLinearDeflection` float parameter under
`BaseApp/Preferences/Mod/Forms`.
