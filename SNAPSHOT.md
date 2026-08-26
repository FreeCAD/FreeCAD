# Assembly Graph Snapshot and Query Implementation Plan

## 1. Purpose

Implement two provider-neutral CadX tools:

- `assembly.graph_snapshot` captures the active Assembly shown in FreeCAD,
  converts its exact document and presentation state into a versioned semantic
  property graph, and stores the immutable graph in process memory.
- `assembly.graph_query` performs bounded, deterministic searches and traversals
  against a stored graph revision.

The implementation belongs entirely in this repository. It must use cad-x's own
contracts, graph representation, C++ services, tests, and provider adapter. The
VibeCAD implementation that motivated this work is design reference only. Do not
copy its modules, schemas, runtime classes, names, or provider workflow.

The authoritative state remains the FreeCAD document. A stored graph is a
rebuildable, read-only projection used for reasoning and tool targeting; it must
never become an alternate document history or silently override live FreeCAD
state.

## 2. Current cad-x baseline

As of this plan, the cad-x module contains:

- a Python Qt chat panel;
- a provider-agnostic local Ollama model selector;
- an OpenAI-compatible streaming chat-completions client;
- a single-background-turn `ChatSession`;
- no executable model tools;
- no implemented tool registry despite the registry boundary described in the
  README;
- no document-thread dispatcher;
- no graph store, graph schema, document observer, or Assembly extractor; and
- no C++ `App` or `Gui` CadX library yet.

The existing Python panel and Ollama support should remain working while the
new capability is added. Do not move or rename the current public Python files
as part of this work. Add the C++ and tool paths alongside them, then migrate
callers incrementally.

The repository-level direction is C++-first:

- authoritative document inspection, graph construction, graph storage, and
  query execution live in `src/Mod/CadX/App`;
- active-view inspection and main-thread scheduling live in
  `src/Mod/CadX/Gui`;
- Python remains the module bootstrap and provider adapter; and
- the local model remains selectable rather than hard-coded.

## 3. Required behavior and boundaries

### 3.1 Exact meaning of “the stuff in the window”

For version 1, the phrase means:

1. the active FreeCAD document;
2. the exact `Assembly::AssemblyObject` active for editing in the active 3D
   view;
3. component occurrences belonging to that Assembly whose effective
   presentation is visible in that view; and
4. the hidden source definitions, joints, and semantic dependencies required
   to explain those visible occurrences.

Visibility selects the scene roots. It does not define model membership. For
example, a visible `App::Link` may reference a hidden source Body in another
document. The graph must include the visible occurrence and its hidden source
definition, with presentation state stored separately.

Version 1 does not capture every unrelated object visible in the document or
infer an Assembly from an arbitrary collection of shapes. If there is no exact
active Assembly, `assembly.graph_snapshot` fails with
`CADX_NO_ACTIVE_ASSEMBLY`; it does not guess from selection, labels, tree order,
or geometry.

### 3.2 Read-only behavior

Both tools are read-only:

- no FreeCAD transaction is opened;
- no document properties are created;
- no persistent FreeCAD Assembly Snapshot object is created;
- no selection, camera, visibility, edit mode, or active document is changed;
- no recompute is triggered merely to make the graph easier to build; and
- no partial graph is published when a hard validation or resource bound fails.

FreeCAD already has an Assembly Snapshot command that persists placements and
visibility in the document. That object is separate from this in-memory
semantic graph and must not be reused as the graph database.

### 3.3 Authority and staleness

- FreeCAD document objects are authoritative.
- Graph nodes retain exact source references and the document revisions from
  which they were captured.
- Query results always identify the graph revision they came from.
- A document or linked-source change marks dependent graph snapshots stale.
- A stale graph can remain available for diagnostics, but normal queries fail
  with `CADX_GRAPH_STALE` unless the request explicitly asks only for stale
  metadata.
- A new successful snapshot atomically replaces the current graph revision for
  that Assembly scope.

## 4. Architectural shape

```text
Ollama tool call
      |
      v
Python provider adapter and turn loop
      |
      v
provider-neutral CadX tool registry
      |
      +---------------------------+
      |                           |
      v                           v
assembly.graph_snapshot      assembly.graph_query
      |                           |
      v                           v
GUI main-thread gateway       immutable GraphSnapshot
      |                           |
      v                           v
ActiveAssemblyResolver        GraphQueryEngine
      |
      v
AssemblyCapture DTO
      |
      v
AssemblyGraphBuilder
      |
      v
per-process GraphStore
```

The tools must not dispatch other provider-visible tools internally. They call
shared C++ services directly. This avoids recursive tool dispatch, repeated JSON
serialization, duplicated locking, and accidental tool receipts inside another
tool call.

## 5. Proposed source layout

Add the following structure without removing the current Python module:

```text
src/Mod/CadX/
  App/
    CMakeLists.txt
    AppCadX.cpp
    AppCadXPy.cpp
    CadXService.h/.cpp
    ToolDefinition.h/.cpp
    ToolRegistry.h/.cpp
    ToolResult.h/.cpp
    GraphTypes.h/.cpp
    GraphSnapshot.h/.cpp
    GraphStore.h/.cpp
    GraphQuery.h/.cpp
    GraphRevision.h/.cpp
    AssemblyCapture.h/.cpp
    AssemblyGraphBuilder.h/.cpp
    AssemblyObjectAdapter.h/.cpp
    AssemblyDocumentObserver.h/.cpp
  Gui/
    CMakeLists.txt
    AppCadXGui.cpp
    AppCadXGuiPy.cpp
    ActiveAssemblyResolver.h/.cpp
    AssemblyViewCapture.h/.cpp
    MainThreadGateway.h/.cpp
  cadx_tests/
    test_cadx_tool_contracts.py
    test_cadx_tool_turn.py
  CadXToolBridge.py
  CadXToolProtocol.py
```

