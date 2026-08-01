# Feeds & Speeds

## STATUS

PROPOSED

## Why it is a priority

Feeds & speeds (F&S) are the most user-visible daily pain point in the CAM
workbench. Three distinct proposals have been open for months — #21817 (full
data-model extension), #16801 (Material `Machinability` model extensions for
chipload and per-tool-material `Vc`), and #28419 (persist F&S on the tool so
it isn't re-entered per job). None have landed because they each address
only one layer of the problem and don't compose with each other.

Concretely, users today must:

- Enter Horizontal Feed, Vertical Feed, and Spindle Speed manually for every
  Tool Controller in every job, even when the same tool is reused across
  jobs against the same stock material.
- Either ignore FreeCAD's existing Material `Machinability` model (the data
  sits there unused by the CAM workbench) or maintain an external
  spreadsheet / calculator / addon.
- Accept a `ToolBit.Material` enum on every tool that is stored but never
  consulted by any operation or generator.

The CAM Big Board treats this as a perennial gap. The external
`FeedsAndSpeeds` addon (maintained by @dubstar-04) demonstrates that users
want in-workbench F&S computation, but it is not integrated into the
operation / tool-controller flow.

This epic covers the architectural work to close the gap durably, phased so
that each phase ships a standalone improvement while building toward a
testable and extensible F&S module.

## Background / current state

- **ToolController** carries `HorizFeed`, `VertFeed`, `SpindleSpeed`,
  `RampFeed`, `LeadInFeed`, `LeadOutFeed`, `HorizRapid`, `VertRapid` as plain
  user-entered values.
- **ToolBit** carries geometry plus a `Material` enum (HSS / Carbide) that is
  not read by any operation or generator.
- **Machine** model carries `max_power_kw`, `min_rpm`, `max_rpm` on the
  spindle; no torque curve, no rigidity, no feed limits.
- **FreeCAD Material workbench** defines a `Machinability` model used by
  other workbenches; CAM does not consult it.
- **Operations** read F&S directly from the assigned Tool Controller on each
  `opExecute`. No per-motion F&S computation exists outside a small number of
  dressup-specific fields.
- **CAMAssets** (the typed, URI-addressable asset store used for toolbits,
  libraries, shapes, machines) is in place and production-ready.
- **External addons** provide partial solutions: `FeedsAndSpeeds` (four-formula
  calculator) and a prior BTL calculator (chipload tables + iterative
  optimizer over deflection, torque, rigidity). Neither is integrated with
  the Tool Controller or operation flow.

## Approaches considered

Four architectural approaches were evaluated before settling on the one
below. They are recorded here so the community can validate the choice.

### (a) Persistence only — store F&S on the ToolBit

Adds feed and speed fields to `ToolBit`. Closes #28419 trivially.

| Pros | Cons |
| --- | --- |
| Smallest change; 1–2 PRs | Does not address the chipload / SFM gap |
| Immediate UX relief | No computation, no suggestion, no materials awareness |
| Easy to test | Does not compose with #16801 or #21817 |
| | Each future phase would rework this layer |

### (b) Table lookup — extend `Machinability` material model

Extends FreeCAD's `Machinability` material model with per-tool-material `Vc`
and `Fz` curves, as proposed in #16801. Operations compute RPM from SFM and
feed from chipload × flutes × RPM.

| Pros | Cons |
| --- | --- |
| Leverages existing FreeCAD Material workbench | Tool-specific overrides (vendor-published coated carbide that runs faster than generic carbide) are awkward to express |
| Tractable scope; formulas are well-understood | Per-op-type variation (finishing vs roughing) needs an extra axis |
| Per-material reuse across many tools | No provenance for where a value came from |
| | Physics-aware de-rating (rigidity, deflection) has no natural home |

### (c) Physics-based optimizer — full data model

A full machine/spindle/chuck/setup data model with an iterative optimizer
that considers rigidity, deflection, torque curve, power, and chip-thinning.
Mirrors the approach of the prior BTL calculator and the proposal in #21817.

| Pros | Cons |
| --- | --- |
| Highest-quality suggestions under ideal inputs | Requires substantial data entry (machine rigidity, torque curve, chuck stickout) from every user |
| Aligns with industrial CAM practice (iMachining, HSMAdvisor) | "Mathematical optimum ≠ practical optimum" — community pushback on treating F&S as deterministic |
| Enables material-removal-rate optimization | Research-project scope; unclear delivery timeline |
| | Cannot be shipped without also shipping simpler fallback |

### (d) Resolver + providers + rule registry (chosen)

A pure-function `FeedSpeedResolver` with a chain of pluggable providers.
Providers are consulted in order; the most specific match wins; a formula
fallback guarantees a result. Phase-2 work adds provider implementations.
Phase-3 work bolts a physics-aware optimizer on as a post-stage without
changing the resolver signature.

