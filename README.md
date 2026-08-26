# cad-x

An early prototype for an AI interface to FreeCAD.

The goal is to let a language model work with a real parametric CAD system
through a small, inspectable tool layer. This repository is a technical
prototype and demonstration for engineers. It is not a customer product yet.

## Design

The prototype has three parts; the current checkout contains the chat
transport, provider-neutral graph tools, and a native in-memory graph path:

1. **Exposed FreeCAD tools (future mutation work)** — operations the model can
   eventually call to create, inspect, modify, and validate a FreeCAD document.
2. **An in-memory model graph (implemented foundation)** — a bounded,
   revisioned representation of the active Assembly, including typed nodes,
   relationships, placements, provenance, and presentation state. The graph is
   exposed through `assembly.graph_snapshot` and `assembly.graph_query`.
3. **A language model** — the current chat adapter connects to a local Ollama
   server and can use any model available there. The selected model is chosen
   in the chat interface rather than being tied to a particular model family.

The current chat integration uses a local Ollama server and requires no
provider credentials. Model discovery and selection are handled by the chat
interface; no particular Ollama model family is required.

The graph tools are owned by an independent, provider-neutral `cadx.graph`
registry:

- `cadx.graph` registers `assembly.graph_snapshot` and
  `assembly.graph_query`.

The future `freecad.document` registry is reserved for document mutation tools;
it is not a dependency of the read-only graph path. The Ollama adapter converts
the graph registry’s definitions to its OpenAI-compatible request shape.

CadX is C++-first. Native application services, tool registries, and the
per-document graph belong in `src/Mod/CadX/App`; the Qt chat interface belongs
in `src/Mod/CadX/Gui`. Python is retained only as FreeCAD's module bootstrap
and as a future adapter boundary for Python workbenches and third-party tools.
Network work must not access FreeCAD documents directly from a worker thread;
document operations return to FreeCAD's main thread and use its transaction,
recompute, undo, and redo machinery. The graph remains a rebuildable projection
of each authoritative FreeCAD document.

```text
                         query graph
                    ┌──────────────────┐
                    │                  ▼
             ┌──────────────┐   ┌──────────────┐
             │ Language     │   │ In-memory    │
             │ model        │◄──│ model graph  │
             └──────┬───────┘   └──────┬───────┘
                    │ call tools       │ sync/inspect
                    ▼                  ▲
             ┌─────────────────────────┴┐
             │ Exposed FreeCAD tools    │
             └────────────┬─────────────┘
                          │
                          ▼
                    FreeCAD document
```

The model proposes actions. FreeCAD evaluates the parametric document, and
deterministic checks validate the result. The graph is a reasoning and
interaction layer. It is not a replacement for the FreeCAD document or its
parametric history.

## Example interaction

A model could receive a request such as:

> Create an 80 × 50 × 6 mm mounting plate with four M4 clearance holes and
> 8 mm edge offsets.

It would query the graph, call typed FreeCAD tools, inspect the updated model,
and validate the resulting dimensions and constraints.

## Prototype goals

- expose a small, understandable set of FreeCAD operations;
- keep the model’s view of the document queryable and synchronized;
- support inspect → act → validate loops;
- preserve parametric, editable FreeCAD models;
- make tool calls and failures easy to inspect and reproduce.

## Current status

The implemented path can register both graph tools, capture the exact active
Assembly from the FreeCAD GUI when one is available, publish an immutable
bounded C++ graph, and run deterministic summary, filter, neighbor, subgraph,
and shortest-path queries. The Ollama turn loop supports fragmented streamed
tool calls, bounded continuations, cancellation, and transcript persistence.

The remaining work is fixture-complete Assembly expansion (link arrays,
nested/flexible links, joints, datums, and geometry summaries), native document
and presentation observer wiring, cursor pagination, and final GUI lifecycle
acceptance. The graph tools are read-only; document mutation tools remain
planned. This repository does not require or assume a particular Ollama model
family.

### Graph evidence and audit trail

Set `CADX_GRAPH_AUDIT_LOG` to write one flushed JSON object per line for each
build, evidence round-trip, publish, and query checkpoint. Each graph-bearing
event records the graph revision, presentation revision, semantic/presentation
hashes, node and edge counts, and any failure code or diagnostic.

```bash
CADX_GRAPH_AUDIT_LOG=/private/tmp/cadx-graph.jsonl \
  FREECAD_USER_HOME=/private/tmp/cadx-user \
  build/debug/bin/FreeCAD

python3 src/Mod/CadX/CadXGraphAudit.py /private/tmp/cadx-graph.jsonl
```

For a full retained snapshot, call the non-provider debug API with the exact
handle and revision returned by `assembly.graph_snapshot`:

```python
import CadXApp
evidence_json = CadXApp.graph_evidence(graph_id, graph_revision)
```

That JSON is a lossless `cadx.assembly-graph-snapshot.v1` evidence record and
is checked by the native decoder before publication. It can reconstruct the
same immutable graph and revisions, but it does not mutate FreeCAD or replace
the authoritative parametric document. Live graph-to-CAD mutation remains an
explicit future phase.

## Non-goals for now

- supporting every CAD workflow;
- replacing FreeCAD’s geometry kernel or parametric system;
- claiming production reliability or customer adoption;
- training a custom foundation model before the tool and graph interfaces are
  useful.
