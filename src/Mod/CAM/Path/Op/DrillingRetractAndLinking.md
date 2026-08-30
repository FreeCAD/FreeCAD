# Drilling: Retract Heights & Linking — How It All Fits Together

Reference notes for `Path/Op/Drilling.py` (`_executeDrilling`), written while restoring
Peck Retract for FreeCAD/FreeCAD#32201. Covers every height/flag involved in a
drilling operation and how they interact, including the inter-hole linking/collision
logic.

Not committed — working notes only.

> **A note on names before anything else.** From here on, this doc always spells out
> the general clearance-adjacent travel plane as **RetractHeight**, and the
> peck-specific plane as **Peck Retract** — it never uses the bare word "SafeHeight."
> In the actual FreeCAD source, the property literally named `obj.SafeHeight` is what
> this doc calls RetractHeight — that's the one translation to keep straight. The
> peck-specific property is literally named `obj.PeckRetract` in code (older documents
> may have it under its former name, `obj.RetractHeight`; a `renameProperty` migration
> in `opOnDocumentRestored` handles that automatically, specifically because that old
> name was too easily confused with `obj.SafeHeight` — see the migration note in
> `Drilling.py:161`), so for `PeckRetract` the doc's term and the code identifier now match and need no
> translation. The table right below makes both mappings explicit; everything after it
> is written entirely in the doc's terms.

## 1. The properties involved

| Property (actual code identifier) | This doc calls it | UI label | Meaning | Typical value |
|---|---|---|---|---|
| `ClearanceHeight` | ClearanceHeight | Clearance | Top rapid plane. Tool starts here, framework forces a rapid back here at the very end of the op ("just for safety" — `Base.py:1236`). Nothing below this is assumed obstruction-free. | highest |
| `SafeHeight` | **RetractHeight** | Retract | General rapid-move plane for traveling between features. Clamped so it never exceeds ClearanceHeight (`Drilling.py:339-345`, with a warning if it does). | below Clearance, above stock |
| `StartDepth` | StartDepth | Start Depth | Top of the hole target — where the drilling edge begins (`v1` in `Drilling.py:357`). **Not** a retract governor (that was the #32201 regression — R used to be hard-forced to this). | at/near stock top |
| `FinalDepth` | FinalDepth | Final Depth | Bottom of the hole. | lowest |
| `PeckRetract` | **Peck Retract** | Peck Retract | The G8x "R" parameter — where a *peck* cycle retracts to between pecks, and (with `KeepToolDown`/G99) between holes. Only used at all when `PeckEnabled` is on. | defaults to RetractHeight (i.e. `obj.SafeHeight`), but can be set anywhere, including below StartDepth |

RetractHeight and Peck Retract are **not the same thing** even though both loosely mean
"retract" — RetractHeight's UI label is literally "Retract" (see `PageHeightsEdit.ui`),
which is exactly why the new field is labeled "Peck Retract" instead of "Retract Height":
there'd be two differently-scoped things both called "Retract Height" in the same UI.

There's also a `CollisionAvoidanceStrategy` enum option literally named **"Retract
Height"** (`Base.py:243,251`) — that one refers to RetractHeight (code: `obj.SafeHeight`)
too, not to Peck Retract (code: `obj.PeckRetract`). Three different "retract"
concepts in play, two of which land on the same code property (`obj.SafeHeight`), one of
which is the peck-specific property. This is the confusion worth keeping straight
while reading the rest of this doc — and exactly why the box above exists.

## 2. The two flags

- **`PeckEnabled`** — turns pecking on (`Q` parameter, `G73`/`G83` instead of
  `G81`/`G82`/`G85`), and is the *only* thing that makes `Peck Retract` matter at all
  (`Drilling.py:463`):
  ```python
  peck_retract = obj.PeckRetract.Value if obj.PeckEnabled else safe_height
  ```
  Off → every cycle behaves exactly as it did before `Peck Retract` existed: R is
  `RetractHeight`, full stop.

- **`KeepToolDown`** — toggles `G98`/`G99` (`Drilling.py:336`). This is a *completely
  separate axis* from pecking: it governs where the tool ends up **after a hole's
  cycle finishes**, i.e. before moving to the next hole (or, for the last hole, before
  the framework's final rapid to `ClearanceHeight`).
  - `G98` (`KeepToolDown` off, default/safest): retract to the initial level — wherever
    the tool was *before* this hole's cycle started (`z_before_cycle`, captured at
    `Drilling.py:466`). In this codebase that's always `RetractHeight` by the time the
    cycle starts (see §4), so G98 always means "fully back to `RetractHeight` between
    every hole," regardless of pecking or `Peck Retract`.
  - `G99` (`KeepToolDown` on): retract only to R. If not pecking, R = `RetractHeight`, so
    G98 and G99 behave identically. If pecking, R = `Peck Retract`, which is where
    G99 actually starts to matter — the tool can stay down near the bottom of a deep
    hole instead of fully retracting, which is the entire point of #32201 (avoid
    repeated full retracts with a long, thin bit).