Add C++ tests under the normal FreeCAD test layout selected during the first
implementation phase, for example:

```text
tests/src/Mod/CadX/App/
  CMakeLists.txt
  GraphStoreTest.cpp
  GraphQueryTest.cpp
  AssemblyGraphBuilderTest.cpp
  AssemblyGraphLifecycleTest.cpp

tests/src/Mod/CadX/Gui/
  ActiveAssemblyResolverTest.cpp
  AssemblyViewCaptureTest.cpp
```

Use Python extension module names `CadXApp` and `CadXGuiApp` so they do not
collide with the existing `CadXGui.py` module.

## 6. Tool ownership and registration

The owning registry is `cadx.graph`. The two public tool names are
`assembly.graph_snapshot` and `assembly.graph_query`. The separate
`freecad.document` registry remains available for future mutation tools and is
not a dependency of either graph tool.

Create a provider-neutral tool definition type containing:

```text
name
description
classification: read | mutation | presentation
input_schema
output_schema_version
executor
thread_requirement
result_size_limit
```

The registry must reject:

- duplicate names;
- malformed or open-ended schemas;
- an executor whose declared thread requirement is unavailable;
- output without the declared schema version; and
- result payloads over their byte limit.

Tool definitions are converted to OpenAI-compatible function definitions only
inside the Ollama provider adapter. The graph and registry layers must not
contain Ollama-specific types.

## 7. Public tool contracts

### 7.1 `assembly.graph_snapshot`

Description:

> Capture the active Assembly view as a revisioned semantic graph and retain it
> in memory for bounded queries.

Version 1 input schema:

```json
{
  "type": "object",
  "properties": {
    "geometry_detail": {
      "type": "string",
      "enum": ["none", "summary"],
      "default": "summary"
    },
    "include_view_state": {
      "type": "boolean",
      "default": true
    },
    "refresh": {
      "type": "string",
      "enum": ["if_stale", "always"],
      "default": "if_stale"
    }
  },
  "additionalProperties": false
}
```

The tool deliberately has no Assembly target. Its purpose is to capture the
exact Assembly currently active in the view. A separate future tool can support
explicit headless Assembly targets if a real workflow requires it.

Successful result envelope:

```json
{
  "schema_version": "cadx.assembly-graph-result.v1",
  "graph_id": "assembly-graph:<opaque-id>",
  "graph_revision": "sha256:<content-hash>",
  "presentation_revision": "sha256:<presentation-hash>",
  "document": {
    "document_uid": "<FreeCAD document UUID>",
    "document_name": "AssemblyDocument"
  },
  "active_assembly": {
    "node_id": "<opaque-node-id>",
    "object_name": "Assembly",
    "label": "Main Assembly"
  },
  "complete": true,
  "node_count": 42,
  "edge_count": 57,
  "visible_occurrence_count": 12,
  "unresolved_reference_count": 0,
  "diagnostics": []
}
```

The tool returns a handle and summary, not the full graph. This keeps model
context bounded and makes `assembly.graph_query` the only graph-read contract.

### 7.2 `assembly.graph_query`

Description:

> Query nodes and relationships from one exact stored Assembly graph revision.

Use a closed `oneOf` schema with five operations. Each operation exposes only
its relevant fields.

1. `summary`
   - returns graph metadata, node/edge counts by kind, active Assembly, source
     documents, diagnostics, and stale state;
2. `find_nodes`
   - filters by node kind, exact native type, normalized label text, semantic
     part kind, visibility, source document, and bounded property predicates;
3. `neighbors`
   - returns incoming, outgoing, or both directions from exact node IDs, filtered
     by edge kind;
4. `subgraph`
   - traverses from exact node IDs to a maximum depth of four using an explicit
     edge-kind allowlist; and
5. `shortest_path`
   - finds one deterministic bounded path between two exact node IDs over an
     explicit edge-kind allowlist.

Common required inputs:

```json
{
  "graph_id": "assembly-graph:<opaque-id>",
  "graph_revision": "sha256:<content-hash>",
  "operation": "find_nodes",
  "limit": 50
}
```

Common bounds:

- `limit`: 1 to 100, default 50;
- traversal depth: 0 to 4;
- at most 16 start nodes;
- at most 16 node kinds or edge kinds per request;
- deterministic sort by node kind, normalized label, then node ID;
- opaque cursor bound to graph revision and canonical query hash;
- maximum encoded result size configured independently of graph size; and
- no arbitrary expression language, regex execution, SQL, Cypher, or user code.

Successful query envelope:

```json
{
  "schema_version": "cadx.assembly-graph-query-result.v1",
  "graph_id": "assembly-graph:<opaque-id>",
  "graph_revision": "sha256:<content-hash>",
  "operation": "find_nodes",
  "nodes": [],
  "edges": [],
  "returned_node_count": 0,
  "returned_edge_count": 0,
  "truncated": false,
  "next_cursor": null,
  "diagnostics": []
}
```

The query tool must never silently switch to a newer graph revision. A stale or
unknown revision returns an exact error and tells the caller to take a new
snapshot.

## 8. Graph database layout

### 8.1 Storage model

Use an in-process immutable property graph implemented with standard C++ data
structures. Do not add an external graph-database dependency for version 1.

`GraphStore` owns entries by `graph_id`:

```text
GraphStore
  unordered_map<GraphId, GraphEntry>

GraphEntry
  ScopeKey scope
  shared_ptr<const GraphSnapshot> current
  deque<shared_ptr<const GraphSnapshot>> retainedRevisions
  StaleState stale
  SourceDependencySet sourceDocuments
  MemoryUsage bytes
```

`GraphSnapshot` contains immutable records and indexes:

