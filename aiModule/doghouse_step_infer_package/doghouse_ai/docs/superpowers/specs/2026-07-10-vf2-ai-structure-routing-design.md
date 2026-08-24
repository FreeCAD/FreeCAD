# VF2 / AI Structure Routing Design

**Date:** 2026-07-10  
**Status:** Implemented and evaluated

## Goal

When fastener hole walls have **no analytic radius** (bspline/freeform), rule/VF2 paths fail and mount faces are wrong. Use AI **structure roles** (mount + through-hole wall) from geometric/topological context as fallback.

When VF2 parses round holes, keep the rule path only if the analytic hole is also supported by AI structure probabilities. This prevents unrelated analytic cylinders from stealing the mount, as seen in COMPOUND009.

## Routing

```
doghouse scope (existing FaceGraphGNN instances)
        │
        ▼
  VF2 / analytic round-hole path
        │
   ┌────┴────────────────────────────┐
   │ reliable analytic hole           │ no reliable analytic hole
   │ + AI structure support           │ or low structure support
   ▼                                  ▼
rule mount+hole scoring              AI structure path
(VF2 score)                          (no radius required)
```

### Reliable analytic hole (rule wins)

VF2 (or geometry cluster) returns ≥1 hole group where **at least one wall face** is `cylinder`/`cone` with parseable `R ∈ [min_radius, max_radius]`. Pure freeform / `axis_source=freeform_pca` without analytic radius does **not** count.

In the implemented gate, analytic radius alone is not enough. If structure probabilities are available, VF2 also requires at least one of:

- selected mount face has `mount_prob >= 0.35`
- selected hole group contains a face with `hole_wall_prob >= 0.35`

### AI structure path (fallback)

Inside the doghouse instance:

1. **Mount:** high `mount_prob`, plane, area in mount band, prefer small outer-support margin (outermost).
2. **Hole walls:** high `through_hole_wall_prob` (`hole_wall_prob`), faces that reach the chosen mount in ≤2 hops, allowing `transition`-like bridges (any non-plane neighbor in scope).
3. Cluster connected hole-wall faces into one group; **do not require** parseable radius. Optional soft radius from edge circles when available; otherwise emit hole group with `radius=null` / `source=ai_structure` and status may be `ok` if mount+walls exist, or `mount_only` if walls empty.

AI must **not** override a VF2 result that already has reliable analytic holes **and** structure support. VF2 results with analytic but semantically unsupported cylinders are routed to AI structure.

## Model

Keep doghouse `node_head` / `edge_head`.

Add / keep binary heads on shared encoder:

| Head | Label source | Meaning |
|------|----------------|---------|
| `mount_head` | `face_semantic == mount` | outer mount face |
| `hole_wall_head` | `face_semantic == hole_wall` | through-hole wall (context, not radius) |

v1: transition is **not** a separate head; short-hop adjacency uses geometry types as bridges.

Inference writes `mount_prob` and `hole_wall_prob` on each face prediction. The production pipeline can use a dual-checkpoint setup: the production checkpoint provides doghouse instances, while a structure checkpoint supplies only `mount_prob` / `hole_wall_prob`.

## Training

- Reuse graph npz `face_semantic`.
- Losses: existing node/edge (+ optional semantic) + `mount_loss_weight * BCE(mount)` + `hole_wall_loss_weight * BCE(hole_wall)` with pos_weight from class imbalance.
- Checkpoint flags: `mount_head`, `hole_wall_head`.

## Acceptance / Current Result

Source: `structure_s200/eval_all8_dual_gate/eval_all8_dual_gate_checked_summary.json`.

| Model | Result |
|-------|--------|
| B pillar trim lower-0612 | mount 5/5, hole 5/5, all `vf2_topo` |
| M5-5402231 | mount 4/4, hole 7/7, all `vf2_topo` |
| pillar | mount 5/5, hole 15/15, all `vf2_topo` |
| 0981 | mount 5/5, hole TP 15/18 |
| 1031001001-B001 | mount 4/4, hole TP 9/9, FP 17 |
| COMPOUND009 | mount 4/4, hole TP 12/12, FP 17 |
| B126302102001-B12 | mount 2/2, hole TP 3/7 |
| B126302301001-B12 | mount 5/5, hole TP 5/5, FP 7 |

Overall mount result: 34/34 with 0 FP and 0 FN.

## Non-goals (v1)

- Root-background “大面” parallel gate
- Replacing doghouse instance segmentation
- Full multi-class structure softmax
- Perfect hole-wall precision in AI structure path; current v1 prioritizes mount correctness and hole recall, with known FP around transition faces.
