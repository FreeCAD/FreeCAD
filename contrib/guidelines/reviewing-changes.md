# Investigating and reviewing changes

[Guide index](README.md) · [Testing](testing-and-validation.md) · [Contributing](contributing-changes.md)

Review establishes that a change solves its stated problem without silently damaging another
workflow. It also makes the implementation understandable to the next contributor. The same
process is useful before writing a fix, while reviewing one's own work, and when reviewing a PR.

## Establish the problem before judging a patch

Read the report, reproduction, relevant discussion, previous attempts, and their review feedback.
Identify the expected behavior and whether the current checkout differs from the reported build.
Inspect the complete affected functions and their callers, not just added lines.

Use history to explain conditions that look unnecessary:

```sh
git log -- path/to/file
git blame -L 120,180 -- path/to/file
git log -S 'relevantSymbol' -- path/to/file
```

The paths and range above are placeholders for the affected code. Use `git log -L` for function
history and `git bisect` when a reliable reproduction and suitable builds are available. Record
the revisions tested. A source-level explanation of a recent change is a suspected cause until
it is established by reproduction, a controlled comparison, or a bisect.

### Follow a regression back to its intent

Read linked or mentioned commits, PRs, and related issues, including closed attempts. Follow
their references back through the relevant fixes and reverts until the chain that explains
the behavior is read and understood. Inspect the actual patches and discussion, not just the
titles or final status. Determine why each PR was closed or merged and which later change,
if any, superseded it. If a reference is unavailable, record that gap instead of assuming its
contents.

Inspect changes around the reported revision and the last known working build. The report's
date bounds the investigation but does not prove when the bug was introduced: an older defect
may become visible only after a new caller, document, or selection sequence reaches it.

Try the exact reproduction on current `main` and on the last known-good commit or build.
Keep the document, preferences, dependencies, and steps comparable, and record the actual
revisions and results. If the current environment cannot build or run an older revision, state
the limitation and what evidence can still be obtained. Do not claim a bisect was performed
when only source history was inspected.

Explain why the suspected change was made and what it intended to preserve or enable. It may
be an implementation mistake, or a deliberate, sound change that exposed another missing
dependency or broke an adjacent workflow. Repair the cause while retaining the original fix's
valid behavior; do not simply revert it because the newer symptom disappears.

Use only APIs, behavior, and evidence that exist and have been checked. If the chain leaves
the expected behavior ambiguous, state the alternatives and ask for a decision before changing
that contract. If a technical fact is uncertain, say so and identify the evidence or experiment
needed to resolve it.

Check why earlier fixes were reverted. For example, broadening a command's type filter may
restore geometric links while also enabling links to non-geometric documents. Repeating the
broad filter is not progress; the regression case and the original positive case both matter.

## Review in passes

| Pass | Questions |
| --- | --- |
| Problem and scope | Is the expected behavior agreed? Does the patch solve it? What else changes? |
| Design agreement | Were the required working groups involved, and is a required FEP in place? |
| Architecture and reuse | Does the owner remain the same? Is an existing helper or hook being bypassed? |
| Correctness | Are inputs, units, references, ordering, failures, and lifetimes handled correctly? |
| Compatibility | Can old documents restore? Do Python callers and saved preferences retain meaning? |
| Integration | Are commands, sources, types, resources, and tests actually registered? |
| Evidence | Do the tests exercise the failure and meaningful unaffected cases? What remains unverified? |
| Maintainability | Are names, structure, comments, and dependencies clear without a verbal walkthrough? |