```text
GraphHeader
vector<NodeRecord> nodes
vector<EdgeRecord> edges
unordered_map<NodeId, node-index> nodeById
unordered_map<NativeObjectKey, vector<NodeId>> nodesByNativeObject
unordered_map<OccurrenceKey, NodeId> nodeByOccurrence
unordered_map<NodeKind, vector<NodeId>> nodesByKind
unordered_map<NormalizedLabel, vector<NodeId>> nodesByLabel
unordered_map<NodeId, vector<edge-index>> outgoing
unordered_map<NodeId, vector<edge-index>> incoming
unordered_map<NodeId, vector<NodeId>> jointsByEndpoint
unordered_map<SourceDocumentUid, vector<NodeId>> nodesBySourceDocument
```

Build indexes before publication. Queries operate only on an immutable
`shared_ptr<const GraphSnapshot>`, so they need no document access and may run
off the FreeCAD main thread.

### 8.2 Typed records, not an unbounded property bag

Use typed payload variants:

```text
NodeRecord
  NodeId id
  NodeKind kind
  NativeIdentity native
  DisplayIdentity display
  Provenance provenance
  Presentation presentation
  variant<NodePayload...> payload

EdgeRecord
  EdgeId id
  EdgeKind kind
  NodeId from
  NodeId to
  Provenance provenance
  variant<EdgePayload...> payload
```

Common scalar properties may be exposed through query filters, but arbitrary
FreeCAD property values must not be copied wholesale into a generic map.
Adapters select relevant bounded properties and preserve an exact diagnostic
when a custom property cannot be represented.

### 8.3 Node kinds

Version 1 node kinds:

```text
Document
AssemblyDefinition
AssemblyOccurrence
PartDefinition
BodyDefinition
FeatureDefinition
Occurrence
OccurrenceGroup
Joint
JointConnector
RigidGroup
GroundConstraint
SemanticInterface
Datum
Material
BomIdentity
OrganizationalGroup
AssemblyArtifact
UnresolvedDefinition
```

`AssemblyArtifact` covers existing non-part Assembly children such as BOMs,
exploded views, simulations, and persistent FreeCAD snapshots. These are
represented so they are not mistaken for physical components.

### 8.4 Edge kinds

Version 1 edge kinds:

```text
CONTAINS
HAS_DEFINITION
INSTANCE_OF
OCCURS_IN
HAS_BODY
HAS_FEATURE
HAS_INTERFACE
HAS_DATUM
HAS_MATERIAL
HAS_BOM_IDENTITY
SOURCE_OBJECT
SOURCE_DOCUMENT
NESTED_OCCURRENCE
EXPANDS_TO
HAS_JOINT
JOINT_ENDPOINT
REFERENCES_INTERFACE
REFERENCES_TOPOLOGY
GROUNDED_BY
MEMBER_OF_RIGID_GROUP
DEPENDS_ON
VISIBLE_IN
SELECTED_IN
HAS_ARTIFACT
UNRESOLVED_SOURCE
```

Containment, occurrence, constraint, source, and presentation relationships
must remain distinct. Do not overload `CONTAINS` to mean instance-of, connected
by a joint, or visible in the viewport.

### 8.5 Identity model

Use three separate identities:

1. `NativeObjectKey`
   - source document `Document::Uid`;
   - immutable FreeCAD object `Name`;
   - exact `TypeId`;
2. `OccurrenceKey`
   - active Assembly root key;
   - stable sequence of component object names from the root to the occurrence;
   - link-group parent and link-element name where applicable;
3. `NodeId`
   - an opaque deterministic 128-bit or SHA-256-derived ID over schema version,
     node role, and canonical identity.

Labels are display metadata only. Tree row, localized text, object index, and
absolute filename are never identity.

FreeCAD objects do not all expose a dedicated persistent UUID. For the
rebuildable version-1 graph, document UUID plus immutable object Name is the
source identity. The observer must tombstone a deleted object before any later
object can reuse the same Name in the process. If future requirements need
identity across destructive delete/recreate operations, introduce an explicit
CadX UUID through a separately approved document-migration design; the snapshot
tool itself must not write such a property.

Exact `FaceN`, `EdgeN`, and `VertexN` names are revision-bound topology
references, not durable semantic IDs. Store them only with the graph revision,
source shape signature, and provenance. Prefer published datum or semantic
interface names when present.

### 8.6 Revisions

Store two hashes:

- `graph_revision` hashes canonical semantic nodes, edges, source identities,
  placements, relevant properties, source-document revision counters, and
  geometry summaries;
- `presentation_revision` hashes effective visibility, selection, active view,
  and camera state.

Exclude timestamps, memory addresses, unordered-container iteration order, and
localized display text from revision hashes. Canonically sort records before
hashing.

Also maintain a monotonic in-process revision counter per document for fast
invalidation. The content hash is the externally visible revision because it
is deterministic and detects accidental nondeterminism in tests.

### 8.7 Memory policy

Initial configurable limits:

- 10,000 nodes per snapshot;
- 40,000 edges per snapshot;
- 64 MiB estimated graph memory per snapshot;
- current revision plus two prior revisions per Assembly scope;
- 256 MiB total CadX graph memory by default; and
- least-recently-used eviction of inactive, unpinned graphs.

If a snapshot exceeds a hard limit, return `CADX_GRAPH_LIMIT_EXCEEDED` and do
not publish a truncated current graph. Queries are independently paginated and
result-size bounded.

## 9. Accurate Assembly part representation

### 9.1 Use orthogonal classification facets

Do not force every object into one fragile `PartType` enum. Preserve the exact
FreeCAD `TypeId` and classify each node along independent dimensions:

```text
role
  definition | occurrence | relation | interface | organization | artifact

container_kind
  assembly | part | body | link_group | group | none

geometry_kind
  solid | compsolid | shell | face | wire | edge | vertex | compound |
  mixed | empty | unavailable

provenance_kind
  local_parametric | local_direct_shape | external_link | imported_shape |
  generated_component | custom_feature | unresolved

semantic_part_kind
  generic | fastener | gear | bearing | shaft | enclosure | pcb | motor |
  user_declared | unknown
```

