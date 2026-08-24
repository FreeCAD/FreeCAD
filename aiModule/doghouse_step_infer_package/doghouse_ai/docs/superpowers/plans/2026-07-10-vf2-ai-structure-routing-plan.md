# VF2 / AI Structure Routing Implementation Plan

> **Status:** Completed. This plan is kept as implementation history and regression checklist.

**Goal:** Route assembly to VF2 when reliable analytic round holes exist; otherwise select mount + through-hole walls from AI structure probs without requiring radius.

**Architecture:** Extend FaceGraphGNN with `mount_head` + existing `hole_wall_head`; wire probs into predictions; in `extract_assembly_features`, try VF2 first and fall back to `_ai_structure_mount_and_holes` when VF2 lacks reliable analytic holes or analytic holes are not supported by AI structure probabilities.

**Tech Stack:** PyTorch FaceGraphGNN, PythonOCC assembly features, unittest.

---

## File map

| File | Change |
|------|--------|
| `graph_model.py` | Add `mount_head` parallel to `hole_wall_head` |
| `train_graph.py` | Mount BCE loss + checkpoint flags |
| `infer_graph.py` | Emit `mount_prob` / keep `hole_wall_prob` |
| `infer_from_step.py` | Pass thresholds; assembly kwargs |
| `doghouse_assembly_features.py` | Reliable-hole check + AI structure fallback path |
| `test_ai_structure_routing.py` | Unit tests for routing and AI path |

---

## Tasks

### Task 1: Model + train + infer probs

- [x] Add `mount_head` to `FaceGraphGNN`; return mount logits from `forward_all`
- [x] Train: `--mount-loss-weight`; save `mount_head` in ckpt
- [x] Infer: write `mount_prob` on face rows
- [x] Support dual-checkpoint inference: production checkpoint for doghouse instances, structure checkpoint for `mount_prob` / `hole_wall_prob`

### Task 2: Assembly routing

- [x] `_vf2_has_reliable_analytic_hole(holes, faces, min_r, max_r)`
- [x] `_vf2_supported_by_ai_structure(...)`: require `mount_prob` or `hole_wall_prob` support before VF2 can win
- [x] `_ai_structure_mount_and_holes(...)` using `mount_prob` + `hole_wall_prob`
- [x] In `extract_assembly_features`: VF2 → if reliable and structure-supported keep; else AI structure; else geometry fallback
- [x] AI never overrides reliable VF2 with structure support

### Task 3: Tests + retrain/eval

- [x] Unit tests for routing
- [x] Train/evaluate structure heads with s200 data
- [x] Retest 8 models: B pillar, M5, pillar, 0981, B001, COMPOUND009, B12-102001, B12-301001

---

## Final result

Source: `structure_s200/eval_all8_dual_gate/eval_all8_dual_gate_checked_summary.json`.

- Mount faces: `34/34`, FP=`0`, FN=`0`.
- `pillar`: remains `vf2_topo`, mount `5/5`, hole `15/15`.
- `M5-5402231`: mount `4/4`, hole `7/7`.
- `COMPOUND009`: routed to `ai_structure`, mount fixed to `4/4`; false analytic cylinders `[738,739]` / `[1024,1025]` no longer win.
- Remaining issue: AI structure hole-wall components have high recall but include transition-face FP.