**`KeepToolDown` affects Tapping too** (`_executeTapping` uses the same `mode` toggle),
but Tapping's R is still hardcoded to `obj.StartDepth.Value` — the `Peck Retract` fix
was scoped to the Drilling strategy only, since Tapping never pecks in this codebase.

## 3. What actually happens inside one `G83`/`G73` cycle

This part is handled entirely by the controller from a single G-code word — we never
emit per-peck commands. Per LinuxCNC/NIST:

- **Every intermediate peck** retracts fully to R. That's what defines `G83` (full
  chip-clearing retract) as opposed to `G73` (small chip-breaking retract, doesn't go
  all the way to R each time).
- **The final retract**, once the hole is finished, honors `G98`/`G99` as described
  above — this is the only place those two flags matter *within* a single hole; R
  governs every peck retract regardless of `KeepToolDown`.

## 4. Per-hole algorithm walkthrough (`_executeDrilling`)

```
Start:            rapid to ClearanceHeight                          (Drilling.py:396-397)
First hole:       rapid X/Y, then rapid Z to RetractHeight              (Drilling.py:412-419)
                  → run cycle (see below)
Each next hole:
  1. current_pos = wherever bookkeeping says the tool is             (Drilling.py:430)
     (G98 last hole → RetractHeight; G99 last hole → Peck Retract or RetractHeight if not pecking)
  2. NEW (the fix from this session): if current_pos.z < RetractHeight,
     climb explicitly to RetractHeight FIRST                            (Drilling.py:431-437)
  3. Collision check: is a straight move at RetractHeight from here to
     the next hole's X/Y clear?                                      (Drilling.py:439-443)
       - clear (≤2 edges) → no extra commands, modal cycle continues
       - not clear (>2 edges) → emit the full retract/traverse/plunge
         moves linking.get_linking_moves() built                    (Drilling.py:448-452)
  4. Run the cycle for this hole (R = Peck Retract if pecking else
     RetractHeight)                                                     (Drilling.py:463-477)
  5. Update bookkeeping for the NEXT iteration:
       G98 → z_before_cycle (captured just before step 4, which by
             now is always RetractHeight thanks to step 2/3)
       G99 → peck_retract (Peck Retract if pecking, else RetractHeight)
                                                                      (Drilling.py:489-495)
End:              framework appends a final rapid to ClearanceHeight  (Base.py:1236)
```

**Why step 2 exists**: without it, a "clear at RetractHeight" verdict in step 3 would be
answering the wrong question whenever the tool is actually sitting lower than
`RetractHeight` (G99 + peck + low `Peck Retract`). No commands get emitted in that
"clear" branch, so the *real* machine motion — the modal G99 cycle just continuing with
new X/Y — happens at whatever height the tool is actually at (R), not at the height the
check evaluated. That mismatch is what produced the crossing lines through the stock in
the screenshot from earlier in this conversation, and is also why the
`CollisionAvoidanceStrategy = "Retract Height"` option (documented as "uses safe height
for rapid moves") was silently not doing that. Step 2 makes the check honest by forcing
the assumption (tool is at `RetractHeight`) to be true before the check ever runs.

**Why grouping still works**: the post-processor's `cannedCycleTerminator`
(`Path/Post/Utils.py:355`) treats *any* non-drill command as a reason to insert `G80`
before it and start a fresh cycle after. So whenever step 2 or step 3 inserts explicit
`G0` moves, the modal group is automatically and correctly broken there — no special
handling needed for "grouping consecutive holes" beyond what already existed.

## 5. Linking / collision-avoidance layer

`CollisionAvoidanceStrategy` (a job/op-level enum, `Base.py:235-251`) changes what
`linking.get_linking_moves()` is allowed to use when it needs to route around
something:

| Strategy | Effect on `heights_clearance` / other args (`Drilling.py:364-384`) |
|---|---|
| Clearance Height | `heights_clearance = ClearanceHeight` only — always climbs all the way up if a detour is needed |
| **Retract Height** | leaves the default `(RetractHeight, ClearanceHeight)` — tries `RetractHeight` first, falls back to `ClearanceHeight`. (Despite the name, this is `RetractHeight`, not `Peck Retract`.) |
| Line of Sight | adds `solids` so `get_linking_moves` actually checks geometry, not just height |
| Tool Diameter | adds `solids` + `tool_diameter`, sweeps a flat strip the width of the tool for the check |
| Tool Shape | adds `solids` + the tool's real 3D shape for the check |

`get_linking_moves()` (`Path/Base/Generator/linking.py:95`) tries each candidate height
in ascending order, builds a retract→traverse→plunge wire (`make_linking_wire`), and
returns the first one whose horizontal traverse segment doesn't collide
(`is_travel_collision_free`) — only the horizontal top segment is checked; the vertical
retract/plunge segments at each hole's own X/Y are assumed clear (reasonable, since
that's straight up out of a hole that's already been drilled). If nothing at any
candidate height is clear, it raises.

Two different thresholds matter in `_executeDrilling`, easy to conflate:

