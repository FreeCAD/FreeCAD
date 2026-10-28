# Testing and validation

[Guide index](README.md) · [Reviewing changes](reviewing-changes.md) · [Documents and geometry](documents-and-geometry.md)

Validation should demonstrate the intended behavior and protect the existing contract. Its
purpose is not to make CI green by weakening the evidence. The human author must understand
and test the affected workflow, even when automated checks or assistants have already run.

## Choose coverage from the failure and the risk

Add a regression test when it is necessary to protect the corrected behavior: a reproducible
logic error, a compatibility boundary, a shared mechanism, or a failure likely to recur deserves
focused coverage. Start with the smallest scenario that exercises the reported failure and
an important unaffected case. Do not create a large new framework for an incidental edit.

Where automated coverage is impractical, explain the limitation and provide repeatable manual
steps and the observed result. A missing test is a gap to assess, not permission to claim that
source inspection has established runtime correctness.

The [testing handbook](https://freecad.github.io/DevelopersHandbook/technical/automated_testing)
introduces the frameworks. C++ suites live under [tests/src](../../tests/src); Python suites
generally live in their workbench package. Choose the layer that observes the contract directly
without requiring unrelated architectural changes.

| Change | Evidence to consider |
| --- | --- |
| Feature result or placement | Recompute, validity, geometric result, transformed parent or linked instance |
| Dependency rerouting | Insertion and deletion, subsequent edits, final geometry, unresolved references |
| Persisted property or enum | Old-document restore, save/reopen, expressions, Python callers, undo/redo |
| Task lifetime | Two documents, workbench switching, repeated reject, document/view closure |
| Command filter | Accepted targets, excluded targets, read-only/frozen state, menu and shortcut paths |
| Input or preview | Initial selection, keyboard handling, accept/cancel, transaction and preview cleanup |
| Rendering | Relevant actions and element state, pixel ratio, selection, existing visual snapshots |
| CI or packaging | Events, branches, forks, artifact content, required-check names and result propagation |

Use this table selectively. It is not a demand to test unrelated workflows for every edit.
Inspect callers and history to decide which risks are actually reachable.

## Preserve existing tests unless their contract changed

Investigate a failing test before editing it. Read its name, assertions, setup, and the history
of the behavior it protects. Determine whether the implementation regressed, the environment
differs, or the intended contract legitimately changed.

Change an existing expectation only when the underlying behavior or original expectation has
been shown to be wrong, and the revised expectation is justified by the agreed design. Keep
coverage of unaffected behavior. Explain why the old expectation is invalid and why the new
one is correct; changing the number to whatever the new implementation returns is not evidence.

Do not remove assertions, skip a failing case, widen tolerances, update snapshots, or alter a
fixture simply to pass CI. Numerical tolerances need a geometric/numerical reason. Visual
baselines need human inspection of the before/after result. If a test was incorrectly written,
correct it with evidence of the intended contract, independently of whether the patch passes.

## Make tests discriminate between correct and incorrect results

Use a meaningful operation, then assert its observable result. Recompute succeeding alone
does not prove the intended feature contributed to the model. A changed property alone does
not prove the displayed or saved result uses it.

For example, [TestFillet.py](../../src/Mod/PartDesign/PartDesignTests/TestFillet.py) checks volume
for a fillet result and separately exercises unique and unresolved reference matches. Choose
assertions that would reject a plausible wrong result, including position or validity where
volume alone is insufficient.

This complete test illustrates shape assertions and cleanup, not a regression for a particular
bug. A real regression would include the operation that originally failed:

```python
import unittest
import FreeCAD as App
import Part


class TestShapeReplacement(unittest.TestCase):
    def setUp(self):
        self.document = App.newDocument()
        self.addCleanup(App.closeDocument, self.document.Name)

    def test_shape_replacement_updates_volume(self):
        box = self.document.addObject("Part::Feature", "Box")
        box.Shape = Part.makeBox(10, 4, 3)
        self.document.recompute()
        self.assertAlmostEqual(box.Shape.Volume, 120)

        box.Shape = Part.makeBox(20, 4, 3)
        self.document.recompute()

        self.assertTrue(box.Shape.isValid())
        self.assertAlmostEqual(box.Shape.Volume, 240)
```

When feasible, run the actual regression against the old implementation and confirm it fails
for the intended reason. A test that cannot reach its named scenario, or silently skips because
the fixture no longer contains the needed geometry, needs investigation.

## Isolate state and register discovery

Register cleanup as resources are created, so failed assertions do not leave documents, editors,
preferences, or global selection behind. Restore the previous preference value, including the
distinction between an absent key and an explicitly saved one. Keep fixtures readable and
deterministic. Share setup when it clarifies the contract, not when it hides each test's scenario.

A new test file must be installed and discovered. For example, Part Design lists Python tests
in [CMakeLists.txt](../../src/Mod/PartDesign/CMakeLists.txt) and imports them through
[TestPartDesignApp.py](../../src/Mod/PartDesign/TestPartDesignApp.py). Its existing suite is
registered in [Init.py](../../src/Mod/PartDesign/Init.py). Adding a case to that suite does not
require registering another suite. For C++, inspect
[tests/CMakeLists.txt](../../tests/CMakeLists.txt) and the module's source lists.

## Use the actual build environment

Inspect the existing CMake cache, build options, [pixi.toml](../../pixi.toml), and relevant
[CI workflows](../../.github/workflows). Do not assume a path from another contributor's machine
or change supported-version rules to match the local environment.

The following are command forms; replace the build directory, target, regex, module, and files
with values discovered in the checkout. Run them in the configured development environment:

```sh
cmake --build <build-directory> --target <affected-target>
ctest --test-dir <build-directory> -R <test-regex> --output-on-failure
FreeCADCmd -t <headless-test-module>
FreeCAD -t <gui-test-module>
```

For a Pixi setup, `pixi run build` and `pixi run test` select the repository defaults; narrower
targets and suites are often sufficient during development. CTest selection does not establish
that every Python workbench test ran. Confirm selected tests actually executed and did not
silently skip. Use the FreeCAD runtime matching the build under test.

Run applicable hooks on changed files with `pixi run pre-commit run --files <files>` when using
the Pixi environment. Inspect [.pre-commit-config.yaml](../../.pre-commit-config.yaml) for coverage.
The [lint workflow](../../.github/workflows/sub_lint.yml) includes further checks such as
clang-tidy, pylint, spelling, and old-style Qt connections. Follow its current commands and
configuration rather than inventing local equivalents. After relevant binding changes, inspect
and run [check-stubs.sh](../../src/Tools/typing/check-stubs.sh) in the configured environment.

## The human author verifies the workflow

Before submission, the human author must test and inspect the final change. For an interactive
fix, repeat the reported steps in FreeCAD and exercise the adjacent cases that the implementation
can affect. Check for crashes, stale state, unexpected behavior, and UI/UX inconsistency as well
as the original symptom. Use the relevant platform/theme/unit settings when they matter.

For non-interactive code, manually exercise the affected API, file, command-line, or build
workflow as appropriate. For documentation, read the rendered result and check the instructions
and links. Record the revision and what was actually done. Automated tests supplement this
human verification; an assistant cannot attest that the author performed it.

After changes prompted by review, reassess affected tests and repeat manual steps when the
changed code invalidates earlier evidence. Inspect surrounding code and shared consumers again
if the final design reaches beyond the original patch.

## Report evidence and gaps accurately

Record the command or workflow, configuration, revision, outcome, and important limitation.
Distinguish static inspection, syntax checks, compilation, automated runtime tests, visual
inspection, manual reproduction, and CI. Compilation without warnings for one target does not
establish all-platform compatibility. A single translation unit check does not establish linking.

When a build is blocked, identify the blocker and what checks still apply. Do not claim success
for unrun or unavailable checks. CI then provides independent platform coverage, rather than
being used as a repeated trial-and-error loop for defects that can be caught locally.