Only set `semantic_part_kind` from exact native/catalog metadata or an explicit
user-authored CadX semantic property. Geometry- or label-based guesses must be
reported as an inference with confidence and evidence, never promoted to exact
identity.

### 9.2 Required adapter coverage

Classify most-specific types before base classes.

| FreeCAD object form | Graph representation | Required facts |
|---|---|---|
| `Assembly::AssemblyObject` | `AssemblyDefinition`; also an `AssemblyOccurrence` when nested directly | root/nested role, parent, local/global placement, component count, source document |
| `Assembly::AssemblyLink` | one subassembly `Occurrence` plus `INSTANCE_OF` source Assembly; nested occurrences through `EXPANDS_TO` | linked Assembly, rigid/flexible state, placement, generated child mapping, unresolved source state |
| `App::Link` with `ElementCount == 0` | one `Occurrence` linked to a definition | linked object, link placement, source document, effective visibility |
| `App::Link` with `ElementCount > 0` | `OccurrenceGroup` plus one occurrence per `App::LinkElement` | source definition, element count, element names, per-element placement and visibility |
| `App::LinkElement` | one occurrence whose path includes its parent link group | parent group, linked object, stable element name, placement |
| direct `App::Part` under Assembly | `PartDefinition` plus direct `Occurrence` | contained Bodies/features, placement, shape summary, external/local provenance |
| `PartDesign::Body` | `BodyDefinition`; an occurrence only when the Body itself is an Assembly component | Tip identity, feature history references, final shape summary, placement |
| `Part::Feature` and derived primitives | `FeatureDefinition`; direct occurrence when it is a component | exact subtype such as `Part::Box`, Shape status, placement, geometry summary |
| imported or copied shape represented by a `Part::Feature` | `FeatureDefinition` with `imported_shape` provenance when exact metadata proves it | import/source properties, shape signature, unknown semantic intent if not declared |
| custom `Part::FeaturePython` or compatible custom feature | `FeatureDefinition` with `custom_feature` provenance | exact TypeId, proxy type/module when safe, bounded scalar/link metadata, shape summary |
| `App::DocumentObjectGroup` | `OrganizationalGroup` only | child ordering and containment; never count it as a physical part |
| PartDesign features inside a Body | internal `FeatureDefinition` nodes when requested by closure | Body membership, predecessor/dependency edges, suppression/activity, never separate Assembly occurrence by default |
| Sketch, datum plane/axis/point, LCS | `Datum` or internal feature nodes | owner definition, placement, published semantic name; no physical-part count |
| `Assembly::JointGroup`, BOM/View/Simulation/Snapshot groups | relation or `AssemblyArtifact` containers | ownership and artifact type; no physical-part count |
| ordinary joint `App::FeaturePython` | `Joint` plus two `JointConnector` nodes | exact joint type, endpoint occurrence paths, connector placements, limits, suppression |
| grounded joint | `GroundConstraint` | exact grounded occurrence |
| rigid-group joint | `RigidGroup` plus membership edges | ordered exact members and suppression |
| unknown shape-bearing object | `FeatureDefinition` or `UnresolvedDefinition`, never omitted | exact TypeId, native identity, diagnostic, available shape summary |

This table is the minimum fixture matrix. New object adapters may enrich
records, but the fallback adapter must preserve unknown objects rather than
drops them.

### 9.3 Definition versus occurrence rules

- Every reusable source object has one definition node per source document and
  object identity.
- Every placement in the active Assembly has a distinct occurrence node.
- Two links to the same Body produce one Body definition and two occurrences.
- A direct shape object under the Assembly still gets a separate occurrence
  node, even though its source definition is the same document object.
- A rigid subassembly is one solver-level rigid occurrence but retains its
  internal semantic definition closure for queries.
- A flexible subassembly exposes nested movable occurrences and internal joints
  in the parent Assembly context.
- Generated mirror objects inside `Assembly::AssemblyLink` are implementation
  projections. Map them to their source occurrence paths and do not count them
  as independent part definitions.
- Features contained inside an `App::Part` or `PartDesign::Body` are definition
  internals unless the Assembly graph explicitly places them as components.

### 9.4 Placement model

Store both:

- `local_placement`: placement relative to the occurrence's immediate Assembly
  or occurrence parent; and
- `world_placement`: placement resolved in the active Assembly view at capture
  time.

Placements contain translation in millimeters and a normalized quaternion. The
hash canonicalizer must normalize equivalent quaternions and negative zero.

For links, arrays, and nested Assemblies, calculate world placement from the
exact FreeCAD subobject/occurrence path rather than multiplying guessed tree
parents. Add fixture tests for direct, external, rigid nested, flexible nested,
and link-element placements.

### 9.5 Geometry summary

`geometry_detail: "summary"` stores only bounded immutable facts:

- shape availability and validity;
- top-level shape kind;
- solid, shell, face, edge, and vertex counts;
- axis-aligned world and local bounds;
- volume, area, and center of mass when defined;
- exact source shape signature for stale topology detection; and
- optional material density/mass only when a real material assignment exists.

Do not retain live `DocumentObject*`, `ViewProvider*`, or mutable `TopoShape`
handles in the graph. Do not serialize full BREP, meshes, tessellations, or all
face/edge geometry in version 1.

Unknown volume, mass, material, or semantic type remains explicitly unknown.

## 10. Rewriting active-Assembly and snapshot behavior for cad-x

### 10.1 `ActiveAssemblyResolver`

Implement an original cad-x resolver in the Gui library. Its internal result is
an `ActiveAssemblyContext` value object:

```text
active App::Document reference
document UUID and name
active Gui::Document reference
active 3D view identifier
exact Assembly::AssemblyObject reference
Assembly object Name and TypeId
edit-mode proof
active-view proof
selection snapshot
camera/projection snapshot
```

Resolution rules:

1. execute on the GUI/main thread;
2. require one active GUI document and active 3D view;
3. read the active object for the Assembly edit role from the view;
4. require exact membership in the active App document;
5. require `Assembly::AssemblyObject` or a derived type;
6. require current edit mode and a live object Name;
7. return an immutable context without changing selection or activation; and
8. fail explicitly on stale, foreign-document, or ambiguous state.

The core `AssemblyGraphBuilder` accepts an explicit Assembly root and capture
DTO, so it remains testable without GUI state. `ActiveAssemblyResolver` is only
the provider-tool scope resolver.

### 10.2 `AssemblyViewCapture`

Capture presentation facts on the GUI/main thread:

- active view and projection type;
- camera orientation/position and clipping state when requested;
- ordered current selection paths;
- local visibility from each relevant ViewProvider;
- effective visibility through parent, link, and Assembly presentation; and
- the visible occurrence seed paths.

Take a presentation checksum before and after capture. If the view changes
during capture, retry once; then fail with `CADX_VIEW_CHANGED` rather than
publishing a mixed snapshot.

### 10.3 `AssemblyCapture`

`AssemblyCapture` is a plain immutable DTO produced on the main thread. It owns
copies of only the data needed to build the graph:

```text
document records
object records
containment records
link/source records
occurrence paths and placements
joint records
datum/interface records
selected bounded properties
geometry summaries
presentation records
capture diagnostics
```

The builder must not retain pointers into the live document after capture.

### 10.4 `AssemblyGraphBuilder`

Build in deterministic passes:

1. validate document and active Assembly identity;
2. enumerate Assembly component and artifact children;
3. establish exact containment and occurrence paths;
4. materialize source documents and definition nodes;
5. expand links, link groups, and nested AssemblyLinks with cycle detection;
6. materialize direct occurrences and definition/instance relationships;
7. materialize ordinary joints, grounded constraints, rigid groups, and
   connector endpoints;
8. attach datum and published semantic-interface nodes;
9. attach presentation state and visible-scene membership;
10. attach geometry summaries and source signatures;
11. validate graph invariants;
12. canonicalize order and compute revisions;
13. build indexes; and
14. atomically publish to `GraphStore`.

No pass may infer a missing source by matching labels. An unresolved external
link becomes an `UnresolvedDefinition` node with an `UNRESOLVED_SOURCE` edge and
diagnostic.

## 11. Graph invariants

Reject publication unless all applicable invariants hold:

- exactly one active Assembly root node;
- every occurrence belongs to the active Assembly scope;
- every resolved occurrence has exactly one `INSTANCE_OF` definition;
- every definition belongs to exactly one source document node;
- every nested occurrence path is unique;
- every ordinary joint has exactly two endpoint records, including an explicit
  ground endpoint where applicable;
- every edge endpoint exists;
- containment and nested-occurrence edges are acyclic;
- source-link cycles are detected and diagnosed;
- visible occurrences have complete occurrence paths and world placements;
- no generated AssemblyLink mirror is counted as an independent source
  definition;
- physical-part counts exclude organizational groups, joints, datums, and
  artifacts;
- all unknown or unsupported objects are represented or cause an explicit hard
  failure—never silently skipped;
- all numbers are finite and all quaternions are normalized;
- graph and presentation hashes are deterministic; and
- the final capture guard still matches the active document, Assembly, and
  presentation checksum.

## 12. Query engine design

### 12.1 Query execution

`GraphQueryEngine` receives only an immutable `GraphSnapshot` and a parsed typed
query. It must not import GUI code or call FreeCAD document APIs.

Execution order:

1. validate graph ID and exact revision;
2. reject stale graphs for normal queries;
3. validate operation-specific bounds;
4. resolve exact node IDs or indexed filters;
5. apply deterministic traversal and filtering;
6. sort before pagination;
7. encode a cursor tied to graph revision and query hash;
8. project only requested bounded fields; and
9. enforce encoded result-size limits.

### 12.2 Queryable fields

Version 1 allows exact filters over:

- node and edge kind;
- exact FreeCAD `TypeId`;
- node ID, native object Name, source document UUID;
- normalized label text with exact or contains matching;
- definition/occurrence role;
- rigid/flexible subassembly state;
- effective visibility and selection;
- geometry kind;
- semantic part kind and confidence;
- joint type and suppression;
- material/BOM identity when present; and
- unresolved/diagnostic state.

Do not expose arbitrary property-path evaluation. Add a new typed filter only
when a concrete agent workflow and fixture require it.

### 12.3 Provenance in results

Every semantic field reports one of:

```text
exact_native
exact_user_declared
derived_geometry
inferred_label
unknown
```

Inferred fields include bounded confidence and evidence. Query clients can
exclude inference when they need exact engineering facts.

## 13. Synchronization and lifecycle

### 13.1 Document observation

Implement `AssemblyDocumentObserver` using FreeCAD's C++ document/application
signals. Observe:

- document create, activate, close, save, and restore;
- object create and delete;
- relevant property changes;
- document recompute completion;
- transaction commit and abort;
- undo and redo; and
- linked source document changes.

The observer marks graphs dirty; it does not incrementally mutate an immutable
snapshot in callbacks.

Version 1 should prefer a correct full rebuild after invalidation. Incremental
patching is deferred until fixture-backed profiling proves full rebuilds are a
real bottleneck.

### 13.2 Presentation observation

The Gui service separately observes:

- active document/view changes;
- active Assembly edit state;
- ViewProvider visibility changes;
- selection changes; and
- camera/projection changes.