| Pros | Cons |
| --- | --- |
| Unit-testable as a pure function with injected providers | More architectural effort up-front than (a) |
| Accommodates (a), (b), and (c) as phases without data migrations | Phase-1 user-visible deliverable is smaller than (a) alone |
| Rule registry becomes a CAMAssets asset type — extensible by addons and vendor catalog packs | Requires a provenance model to explain "where did this suggestion come from" |
| Opt-in, user-confirmed; matches "user has final say" philosophy | |
| Leaves room for each future provider or stage without schema change | |

**Approach (d) is the chosen direction.**

## Proposed architecture

- `Path/Tool/FeedsSpeeds/Resolver.py`: pure function
  `resolve(tool, material, op_type, machine, setup) → FeedSpeedResult`.
  The resolver is GUI-independent and FreeCAD-Document-independent at the
  boundary; it can be unit-tested from fixtures.
- **Provider protocol.** Each provider implements a `suggest(context) ->
  PartialResult | None`. Providers are consulted in priority order:
  1. `ToolPresetProvider` — reads presets stored on the `ToolBit`.
  2. `RuleRegistryProvider` — reads `MachiningRule` assets from CAMAssets.
  3. `MaterialMachinabilityProvider` — reads the stock material's
     `Machinability` data.
  4. `FormulaProvider` — computes from fundamental relations
     (`rpm = sfm × 1000 / (π × d)`, `feed = rpm × flutes × chipload`,
     `chipload = d / divisor` as last-resort fallback).
- **`FeedSpeedResult`** carries `feed`, `speed`, `chipload`, `source` (a
  CAMAssets URI or a literal like `"formula:fallback"`), `confidence`, and
  a list of `warnings`. The data model is machine-type-neutral — a
  `parameters: dict[str, Quantity]` escape hatch accommodates
  turning / laser / plasma without schema change when those machine types
  are brought in scope later.
- **ToolController gains three properties:**
  - `OpTypeHint` (enum) — "what op category is this TC intended for"
  - `MaterialHint` (link or name) — "what material is this TC intended for"
  - `FeedSpeedProvenance` (`App::PropertyMap`) — per-field source tracking:
    `"user"` | `<asset-uri>` | empty. Resolver never overwrites fields
    whose provenance is `"user"`.
- **Operations are unchanged.** They continue to read `TC.HorizFeed`,
  `TC.VertFeed`, `TC.SpindleSpeed` as today. The resolver writes to these
  properties; the provenance map records the source.
- **Rules as CAMAssets.** A `MachiningRule` is a new asset type:
  `condition` (tool id / op type / tool material / coating / workpiece
  material — any field optional; `None` matches all) and `result`
  (feed / speed / chipload — optionally formulaic, e.g.
  `chipload = {tool.diameter}/160`). Rules can be packaged into rulesets
  and shipped by addons (Sandvik, Kennametal, user-contributed, etc.).

## Phasing

### Phase 1 — "Tool and TC remember their own F&S"

Closes #28419. Ships the resolver skeleton, one concrete provider, and the
TC data shape needed by later phases.

- `FeedSpeedResolver` module with provider protocol.
- `ToolPresetProvider` + `ToolBit.presets` storage (list of
  `(material_hint?, op_type_hint?) → values`).
- ToolBit editor: Presets tab.
- Tool Controller editor: Suggest F&S button (per-TC), OpTypeHint and
  MaterialHint fields, FeedSpeedProvenance map property, inline provenance
  badges next to feed/speed fields.
- Suggest dialog: confirm-before-apply (current vs suggested, source,
  warnings). If the TC is referenced by ops and the suggested values diverge
  from current, offer to clone the TC and attach the clone to the relevant
  op instead of overwriting.
- Sanity rule: flag any TC or operation whose F&S are unset.
- Unit test harness for the resolver with fixture tools, fixture materials,
  fixture rules; no FreeCAD-Gui dependency.

### Phase 2 — "Tool + material suggests F&S"

Closes #16801 and the bulk of #21817.

- `RuleRegistryProvider` — rules as CAMAssets; `condition → result` matching;
  specificity ranking; optional formulaic `result` expressions.