The [handbook review guide](https://freecad.github.io/DevelopersHandbook/bestpractices/codereview)
offers broader conventions. Apply them with local context. A historical naming style is not
automatically a defect in a focused fix, and a reviewer preference is not an API contract.

Assess both the complete PR and its individual commits. Is the intention clear, and can each
step be reviewed and built in order? Apply the
[PR size guideline](contributing-changes.md#keep-prs-small-and-structured): roughly 400–500 changed
lines is the usual upper range, with larger changes split into coherent PRs where possible.
An exception needs a reason and a review plan, not weaker coverage or an artificial split.

## Trace a result through all its dependencies

For a shape result, follow the inputs through property changes, invalidation, execution, mapping,
and presentation. Check undo/redo and save/restore where persistence is involved. For GUI
callbacks, follow the owning document and edit session through switching and closure. For shared
code, inspect all relevant callers and overrides, including Python exposure.

Example: a panel calls `resetEdit()` on the active document when rejecting an old task. The
normal one-document test passes. Switching to another document before rejection can end the
wrong edit session. The finding should identify the captured owner that is lost, the sequence
that changes the active document, and the cleanup that must use the original session.

For a public API, check actual callers and declared semantics. A symbol that looks like a kernel
primitive may be a FreeCAD wrapper that also preserves naming metadata. A helper that returns
null only for detached objects still requires the attachment invariant at its call site.

## Verify disputed claims

Check the implementation and supported platform documentation before accepting a claim about
an API, a fallback, or an operating-system flag. A review comment is evidence to investigate,
not authority that replaces the code. Distinguish unsupported platforms from supported cases
that merely are not represented in one local environment.

For example, a file-lock change can involve transient interruption, lock contention, unavailable
locking, access rights, and deletion semantics. Those are different outcomes. Decide from the
actual state transitions and API contracts whether retry, reporting, or fallback is appropriate.
Do not recommend a guard merely because failure sounds possible, or remove one merely because
the common path succeeds.

When a claim cannot be verified locally, state what supports it, what is uncertain, and the
smallest experiment that would resolve it. Do not present an automated suggestion or a plausible
explanation as a reproduced defect.

## Make findings actionable

A useful finding contains a precise location, triggering conditions, the incorrect result,
and its practical impact. Suggest a direction when there is a well-supported one. Avoid demanding
a particular rewrite without establishing why the existing contract fails.

Compare these observations:

- Vague: the pointer might be unsafe.
- Actionable: after closing the task's original document, the delayed callback dereferences
  its cached GUI document; re-resolve the owner and stop if that session no longer exists.

Distinguish a correctness problem, a missing regression case, a design concern, and optional
cleanup. Resolve serious findings before polishing small style details. If a change is safe
only under an assumption, make the assumption explicit and verify the caller establishes it.

Readable code should reveal intent, but necessary domain explanations are valuable. Keep the
comment explaining a coordinate transform or selection order. Prefer extracting a named
operation to adding paragraphs that narrate a complicated block line by line.

## Check the evidence itself

A test that inspects only an updated link may pass even when the final geometry ignores it.
A test named after undo/redo must actually perform both operations and inspect their results.
A skipped test or zero selected tests is not a successful execution of that behavior.

Separate what each check establishes: whitespace checks are not compilation; compilation is
not GUI reproduction; mocked CI-event tests are not a live Actions run. Check that evidence
corresponds to the final revision, especially after subsequent fixes change the tested path.

For performance claims, require comparable inputs, build settings, and measurement boundaries.
Consider incremental and clean builds separately. An improvement on one machine can justify
further investigation without establishing the same ratio for every contributor.

## Finish with the whole change

After the reproduction works, reread the final diff, including registrations, removed code,
comments, and shared consumers. Ask whether an existing mechanism can achieve the behavior
with a simpler design. Resolve questionable assumptions instead of defending them with a longer
description. Keep independent improvements for another change.

Inspect the code around the changed lines and the reachable callers and users again after
the implementation settles. Check the affected contracts for regressions, crashes, build or
link failures, stale state, UI/UX inconsistencies, and unexpected behavior. The depth follows
the actual impact of the change; a local successful example does not establish shared callers
remain correct. Use the validation guide and the human author's workflow testing as evidence.

Conclude with the verified behavior, important limitations, and validation still needed. Review
is a collaborative engineering discussion; new evidence can change either the implementation
or the reviewer's initial assessment. The contributor remains responsible for understanding
and presenting the final work under [AI_POLICY.md](../../AI_POLICY.md).
