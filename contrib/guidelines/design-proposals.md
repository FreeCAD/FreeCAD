# Design decisions and FreeCAD Enhancement Proposals

[Guide index](README.md) · [Architecture](architecture.md) · [GUI and translations](gui-and-translations.md)

Agree on the problem and expected workflow before investing in a broad implementation. A small
fix can usually follow the existing contract. A change to that contract may need design input
from the people who maintain and use the affected subsystem.

## Find decisions before making new ones

Read the issue and comments, linked discussions, previous open and closed attempts, and the
history of the affected code. Check whether a previous patch was closed because the problem
was solved elsewhere, its dependencies were missing, the behavior was rejected, or it was
superseded. A closed PR is not automatically a rejected design, and a merged PR is not proof
that every choice remains correct after later changes.

Read the relevant [Developers Handbook](https://freecad.github.io/DevelopersHandbook/) sections
and search the [FEP index](https://github.com/FreeCAD/FreeCAD-Enhancement-Proposals#feps). Verify
the proposal's current text, status, and linked implementation. Do not infer available APIs
from an accepted design or from a prototype branch.

## Choose a proportionate discussion

| Change | Required involvement and useful starting points |
| --- | --- |
| Bug fix preserving the existing workflow | Issue, reproduction, relevant history, focused implementation |
| Small feature fitting an established pattern | Existing issue/design discussion and module examples |
| Changed interaction, terminology, or visual behavior | Notify the Design Working Group; involve it early for substantial changes; consult the design guide and FEP-0007 |
| Changed standards, CAD workflows, or engineering semantics | Consult the CAD Working Group, with workflow examples and compatibility analysis |
| Radical codebase change or new core module/workbench | A FEP is required; follow the FEP process and discuss architecture before broad implementation |

The formal requirements and current process belong in
[FEP-0001](https://github.com/FreeCAD/FreeCAD-Enhancement-Proposals/blob/master/FEPs/FEP-0001-process/README.md).
Radical changes to the codebase and new core modules or workbenches require a FEP. Minor fixes
do not need a formal proposal by default. Other substantial changes or changes lacking consensus
may also need one; consult the process and the relevant project group.

## Involve the responsible working groups

For UI changes, notify the Design Working Group (DWG) and give it an opportunity to review the
affected interaction. For substantial or radical changes to layout, navigation, command behavior,
or interaction patterns, involve the group while the design is still open, before committing to
a broad implementation. Provide concrete workflow steps and before/after evidence, including
keyboard use, cancellation, discoverability, and consistency with related commands.

Consult the CAD Working Group (CWG) when changing standards, engineering semantics, or CAD
workflows. Explain the effect on professional use, applicable industry conventions, and the
existing FreeCAD workflow. Identify compatibility consequences for documents and downstream
work. A workflow used by another CAD product is useful evidence when verified, but it does not
by itself establish the right behavior for FreeCAD.

A change can need both groups: an interaction redesign that changes modeling semantics needs
both UI and domain input. Working-group consultation and the FEP process serve different
purposes; one does not replace the other when both apply. Record the relevant discussion and
resolve material design questions before treating the implementation as ready for review.

The human contributor contacts the groups through the project's current channels. An assistant
identifies the consultation needed and supplies technical facts; it does not post or ping groups
on the contributor's behalf. Follow the communication responsibilities in
[AI_POLICY.md](../../AI_POLICY.md).

## Understand status and implementation separately

A draft explores an approach; a proposed document is being considered. Accepted design and
implemented behavior are distinct. Active informational/process guidance can apply without
having a corresponding C++ feature. Rejected or withdrawn proposals can still explain why an
alternative was not pursued. Always use the current FEP's own status and discussion.

Keep the evidence separate:

- The proposal establishes the intended contract and rationale.
- The merged implementation establishes what is available in the checkout.
- Tests and manual verification establish the behavior exercised in a particular revision.

For example, a proposed future recompute architecture does not make a new scheduler API
available today. A compatibility fix must work with the document machinery actually present,
while avoiding choices that unnecessarily obstruct an agreed direction.

## Explain the design with a concrete workflow

Start with a scenario rather than an abstraction. For a path-pattern change, identify the
selected path, the instance orientation, the expected coordinate frame, and the geometric
result. For a property-editor interaction, identify what happens before editing, when a value
changes, and when the user cancels. Screenshots alone do not specify dependency or undo behavior.

Useful design notes answer these questions:

1. Who is affected, and which operation is currently impossible, misleading, or incorrect?
2. Which existing contract will stay the same, and which behavior will change?
3. What existing mechanism can implement it, and which alternative was considered?
4. What are the effects on documents, Python callers, units, references, preferences, and UI?
5. What evidence will distinguish a correct implementation from a superficially working one?
6. Which parts can land independently, and which follow-up work is intentionally deferred?

These are prompts for engineering work, not a PR-description template. Contributors communicate
their own rationale in accordance with [AI_POLICY.md](../../AI_POLICY.md).

## Evaluate alternatives against the same constraints

Compare approaches on correctness, dependency direction, migration cost, interaction, testability,
and maintenance. A small patch that changes a saved meaning can cost more than a somewhat
larger local fix. A new generic service can also be excessive when one private method suffices.
Use actual callers and representative documents instead of hypothetical future needs.

For example, exposing an existing command through a module-owned menu manipulator preserves
loading boundaries. Putting the workbench command directly in core Gui may require the module
to be loaded unconditionally. The latter can look simpler in the diff while changing startup
and optional-module behavior. Compare the complete contracts, not the line counts.

For performance choices, measure representative workloads. Separate clean-build improvements,
incremental builds, interactive latency, and memory use. A new option needs a meaningful owner,
default, documentation, and validation; it should not conflate otherwise independent policies.

## Use a prototype to answer a question

Identify the uncertainty the prototype will resolve: whether a kernel operation preserves the
needed geometry, whether a widget supports a workflow, or whether the shared hook has enough
context. Keep experiments local and do not mistake a one-off successful demo for merge readiness.

Before integrating, replace temporary assumptions with the real document/lifetime contracts,
use existing facilities, register resources and tests, and remove debugging artifacts. Read
[writing code](writing-code.md), then [review the final implementation](reviewing-changes.md).
If the experiment disproves the design, update the design decision instead of accumulating
workarounds around the prototype.

## Keep guidance discoverable

Link the agreed behavior from the owning documentation or relevant code comment when the
rationale cannot be expressed by code alone. Link to the proposal instead of copying its full
specification into this repository. Update affected workflow documentation and release notes
when appropriate. Preserve the difference between a temporary workaround, an implemented
contract, and a future direction.
