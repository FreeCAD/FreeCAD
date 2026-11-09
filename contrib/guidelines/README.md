# Contributor guidelines

These guides help contributors design, implement, and review changes in the FreeCAD repository.
They apply equally to work written with or without assistance. Start with the task at hand;
there is no need to read the entire collection before fixing a small bug.

| Guide | Questions it helps answer |
| --- | --- |
| [Writing code](writing-code.md) | How can the implementation be clear, idiomatic, and safe without unnecessary scaffolding? |
| [Reviewing changes](reviewing-changes.md) | What evidence establishes a bug, and what makes a review finding actionable? |
| [Architecture](architecture.md) | Which layer owns the behavior, and how should modules interact? |
| [Code reuse](code-reuse.md) | Where should an existing solution be sought, and when should a helper become shared? |
| [Documents and geometry](documents-and-geometry.md) | How do persistence, recompute, links, placements, and naming affect correctness? |
| [GUI and translations](gui-and-translations.md) | How should commands, task panels, previews, input, and text behave? |
| [Design proposals and FEPs](design-proposals.md) | When is design discussion needed, and what does a proposal's status establish? |
| [Testing and validation](testing-and-validation.md) | Which tests and checks demonstrate the change works and preserves existing behavior? |
| [Contributing changes](contributing-changes.md) | How should scope, history, review rounds, and attribution be handled? |

For a first bug fix, read the writing, reuse, and testing guides, then the guide for the affected
subsystem. Before submitting, use the review guide to inspect the whole change. For a new
workflow or shared API, start with architecture and design proposals before implementation.

## How these guides fit together

[CONTRIBUTING.md](../../CONTRIBUTING.md) defines the contribution process and
[AI_POLICY.md](../../AI_POLICY.md) defines responsibilities when using AI. The
[Developers Handbook](https://freecad.github.io/DevelopersHandbook/) explains project-wide
development and design practices. The [FEP repository](https://github.com/FreeCAD/FreeCAD-Enhancement-Proposals)
records substantial proposals and decisions. These guides make that material easier to find
and connect it to implementation choices in the repository; they do not replace those sources.

Use the checked-out configuration for exact formatter settings, supported dependencies, and
commands. Follow the affected module's established conventions, while examining old patterns
critically. Examples elsewhere demonstrate a technique, not permission to introduce a dependency.

## Reading the examples

Code examples illustrate a specific contract or failure mode. Unless identified as a complete
test, they are excerpts that need the surrounding class, imports, and build registration.
Repository links identify the implementation to inspect for current signatures and ownership.
An excerpt is not evidence that the associated behavior has been tested in your environment.

Several examples contrast an unsafe approach with a narrower one. The important part is the
assumption that makes the latter valid: document ownership, a constructor invariant, a property
migration, or a supported library contract. Carry that assumption into the implementation.

## Maintaining the guidance

Keep each topic's explanation in one place and link to it from related guides. Prefer links to
source files over copied implementations, especially for registration and version-dependent APIs.
When changing a helper or workflow used as an example, check that its explanation still applies.
Add a new rule when it explains a recurring engineering decision, with its rationale and limits.
Avoid turning a single review preference into an unconditional project-wide prohibition.