- `MaterialMachinabilityProvider` — reads extended `Machinability` data
  (coordinate with #16801). Per-tool-material SFM and chipload curves.
- `FormulaProvider` — four-formula fallback (coordinate with the external
  `FeedsAndSpeeds` addon; absorb/share the calculator core).
- Sanity rule: warn when a TC's F&S diverge from what the resolver would
  suggest for the current op's context.
- Optional standalone F&S Explorer panel — design-time tool to explore
  rules and suggestions, analogous to the external addon UI.
- Ruleset import/export for addon distribution.

### Phase 3 — "Physics-aware optimization"

Frames out the optimizer. Ships a linear optimizer stub; tuned optimization
is follow-up work outside this epic.

- `Setup` (in-job): references Machine + Chuck; carries stickout.
- `Spindle.torque_curve` — simple linear curve definition initially.
- `Machine.rigidity` — per-axis deflection-under-load.
- `Optimizer` post-stage: takes a Phase-2 `FeedSpeedResult` plus Setup,
  returns a constrained result that respects power / torque / rigidity /
  deflection. Opt-in, user-triggered.
- Per-op context awareness in the optimizer (engagement, effective stickout)
  — data structure only; consumer logic is follow-up work.

## Data locations

| Data | Home | Referenced from job by |
| --- | --- | --- |
| Tool presets | ToolBit file (`.fctb`) | Tool reference |
| `MachiningRule` assets | CAMAssets (user + builtin + addon stores) | Consulted globally at resolve time |
| Material `Machinability` data | FreeCAD Material file (`.FCMat`) | Job's Stock material |
| Machine, Spindle, Chuck | CAMAssets / `.fcm` | Job's Machine reference |
| Setup (stickout, which chuck) | Inside the job | — |
| `FeedSpeedProvenance` per-field source | On the Tool Controller | — |

The .FCStd job file gains no new top-level schema. The ToolController gains
three properties (`OpTypeHint`, `MaterialHint`, `FeedSpeedProvenance`); all
three are lazy-added on first use so existing jobs open unchanged.

## Scope

| In | Out |
| --- | --- |
| `FeedSpeedResolver` module — pure function, unit-testable | Turning / lathe F&S (data model accommodates, implementation deferred) |
| Provider protocol + chain | Laser / plasma / waterjet / WEDM F&S |
| `ToolPresetProvider` + presets on `ToolBit` | Tuned physics optimizer (Phase 3 ships the framework and a linear stub only) |
| `RuleRegistryProvider` + `MachiningRule` asset type | Engagement-aware / chip-thinning feed modulation in toolpaths (belongs with Adaptive / Surface) |
| `MaterialMachinabilityProvider` coordinated with #16801 | Vendor catalog ingestion pipelines (enabled by rule-asset design, out of epic) |
| `FormulaProvider` coordinated with FeedsAndSpeeds addon | Per-operation F&S overrides that bypass the TC — TC-cloning replaces this |
| TC properties: `OpTypeHint`, `MaterialHint`, `FeedSpeedProvenance` | RampFeed / LeadInFeed / LeadOutFeed resolution (can be added later without schema change) |
| Per-TC Suggest F&S button + confirm dialog + provenance badges | Workpiece rigidity / thin-wall chatter modeling |
| TC cloning flow when suggested values diverge from current | Chatter / stability-lobe prediction |
| Sanity flag: F&S unset | Automatic "resolve on property change" behavior (explicit opt-in only) |
| Sanity flag: TC divergent from resolver suggestion (Phase 2) | Job-file schema changes beyond ToolController property additions |
| `Setup` / rigidity / torque-curve data model (Phase 3) | |
| Optimizer stage API + linear stub (Phase 3) | |
| Ruleset import/export (Phase 2) | |

## Coordination

- **@Connor** — current owner of the tool library and CAMAssets work. Phase 1
  touches ToolBit storage (presets), `.fctb` serialization, and the
  ToolBit editor; Phase 2 adds a new CAMAssets asset type for rules.
  Design review sought on Phase-1 data shape and the rule-asset schema.
- **@dubstar-04** — CAM core maintainer and author of the external
  `FeedsAndSpeeds` addon. Invited to review the Phase-1 UX (confirm-dialog
  flow, provenance badges, Sanity integration) and to collaborate on
  Phase 2's `FormulaProvider` / Explorer panel, either by absorbing the
  addon's calculator core into the workbench or by sharing code between
  the workbench implementation and the addon.
- **#16801 reporter** — Phase 2's `MaterialMachinabilityProvider` builds
  directly on the model extensions proposed in that issue. The epic's
  phase-2 kickoff should link back.

## Related issues

- #21817 — Data model extensions for high-quality F&S calculations. Closed
  across Phases 1, 2, and 3.
- #16801 — `Machinability` requires coding changes per tool-material and is
  missing chipload data. Addressed in Phase 2 via the
  `MaterialMachinabilityProvider`.
- #28419 — Feeds and speeds should be stored on the toolbit. Addressed in
  Phase 1 via `ToolPresetProvider` + `ToolBit.presets`.
- #14460 — Historical: introduced feeds/speeds on the material model.
  Baseline for the Phase 2 extension.

## Related Epics

- **Tools** — Phase 1 touches ToolBit storage and the
  ToolBit editor; coordinate to avoid conflicting changes in the tool
  library UI.
- **Postprocessors** — no direct dependency; post-processors
  continue to read `TC.HorizFeed` / `TC.VertFeed` / `TC.SpindleSpeed`.
- **Circular Holes Improvement** — drilling-specific F&S (`FeedPerRev`,
  peck retract feed) may eventually want to flow through the resolver;
  out of this epic's scope, flagged for alignment.