Presentation-only changes invalidate `presentation_revision` but do not require
semantic extraction unless the visible occurrence seed set changed. Camera-only
changes update a lightweight presentation overlay.

### 13.3 Transaction behavior

- Never publish while a FreeCAD transaction is open or a document is
  recomputing.
- Coalesce object/property callbacks during a transaction.
- On commit, mark the affected document and dependent source graphs dirty.
- On abort, discard the pending dirty set after verifying the prior document
  state is restored.
- Undo and redo always invalidate affected graph revisions.
- Snapshot capture takes start and end guards; if the guards differ, discard the
  candidate and retry once.

### 13.4 Document close and memory eviction

- Closing the active root document removes its current graph entries.
- Closing a source document marks dependent graphs stale and retains explicit
  unresolved-source diagnostics until rebuilt or evicted.
- Eviction releases only immutable graph memory; it never touches the FreeCAD
  document.

## 14. Threading model

- The Ollama network stream remains on the current background turn thread.
- Provider tool calls enter `MainThreadGateway` and wait on a future.
- All App document reads and all Gui/ViewProvider reads occur on FreeCAD's main
  thread.
- Main-thread capture copies bounded immutable DTO data and releases document
  access.
- Canonical sorting, graph indexing, hashing, and graph queries may execute on a
  worker using only immutable DTOs/snapshots.
- `GraphStore` uses a mutex for entry replacement and a shared mutex or immutable
  pointer handoff for reads.
- Never move live `DocumentObject`, `ViewProvider`, or Python proxy calls to a
  worker thread.

If geometry-summary profiling shows UI stalls, first split main-thread capture
into bounded queued batches. Moving OCCT shape work to a worker requires an
isolated immutable geometry transfer and dedicated safety tests; do not assume a
copied shape handle is independent of live document mutation.

## 15. Ollama and session integration

The current chat client sends plain messages only. Add tool support without
coupling graph code to Ollama.

### 15.1 Protocol model

Replace the text-only message assumption with provider-neutral turn items:

```text
UserMessage
AssistantText
AssistantToolCall
ToolResult
SystemMessage
```

Keep backward compatibility for existing `ChatMessage(role, text)` callers
during migration.

### 15.2 Request and stream parsing

- Include provider-converted tool definitions in the request.
- Parse streamed `delta.tool_calls`, including fragmented IDs, names, and JSON
  arguments.
- Validate complete arguments only after the tool call finishes.
- Append the assistant tool-call item and a tool result item to transcript state.
- Continue the same assistant turn after tool results until final text.
- Bound calls per turn, nested continuation count, argument bytes, result bytes,
  and total wall time.
- Preserve cancellation while waiting for main-thread dispatch or a continuation
  request.
- Reject unknown tools and malformed arguments with structured tool errors the
  model can correct.

### 15.3 Initial call limits

- at most 16 tool calls per user turn;
- at most 4 snapshot rebuilds per turn;
- one snapshot build per Assembly scope at a time;
- at most 64 KiB tool arguments;
- at most 128 KiB encoded tool result before provider projection; and
- cancellation checked between every dispatch and continuation.

The system prompt should be updated only after tools are operational. Replace
“You cannot execute tools yet” with durable behavior: query current graph state,
use exact returned identities and revisions, and refresh stale graphs before
dependent work. Do not add Assembly recipes or model-family-specific prompting.

## 16. Error model

All errors use a stable envelope:

```json
{
  "ok": false,
  "error": {
    "code": "CADX_GRAPH_STALE",
    "message": "The requested graph revision no longer matches the document.",
    "retryable": true,
    "details": {}
  }
}
```

Required codes:

```text
CADX_NO_ACTIVE_DOCUMENT
CADX_NO_ACTIVE_VIEW
CADX_NO_ACTIVE_ASSEMBLY
CADX_ACTIVE_ASSEMBLY_STALE
CADX_VIEW_CHANGED
CADX_DOCUMENT_BUSY
CADX_CAPTURE_CHANGED
CADX_UNSUPPORTED_OBJECT
CADX_UNRESOLVED_SOURCE
CADX_GRAPH_INVARIANT_FAILED
CADX_GRAPH_LIMIT_EXCEEDED
CADX_GRAPH_NOT_FOUND
CADX_GRAPH_STALE
CADX_GRAPH_REVISION_MISMATCH
CADX_QUERY_INVALID
CADX_QUERY_CURSOR_INVALID
CADX_QUERY_RESULT_TOO_LARGE
CADX_TOOL_ARGUMENTS_INVALID
CADX_TOOL_CANCELLED
CADX_INTERNAL_ERROR
```

Unknown part types normally produce a fallback node plus diagnostic. Use
`CADX_UNSUPPORTED_OBJECT` only when the object's structure prevents safe graph
identity or placement, not merely because CadX does not recognize its product
semantics.

## 17. Test fixtures and coverage matrix

Build fixtures programmatically so tests do not depend on local user files.
Retain a small saved-FCStd fixture set only for save/reopen and external-link
coverage.

Required fixtures:

1. empty active Assembly;
2. direct `Part::Box` component;
3. direct `App::Part` containing a `PartDesign::Body` and feature history;
4. direct `PartDesign::Body` component;
5. two `App::Link` occurrences referencing the same local Body;
6. external-document `App::Link` to an `App::Part`;
7. `App::Link` array with `App::LinkElement` occurrences;
8. nested rigid `Assembly::AssemblyLink`;
9. nested flexible `Assembly::AssemblyLink` with internal joints;
10. nested AssemblyLink containing another nested AssemblyLink;
11. direct primitive, imported `Part::Feature`, and custom
    `Part::FeaturePython` shape;
