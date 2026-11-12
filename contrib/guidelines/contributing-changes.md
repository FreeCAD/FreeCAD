# Preparing and maintaining a contribution

[Guide index](README.md) · [Reviewing changes](reviewing-changes.md) · [Testing](testing-and-validation.md)

[CONTRIBUTING.md](../../CONTRIBUTING.md) defines the contribution process. This guide focuses on
keeping a change understandable, independently verifiable, and economical to review and build.
Follow [AI_POLICY.md](../../AI_POLICY.md) for assistance, disclosure, and human responsibility.

## Keep one problem visible

Describe the failure and agreed result before expanding the implementation. Keep necessary
registration, migration, and regression coverage in scope. Defer independent improvements to
another change. Formatting, renaming, or reorganizing unrelated code makes the behavior change
harder to inspect and increases conflict risk.

If the fix requires a preparatory refactor, separate that behavior-preserving step. A series can
make a large capability reviewable, but each step must have a useful, coherent purpose and keep
the existing application working. Do not leave a broken intermediate state merely because the
last commit repairs it.

Read previous attempts and explain their disposition to yourself before repeating them. Check
the actual target branch and remotes rather than assuming every checkout calls its fork
`origin` and upstream `upstream`.

## Keep PRs small and structured

As a rule of thumb, aim for a maximum of roughly 400–500 changed lines per PR, considering
additions and deletions rather than only the net line count. This is a reviewability guideline,
not a target to fill. A small behavioral change can still need careful review, while repetitive
generated output may be large without the same conceptual complexity.

When a change grows beyond that range, split it into multiple PRs where possible. Choose
coherent boundaries: an independently useful preparatory refactor, a shared facility with its
tests, then the feature that uses it. Explain the series and its dependencies so each PR and
each commit can be reviewed in context. Keep every merged step buildable and working; do not
split a fix from a migration or regression coverage that is necessary to evaluate it safely.

Larger PRs can be justified when a safe, meaningful split is impractical, but exceptions should
not become the normal approach. Explain the reason and agree on the review approach with the
maintainers. Do not hide complexity by removing tests or documentation, minifying code, or
splitting one inseparable operation into incomplete PRs just to meet a line count.

Make the intention clear at both levels: what the complete PR achieves, and why each commit
exists. Separate mechanical changes from behavioral ones where useful. Reviewers should be
able to inspect the commits in order and then assess the complete PR diff without reconstructing
the development history. The contributor provides that context in their own words.

Before broad implementation, check the
[working-group and FEP requirements](design-proposals.md#involve-the-responsible-working-groups).
Reviewable commit structure does not replace agreement on a substantial design or workflow change.

## Build a history that can be reviewed and bisected

Each commit should build and work with the commits preceding it. Group by responsibility and
dependency. A local refactor followed by the fix and its focused tests can be useful; keeping
the behavior and regression together is also appropriate when that makes the change easier
to evaluate. There is no blanket requirement that tests be disposable or always separate.

Use short imperative subjects with the module's established prefix. Explain the cause and
design choice in the body when the diff alone is insufficient. These messages are the human
contributor's own words; AI disclosure does not replace an explanation of the change.

Preserve human authorship when bringing in existing work. CONTRIBUTING.md specifies the
attribution and follow-up requirements for cherry-picks, including when a human co-author
trailer is needed. AI-assistance disclosure is a separate matter from human authorship.

Before cherry-picking, inspect prerequisites and check whether the patch is already present
under another commit. When adapting it, preserve current transaction, restore, and API behavior
rather than resolving a conflict by accepting one entire side. Review the resulting diff against
the intended base and use patch/range comparison where useful.

## Complete local verification before a push

Run the applicable hooks, build affected targets with warnings visible, and run the relevant
tests. The [validation guide](testing-and-validation.md) covers discovery, binding checks,
human manual testing, and the evidence required for changing test expectations.

Inspect the final files and registrations. Check that generated files, baselines, translations,
and dependency locks changed only when the task requires it. Verify icons and forms are packaged,
not just visible from the source checkout. A branch that builds locally with optional components
disabled can still be missing registrations needed by other configurations.

For UI changes, capture the before/after behavior the contributor needs to present, and include
failure/cancel states when changed. State limitations accurately. Use closing-issue language
only when the supported scope actually resolves that issue.

## Handle review as one coherent round

Understand each finding and check its assumptions in the code. Implement the correction and
revisit shared consumers and affected tests. If a concern is based on an incorrect contract,
collect the evidence rather than adding a workaround solely to satisfy it. The contributor
communicates the reasoning in their own words.

Group related fixes, validate the final result, and avoid repeatedly pushing small unchecked
changes. Fold fixes into appropriate commits when the project's history requirements and
branch ownership permit it. Do not rewrite another contributor's history without agreement.

Rebase or merge the target branch for a concrete reason: a conflict, a dependency the change
needs, or a maintainer request. Refreshing a PR does not itself accelerate review and can cause
additional notifications and builds. When an authorized history rewrite requires a guarded push,
verify the remote head and use a lease tied to that inspected state.

## Preserve CI meaning

A CI check represents evidence for a particular revision and event. For workflow changes,
consider PR identity, head SHA, branches, label events, forks, cancellation, and retries. Required
check names and result propagation are part of the contract. A skipped job is not necessarily
equivalent to success, and missing or failed required evidence must not be turned into a pass.

Avoid starting duplicate expensive builds for unchanged input or occupying scarce runners while
waiting for work that needs those runners. Test the actual event/state combinations. Mocked
workflow tests can support the logic, but do not establish live platform behavior on their own.

## Keep responsibility explicit

The contributor reviews, understands, and tests the final work. Public issues, PR descriptions,
commit messages, comments, and review replies are written by the contributor as required by
AI_POLICY.md. The policy's attestation is theirs to make; tool access or an automated check
cannot make it on their behalf. Disclose assistance accurately and preserve applicable license
and attribution requirements.