1. **Step 2's `< safe_height` check** — decides whether an explicit climb is needed at
   all before even running the collision check. Purely about the tool's *current* Z vs.
   `RetractHeight`; no geometry involved.
2. **Step 3's `len(linking_moves) > 2` check** — decides whether the *collision-free*
   path at `RetractHeight` needed extra height (a "cannot traverse at `RetractHeight`, must
   detour higher" verdict). Purely about geometry vs. the candidate heights above.

## 6. Case matrix

All cases assume the default/common setup: `ClearanceHeight > RetractHeight > StartDepth >
FinalDepth`, `RetractHeight` not clamped down to `ClearanceHeight`.

### 6a. Peck off (`PeckEnabled = False`)

`Peck Retract` is completely ignored — `peck_retract` is forced to `RetractHeight`
(`Drilling.py:463`) no matter what value the (now-dimmed) Peck Retract field holds.

| `KeepToolDown` | R used in G81/G82/G85 | Between-hole retract | Notes |
|---|---|---|---|
| Off (G98) | `RetractHeight` | Full retract to `RetractHeight` | same as before #32201 existed |
| On (G99) | `RetractHeight` | Retract to `RetractHeight` | **behaves identically to G98** here — with no peck, R and the G98 initial level both land on `RetractHeight`, so the flag has no practical effect |

### 6b. Peck on (`PeckEnabled = True`), varying where `Peck Retract` sits

| `Peck Retract` position | Intra-hole peck retracts | `KeepToolDown` off (G98) | `KeepToolDown` on (G99) |
|---|---|---|---|
| **Above `RetractHeight`** | every peck retracts above `RetractHeight` — extra travel, no benefit; not clamped anywhere in code, so it's on the user | between-hole: full retract to `RetractHeight` anyway (G98 always uses the initial level, not R) | between-hole: retract to R (above `RetractHeight`!) → §4 step 2 sees `current_pos.z >= RetractHeight`, so *no* forced climb, but this is already higher than needed |
| **= `RetractHeight`** (the default) | pecks retract to `RetractHeight` — behaves like a normal safe retract every peck | full retract to `RetractHeight` | retract to `RetractHeight` — same as G98 in this case, no regression risk |
| **Between `StartDepth` and `RetractHeight`** | pecks retract partway up, still above the hole's start | full retract to `RetractHeight` between holes (peck savings only *within* the hole) | retract only partway between holes too — §4 step 2 forces a climb to `RetractHeight` before evaluating/making the move to the next hole |
| **= `StartDepth`** | pecks retract to the top of the hole (this was the *forced, only* behavior before the fix — R was hardcoded to `StartDepth`) | full retract to `RetractHeight` between holes | retract only to `StartDepth` between holes → forced climb (step 2) before moving on |
| **Below `StartDepth`, above `FinalDepth`** (the deep-hole/#32201 case) | pecks only back off partway *into* the material — no full clear of the hole, less bit wobble/re-entry risk | full retract to `RetractHeight` between holes regardless (G98 always ignores R for the between-hole move) | tool stays down near the bottom between pecks; §4 step 2 **always** forces an explicit climb to `RetractHeight` before the linking check, so it still can't drag the tool through stock going to the next hole — you get the deep-retract benefit *within* the hole, and the safety climb *between* holes |
| **At or below `FinalDepth`** | degenerate — R would be at/past the bottom of the hole, so the "retract" isn't really a retract; not validated/blocked anywhere in code today | same as above, full retract to `RetractHeight` regardless | same forced climb applies before the next hole, so it's not unsafe, but the peck cycle itself is nonsensical — worth a warning in the UI/validation at some point (not implemented) |

### 6c. `RetractHeight` clamp interaction

If `RetractHeight > ClearanceHeight`, the code silently uses `ClearanceHeight` in its place
for that run (`Drilling.py:339-345`, with a `Path.Log.warning`). Every "`RetractHeight`" in
the tables above should be read as this clamped value. `Peck Retract` itself is **not**
clamped against anything — it can be set above `ClearanceHeight`, below `FinalDepth`,
wherever.

## 7. tl;dr

- Not pecking → `KeepToolDown` is a no-op for retract height (both land on
  `RetractHeight`); it still matters for the `G98`/`G99` word emitted (post-processor may
  care) but the physical height is the same either way.
- Pecking + `KeepToolDown` off → `Peck Retract` only affects travel *inside* one hole's
  peck cycle; between holes it's always a full retract to `RetractHeight`.
- Pecking + `KeepToolDown` on + `Peck Retract` below `RetractHeight` → this is the
  interesting/valuable case from #32201: less travel during pecking *and* between holes
  that are close enough not to need a full retract — but the tool is guaranteed to climb
  to `RetractHeight` before any inter-hole move is attempted, so it can't be dragged through
  stock to get there.
- `Peck Retract` is never validated against `StartDepth`/`FinalDepth`/`ClearanceHeight`
  — the user has full control, per the original issue's request, including values that
  don't make physical sense. Nothing currently warns about that.