12. custom fastener-like and gear-like feature with exact declared semantics;
13. organizational `App::DocumentObjectGroup` around valid components;
14. Sketch, datum plane/axis/point, and LCS inside a part definition;
15. each of the thirteen ordinary Assembly joint kinds;
16. grounded component and rigid-group relation;
17. suppressed joint and unavailable endpoint;
18. hidden source with visible link occurrence;
19. visible source with hidden occurrence;
20. hidden parent with locally visible child;
21. current selection containing whole objects and subelements;
22. unresolved or closed external source document;
23. source-link cycle and nested-containment cycle rejection;
24. unknown shape-bearing derived type;
25. duplicate labels with distinct object Names;
26. large graph at and over each resource limit;
27. transaction commit, abort, undo, redo, and recompute invalidation;
28. source-document change invalidating a parent Assembly graph;
29. save/reopen preserving canonical source and occurrence identities; and
30. active document, active view, or active Assembly changing mid-capture.

For every physical fixture, assert independently:

- definition count;
- occurrence count;
- physical-part count;
- exact source identities;
- occurrence paths;
- local and world placements;
- effective visibility;
- joint endpoints and types;
- rigid/flexible behavior;
- unresolved-source behavior;
- deterministic graph revision; and
- query results and pagination.

## 18. Red/green implementation phases

Every code phase begins with a failing focused test. Do not combine the work
into one large unverified change.

### Phase 0 — Freeze contracts and fixture vocabulary

Deliverables:

- checked-in JSON schemas for both tools;
- C++ enums and data-contract documentation for node/edge kinds;
- fixture manifest covering every row in Section 9.2;
- stable error-code list; and
- golden canonical JSON examples for one simple and one nested Assembly.

Red gate:

- schema-validation tests fail because the tools are not registered.

Green gate:

- schemas are closed, bounded, provider-valid, and round-trip through the
  provider-neutral definition model.

Do not proceed if the definition/occurrence boundary or active-view scope is
still ambiguous.

### Phase 1 — Add the C++ CadX App/Gui module skeleton

Deliverables:

- `App` and `Gui` CMake targets;
- `CadXApp` and `CadXGuiApp` Python extension bootstraps;
- additive updates to `src/Mod/CadX/CMakeLists.txt` and copied/installed files;
- an application-owned `CadXService` lifetime; and
- smoke imports from FreeCAD and FreeCADGui.

Red gate:

- module import and service-lifetime tests fail.

Green gate:

- headless `import CadXApp` succeeds;
- GUI `import CadXGuiApp` succeeds;
- existing Python chat tests still pass; and
- enabling/disabling `BUILD_CADX` remains deterministic.

### Phase 2 — Implement graph records, invariants, revisions, and store

Deliverables:

- typed node/edge records;
- immutable `GraphSnapshot`;
- canonical serializer and hash;
- indexes and memory accounting;
- atomic `GraphStore` publication; and
- LRU revision retention.

Red gate:

- graph invariant, determinism, index, cursor-binding, and eviction tests fail.

Green gate:

- equal graphs built in different insertion orders have identical hashes;
- invalid graphs cannot publish;
- concurrent immutable queries are race-free; and
- eviction respects pinned/current revisions.

### Phase 3 — Implement exact active-view capture

Deliverables:

- `MainThreadGateway`;
- `ActiveAssemblyResolver`;
- `AssemblyViewCapture`;
- capture guards and presentation revision; and
- injected resolver interfaces for headless tests.

Red gate:

- no-active-document/view/Assembly, foreign document, edit-mode, visibility,
  and mid-capture-change tests fail.

Green gate:

- resolver identifies only the exact active Assembly;
- view state is captured without mutation;
- all document/GUI reads are asserted on the main thread; and
- start/end guards reject mixed state.

### Phase 4 — Implement Assembly object adapters and graph builder

Deliverables:

- adapter registry with most-specific-first matching;
- adapters for every row in Section 9.2;
- fallback unknown-object adapter;
- definition/occurrence expansion;
- placement and visibility computation;
- joint/constraint extraction;
- source-document dependency tracking; and
- geometry summaries.

Red gate:

- add one failing fixture test for a type before implementing its adapter.

Green gate:

- the entire fixture matrix produces exact counts and identities;
- no supported component is omitted or double-counted;
- repeated links share definitions but retain distinct occurrences;
- rigid/flexible nested Assembly behavior is correct; and
- graph build is deterministic across save/reopen.

### Phase 5 — Register and execute `assembly.graph_snapshot`

Deliverables:

- provider-neutral registration in `cadx.graph`;
- typed argument parser and result serializer;
- main-thread capture plus off-thread graph finalization;
- atomic store publication;
- structured errors and cancellation; and
- no-op cache reuse for `refresh: "if_stale"`.

Red gate:

- contract, stale-cache, cancellation, mutation-detection, and resource-limit
  tests fail.

Green gate:

- the tool returns only a graph handle and bounded summary;
- repeated unchanged snapshots return the same content revision;
- changed documents publish a new revision;
- failed candidates do not replace the last valid graph; and
- document, selection, camera, visibility, and undo state remain unchanged.

### Phase 6 — Implement and register `assembly.graph_query`

Deliverables:

- typed parsers for all five query operations;
- indexed filters and deterministic traversals;
- revision-bound cursors;
- field projection and provenance; and
- result-size enforcement.

Red gate:

- operation, filter, traversal, stale-revision, pagination, and adversarial-bound
  tests fail.

Green gate:

- all fixture questions can be answered without document access;
- repeated queries are byte-for-byte deterministic;
- no query can exceed depth, count, time, or result-size bounds; and
- stale graphs cannot masquerade as current state.

### Phase 7 — Integrate the Ollama tool-call loop

Deliverables:

- provider-neutral turn item model;
- tool definitions in chat requests;
- streamed tool-call assembly;
- main-thread dispatch bridge;
- transcript tool-call/result persistence;
- continuation requests, cancellation, and call limits; and
- updated durable system prompt.

