# cad-x

An early prototype for an AI interface to FreeCAD.

The goal is to let a language model work with a real parametric CAD system
through a small, inspectable tool layer. This repository is a technical
prototype and demonstration for engineers. It is not a customer product yet.

## Design

The planned system has three parts:

1. **Exposed FreeCAD tools** — operations the model can call to create,
   inspect, modify, and validate a FreeCAD document.
2. **An in-memory model graph** — a queryable representation of the current
   FreeCAD model, including objects, relationships, parameters, and useful
   geometric or semantic information. The graph is exposed through tools so the
   model can inspect context before changing the document.
3. **A language model** — initially `gpt-luna` as the planning and interaction
   layer. The longer-term direction is a custom post-trained Qwen 27B model,
   using the Markov AI CAD dataset on Hugging Face as one training resource.

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

This is an early-stage plan and prototype. Tool schemas, graph structure, and
model choices will change as the first workflows are built. The custom Qwen
training path is future work; this repository does not claim that model has
already been trained or evaluated.

## Non-goals for now

- supporting every CAD workflow;
- replacing FreeCAD’s geometry kernel or parametric system;
- claiming production reliability or customer adoption;
- training a custom foundation model before the tool and graph interfaces are
  useful.
