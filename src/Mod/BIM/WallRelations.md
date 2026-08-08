# BIM wall relations

Wall joins are persistent relation objects.  A relation stores the user
choices and the links to its walls; it does not own a replacement wall shape.
During recompute it solves global trim planes, and the linked walls apply the
winning planes to their own shapes.

## Example document

`data/examples/BIMWallJoins.py` is the source for the checked-in
`data/examples/BIMWallJoins.FCStd` document.  Regenerate the document from the
repository root with the GUI FreeCAD binary:

```text
./build/bin/FreeCAD data/examples/BIMWallJoins.py
```

The script saves the generated document at the path above.  Use the GUI
binary because the example includes visible ShapeString labels and relation
view providers.

## Recompute flow

The implementation is intentionally split into pure geometry, relation
objects, arbitration, and wall geometry:

1. `Arch.makeWallJoint()` or `Arch.makeWallJunction()` creates a persistent
   `App::FeaturePython` relation and stores its input links and settings.
2. The relation proxy calls `ArchWallRelationResolver.solve_wall_relation()`
   from `execute()`.  The resolver dispatches to the two-wall or junction
   solver and then applies relation precedence.
3. The solver returns status, resolved wall ends, global cutting planes, and
   (for miters) any temporary extension distance.  It must only read document
   state; it must not mutate wall geometry or call `recompute()`.
4. `ArchWallRelationResolver.collect_wall_relation_endings()` gathers the
   enabled, winning claim for each wall end.
5. `ArchWall._Wall.execute()` resolves the winning end conditions once,
   extends the construction solid along the resolved baseline, and then calls
   `processSubShapes()` so openings are cut into the extended construction.
6. `ArchWall._Wall.process_endings()` applies the selected global or wall-local
   cutting planes to that processed solid.
7. IFC export asks the wall whether its processed shape is required.  A wall
   with a manual or relation-derived end condition is exported as the exact
   processed BREP instead of the untrimmed swept extrusion.

## Module responsibilities

| Module | Responsibility |
| --- | --- |
| `ArchWallGeometry.py` | Represent resolved baselines, finite paths, section layers, and pure geometry queries. |
| `ArchWallRelation.py` | Two-wall miter, butt, and tee geometry plus common relation helpers and solution data. |
| `ArchWallJunctionSolver.py` | Solve the supported carrier-and-branch topology for three or more walls. |
| `ArchWallRelationResolver.py` | Dispatch relation types, arbitrate end ownership, and collect winning trims. |
| `ArchWallTrimming.py` | Extend construction solids along resolved baselines and apply finite cutting-plane tools with graceful OCC fallback. |
| `ArchWallJoint.py` | Persistent two-wall relation proxy, outputs, view provider, and edit panel. |
| `ArchWallJunction.py` | Persistent multi-wall relation proxy and view provider. |
| `ArchWallEndCondition.py` | Select among manual and relation-derived wall-end cuts. |
| `ArchWallEndpoint.py` | Calculate and apply wall endpoint edits without recomputing implicitly. |
| `ArchWall.py` | Recompute wall solids and consume the resolved end conditions. |
| `bimcommands/BimJoin.py` | GUI commands that create, edit, and remove relation objects. |

## Geometry contract

`WallPath` endpoints and all solver-produced cutting-plane placements are in
global coordinates.  `Start` and `End` refer to the finite baseline endpoint
nearest the computed intersection.  The solver first intersects the infinite
extensions of two straight baselines, then checks that the point lies on both
finite segments.  If it does not, the relation reports `RequiresExtension`
and applies no trim; the solver does not silently move or lengthen a wall
baseline.

Only a single straight baseline is supported.  Curved, multi-edge, and
otherwise unsupported bases return `UnsupportedBaseline`.

Relations also require a shared resolved section normal.  The wall proxy
resolves that normal once; paths use it for their lateral frame and relation
planes.  Walls with incompatible tilted frames return `UnsupportedTopology`.

Section extents are measured laterally from the wall centerline after wall
alignment and layers have been normalized.  Miter planes use the angle
bisector between the two inward tangents.  A closed miter may need a temporary
extension of `other_extent / sin(angle)` before the plane cut; this extension
is only construction geometry and is never persisted as a new wall length.
Butt joints offset the terminating cut to the selected face of the supporting
wall.  Tee joints trim only the selected stem wall; a junction is the
multi-wall form where one carrier passes through a common point and every
branch wall ends at that point.

`WallTrimClaim` is the common solver output: it identifies one wall end, its
global cutting plane, and any temporary construction extension.  The resolver
arbitrates these claims without knowing whether they came from a joint or a
junction.  `WallSection` is resolved by `ArchWall._Wall.get_resolved_section()` from the
same width, alignment, offset, and material-layer choices used to build the
wall.  Negative layers remain in the cursor progression but do not contribute
faces or physical bounds.

Miter extensions are overlapping translated copies of the unprocessed
construction solid along its resolved global baseline.  The overlap keeps the
extended result continuous even when the requested distance exceeds the wall
length.  They are never fused into a wall after openings or other subtractions
have been processed, which prevents extensions from refilling openings or
creating axis-aligned corner slabs.

## Status and ownership

`OK` means the relation solved and may claim its resolved wall ends.
`Disabled`, `MissingWall`, `UnsupportedBaseline`, `NoIntersection`,
`RequiresExtension`, `UnsupportedTopology`, and `SolverError` do not provide
an applied trim.  `Conflict` means the relation itself solved, but one or
more of its end claims lost ownership to an enabled relation with an earlier
precedence key; the winning relation remains responsible for the wall shape.

Relation precedence is the stable tuple `(Priority, Name)`.  Lower values win,
and `Name` provides deterministic ordering when priorities are equal.  A
conflict exists only when two enabled, successfully solved relations claim the
same wall end.  Conflict detection solves the competing relations with
`include_conflicts=False` so arbitration cannot recurse.  Invalid or disabled
relations are ignored when collecting wall trims.

## End-condition providers

Each wall end has an ordered list of providers.  The default order is
`Relation`, then `Manual`; the first active provider wins.  Relation planes are
global placements and can carry the temporary miter extension distance.
Manual `EndingStart` and `EndingEnd` placements are relative to the wall
placement.  `EndConditionOrderStart` and `EndConditionOrderEnd` are persisted
provider orders and are normalized to the known providers when changed.

## Persistence and lifecycle

`_WallJoint` and `_WallJunction` are scripted-object proxies referenced by
their module names when documents are restored.  Keep those module names and
their persisted property names stable; a rename requires an explicit
compatibility migration.  `Status`, resolved planes, resolved ends, and
conflict details are derived output properties, not additional user inputs.
The older `ConflictJointA` and `ConflictJointB` links are retained only for
document compatibility; structured conflict data is authoritative.

Changing relation inputs touches the linked walls and competing relations.
Changing a wall's geometry touches its relations, except while the document is
already recomputing.  Deleting a relation touches its former walls so they
restore their untrimmed or next-winning shape.

When extending this system, keep geometry calculations in the solver modules,
keep document mutation in the proxies, and add matrix coverage for every new
status, topology, precedence case, and wall-end provider.