Red gate:

- fragmented tool-call, malformed JSON, unknown tool, cancellation, multiple
  call, and continuation tests fail using injected transports.

Green gate:

- an injected Ollama stream can call snapshot, query the returned graph, and
  finish with assistant text;
- existing plain text chat still works;
- model choice remains provider-agnostic; and
- network threads never touch FreeCAD objects directly.

### Phase 8 — Add synchronization and invalidation

Deliverables:

- App document observer;
- Gui presentation observer;
- source dependency reverse index;
- transaction/recompute coalescing;
- stale-state reporting; and
- close-document eviction.

Red gate:

- mutation, source change, undo/redo, abort, view change, and close-document
  lifecycle tests fail.

Green gate:

- every relevant change invalidates exactly the necessary semantic or
  presentation revision;
- aborted transactions do not publish phantom revisions;
- source edits invalidate parent graphs; and
- no observer callback mutates the graph currently being queried.

### Phase 9 — Performance and release hardening

Deliverables:

- benchmark fixtures for 100, 1,000, and 10,000 occurrences;
- timing broken down into view capture, document extraction, geometry summary,
  graph build, indexing, hashing, and query;
- UI heartbeat test during snapshot capture;
- memory-accounting and eviction evidence;
- fuzz/property tests for query schemas and malformed graphs; and
- user-visible diagnostics in the chat panel.

Initial targets, to be confirmed on representative hardware:

- 100-occurrence metadata snapshot under 100 ms;
- 1,000-occurrence metadata snapshot under 500 ms;
- indexed node/neighbor queries under 20 ms at 10,000 nodes;
- no GUI heartbeat gap over 100 ms during a normal bounded snapshot; and
- memory estimates within 10 percent of measured retained graph allocations.

Do not weaken identity, closure, or validation to hit a timing target. Profile
the first failing stage and optimize that stage.

## 19. Validation commands

Use a writable out-of-tree build directory. The exact preset and generator may
be adapted to the developer machine, but the PR must report the commands that
actually ran.

Pure Python baseline and provider tests:

```bash
python3 src/Mod/CadX/cadx_tests/run_all.py
```

Representative configure/build sequence:

```bash
cmake -S . -B /private/tmp/cadx-snapshot-build \
  -G Ninja \
  -DBUILD_CADX=ON \
  -DBUILD_ASSEMBLY=ON \
  -DBUILD_GUI=ON \
  -DBUILD_TESTING=ON
cmake --build /private/tmp/cadx-snapshot-build --target CadX CadXTests
```

Focused C++ and GUI tests after their targets exist:

```bash
ctest --test-dir /private/tmp/cadx-snapshot-build \
  --output-on-failure \
  -R 'CadX(Graph|Assembly|Tool)'
```

Repository hygiene:

```bash
git diff --check
git status --short
```

Final GUI acceptance must run the built FreeCAD application and prove:

1. open a fixture Assembly;
2. activate it in the Assembly workbench;
3. make only a subset of occurrences visible;
4. call `assembly.graph_snapshot` through the same dispatcher used by chat;
5. query visible parts, definitions, nested occurrences, and joints;
6. change a source part and verify the prior revision becomes stale;
7. snapshot again and verify the changed graph;
8. undo/redo and verify invalidation/rebuild;
9. save/reopen and verify source/occurrence identity and graph determinism; and
10. confirm selection, camera, edit mode, visibility, and document contents were
    never changed by either graph tool.

## 20. Stop conditions

Stop the implementation phase and resolve the design rather than proceeding if:

- a supported Assembly component form cannot be represented without conflating
  definition and occurrence;
- active Assembly identity cannot be read without changing GUI state;
- an external or nested source cannot be assigned an exact source key;
- generated AssemblyLink mirror objects cannot be distinguished from source
  definitions;
- a snapshot requires document mutation to create stable identity;
- any FreeCAD object or ViewProvider is accessed from the network/query worker;
- the graph silently omits an unsupported object or silently truncates a hard
  limit;
- graph revisions are nondeterministic across equivalent builds or save/reopen;
- stale graph queries return apparently current facts;
- a tool schema requires arbitrary code, unbounded properties, or an unbounded
  traversal; or
- the implementation would require removing or changing the existing chat API
  instead of adding a compatibility path.

## 21. Explicit non-goals for this implementation

- copying VibeCAD Native tool code or its runtime architecture;
- changing FreeCAD Assembly's document schema;
- persisting the graph in FCStd, SQLite, Neo4j, or a remote service;
- using screenshots or a vision model as the authoritative semantic extractor;
- reconstructing full BREP topology in the graph;
- inferring holes, fillets, fastener standards, materials, or product identity
  without exact evidence;
- mutation tools for creating or editing Assembly objects;
- incremental graph patching before full rebuild correctness is proven;
- supporting every FreeCAD workbench in the first graph schema; or
- hard-coding behavior for one Ollama model family.

## 22. Definition of done

The work is complete only when:

- both tools are registered through cad-x's own provider-neutral registry;
- the Ollama turn loop can call them end to end;
- the graph lives in a bounded in-memory C++ store;
- every Assembly part/object form in Section 9.2 is represented by fixture-backed
  definition, occurrence, source, placement, and presentation semantics;
- rigid/flexible nested Assemblies, link arrays, external sources, ordinary
  joints, grounding, and rigid groups are represented accurately;
- graph and presentation revisions are deterministic and stale-safe;
- queries are indexed, paginated, bounded, and document-independent;
- no snapshot or query changes the FreeCAD document or GUI state;
- source changes, transactions, undo/redo, save/reopen, and close-document
  lifecycle pass;
- exact build, unit, integration, GUI, and performance results are recorded; and
- the README current-status section is updated to distinguish implemented graph
  capability from remaining planned document mutation tools.
