# Forms workbench

Forms is intended to make free-form product shapes inside a normal FreeCAD
document. The editable representation is a low-resolution quad control cage;
the generated surface or solid is an output used by downstream workbenches.

## Architecture

- Commands, feature proxies, and view providers stay in Python. Task-panel
  layouts are Qt Designer `.ui` files, with Python limited to binding controls
  and implementing behavior.
- `Forms/edit.py` owns the editor session, selection, draggers, and cleanup;
  `Forms/edit_tools.py` owns the individual topology-tool panels and previews.
- Every primitive shares one feature-proxy lifecycle for common persistence,
  restore migration, parameter locking, and BRep recompute behavior.
- Cage topology is persisted independently of the generated `Part::TopoShape`.
- Interactive previews must not convert the cage to BRep after every mouse move.
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

The evaluator direction is Dyadic T-mesh Subdivision (Kovacs, Bisceglio and
Zorin, ACM TOG 2015, DOI 10.1145/2766972).  It extends Catmull-Clark to local
T-junctions, agrees with analysis-suitable T-splines on regular regions, and
supports extraordinary vertices needed by closed Forms.  A Python reference
evaluator comes first for conformance tests; once its masks and BRep patch
mapping are stable, only the numerical evaluator moves to a localized native
Forms target.  UI, persistence, topology commands, transactions, and selection
mapping remain Python.

`TMeshData` persists that mesh; `LocalControlPoints` is its editable FreeCAD
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
editor opened.

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
