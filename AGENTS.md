# FreeCAD agent instructions

These instructions apply to AI-assisted work throughout this repository, regardless of model
or tool. The goal is a pull request that fits the existing code, is easy to review, and merges
without rounds of corrections or repeated CI runs: a small change with a clean, long-term
design that solves the agreed problem and keeps existing documents and workflows working.

## Project guidance

- Follow [CONTRIBUTING.md](CONTRIBUTING.md) and [AI_POLICY.md](AI_POLICY.md).
- Consult the relevant sections of the [Developers Handbook](https://freecad.github.io/DevelopersHandbook/),
  including the [code review guide](https://freecad.github.io/DevelopersHandbook/bestpractices/codereview)
  and the [design guide](https://freecad.github.io/DevelopersHandbook/designguide/).
- Check the status of relevant [FreeCAD Enhancement Proposals](https://github.com/FreeCAD/FreeCAD-Enhancement-Proposals#feps).
  Draft proposals are discussion material. Acceptance does not mean an API is implemented.
  Use [FEP-0007](https://github.com/FreeCAD/FreeCAD-Enhancement-Proposals/blob/master/FEPs/FEP-0007-consistent-language/README.md)
  for user-facing language.
- Repository configuration (`.clang-format`, `.clang-tidy`, `.pre-commit-config.yaml`,
  `.pylintrc`, CMake, CI workflows) is authoritative for formatting, tooling and commands. If
  guidance and code disagree, explain the discrepancy and choose a justified approach for the
  affected module. Do not copy every existing pattern uncritically or impose unrelated
  modernization.

## The human contributor is in charge

FreeCAD accepts AI-assisted work only with a human in charge ([AI_POLICY.md](AI_POLICY.md)).
The contributor decides what is done, understands, reviews and tests every change, and takes
responsibility for it, including correctness and licensing. The agent assists; it never acts
as the contributor.

- Do not communicate on GitHub or any other project channel. Do not open, comment on, review,
  label or react to issues and pull requests, even when the available tools allow it.
- All text that leaves the local checkout is written by the contributor: commit messages, PR
  titles and descriptions, issue reports, comments, and replies to reviewers. Do not write it
  for them to paste, and do not rephrase, polish or restructure their own text for those
  places either; the project wants the contributor's genuine words, however rough. When
  asked, give the facts they need (cause, changed behavior, affected files, validation done,
  open questions) as short notes. Translation is the only exception: keep it as close to the
  original sentence structure as possible, and the contributor includes the original text, as
  AI_POLICY.md asks.
- Commit, push, or rewrite history only when the contributor asks, and only with the message
  the contributor wrote.
- Do not merge `main` into a PR branch or rebase it without a reason: a conflict, a change
  on `main` the PR needs, or a maintainer's request. Every push notifies everyone following
  the PR and reruns CI, and it does not make a PR get reviewed or merged sooner.
- The contributor discloses AI assistance in their own words in the PR description and with an
  `Assisted-by: <model family> (<version or id>)` trailer in commits. When asked, state the
  actual model identity; never guess it. Do not add AI `Co-Authored-By` lines, "Generated with"
  signatures, or other tool branding.
- Never tick contributor attestations or claim human review, approval or testing that has not
  happened.
- Explaining review feedback, checking whether it is correct, and changing the code in response
  are in scope. The reply to the reviewer is the contributor's.

## Understand before editing

- Before starting, inspect the architecture around the change: which module and layer own the
  behavior, how its App, Gui and Python parts interact, and which existing mechanism already
  handles similar cases.
- Read and understand the code you are about to change before proposing anything: the
  function, its class, its callers and callees, the neighbouring features that follow the same
  pattern, and the tests that cover it. Search results alone are not understanding.
- Read the issue, reproduction, relevant comments, and linked fixes or reverts. Read the history
  of the lines you touch (`git log -L`, `git blame`): earlier fixes often explain conditions
  that look redundant.
- Read every earlier attempt at the same issue, open and closed PRs alike, and the review
  feedback they received. Understand why they stalled or were rejected and do not repeat
  them. Issues labelled as good first issues in particular attract repeated near-identical
  attempts.
- For regressions, identify the change that introduced the problem (`git log -S`, `git bisect`)
  and fix the cause, not the symptom. Distinguish a suspected cause from a reproduced failure
  or confirmed bisect.
- State the failing behavior, intended result, scope, and proposed approach briefly before
  editing.
- Verify APIs, types, signatures, namespaces, and ownership in the source before using them.
  Never invent a FreeCAD API or claim behavior that has not been established.
- Inspect the branch and working tree. Preserve unrelated changes. Do not reformat or change
  the line endings of unchanged code, even if they are inconsistent or do not match broader
  policies.

## Reuse before writing

FreeCAD is a large, mature code base. Almost every problem, algorithm, widget, helper or
workflow an agent is tempted to write already exists somewhere in it, in a library it links,
or in a sibling feature. New code is a last resort, not the starting point. Quickly written
new code is the most common reason reviewers send a PR back.

- Before writing anything, search for an existing solution: in the module you are changing,
  in the layers below it (`Base`, `App`, `Gui`, `Mod/Part`), in sibling workbenches, and in the
  libraries FreeCAD already depends on (OpenCASCADE, Coin, Qt, Eigen, boost and others). Look
  for the same task under different names, and for how the closest analogous feature, command,
  task panel or earlier fix solved it.
- Use what exists as it is. If it almost fits, extend or generalize it in place, with its
  existing callers kept working, instead of writing a variant next to it. If it lives in the
  wrong layer, move it down in a preparatory commit (for example from a workbench into Part or
  App so other workbenches and Python can use it) and delete the old copy instead of copying it.
- Write new code only for the part that nothing existing covers, and keep that part as small
  as the problem allows. Be ready to say what you searched and why the existing code does not
  apply; the contributor and reviewers will ask.
- Never duplicate an algorithm, a geometric computation, a conversion, a tolerance, a
  validation, a parser, a widget or a preference handling routine that the code base already
  has, even when the existing one is harder to call. If calling it would add a dependency the
  layering does not allow, move it to a layer both sides can use.
- Examples of what is easy to reinvent by mistake (the list is illustrative, not complete):
  linear algebra and least squares (Eigen), shape fitting (`PlaneFit`, `CylinderFit`,
  `SphereFit` in Mesh), shape operations (`TopoShape::makeElement...`, `FCBRepAlgoAPI_*`),
  constraint solving (the Sketcher solver), vectors, placements, rotations, bounding boxes,
  quantities and tolerances (`Base::Vector3d`, `Base::Placement`, `Base::Rotation`,
  `Base::BoundBox3d`, `Base::Quantity`, `Base::Precision`), type conversion
  (`Base::convertTo<T>()`), global placements and subelement paths
  (`App::GeoFeature::getGlobalPlacement()`, `App::SubObjectT`, `Base::Tools::splitSubName()`),
  observers and task dialogs (`App::DocumentObserver`, `Gui::SelectionObserver`,
  `Gui::TaskView::TaskDialog`), unit-aware input (`Gui::QuantitySpinBox`, preference widgets),
  and Python geometry helpers (`DraftGeomUtils`, `DraftVecUtils`, `draftutils`, `Part` shape
  methods).

## Keep intended behavior

- Before changing behavior, list what the touched code currently does for its other callers and
  inputs, and keep all of it unless the change is explicitly meant to alter it. The fix is only
  for the reported case; neighbouring modes, commands and options must behave as before.
- Treat existing conditions, ordering and special cases as intentional until history, tests or
  callers show otherwise. Removing a check, a recompute, a signal or a selection step needs a
  reason.
- Find every place the changed code is called or used, not only the reported path: C++ callers,
  overrides of virtual methods, other workbenches, Python bindings and Python callers, macros
  and addons using the Python API, expressions, and preferences. Make sure the change does not
  regress any of them, and run the tests of the modules that use it.
- Check adjacent workflows the change can reach: undo/redo, save and reopen, recompute,
  links, multiple documents, edit mode entry and exit, preselection.
- Report every behavior change to the contributor, however small, including changes to
  documents created with development versions. Reviewers treat a behavior change the PR does
  not mention as a defect, and a change that affects all users when only an opt-in feature was
  intended is out of scope.

## Scope and pull request shape

- One problem per PR (CONTRIBUTING.md), kept as small as the problem allows. A larger change
  needs a reason and is easier to land as a series of small PRs.
- Keep the diff to what the problem needs: no drive-by reformatting, renames, reordering or
  cleanups in code you are not otherwise changing. Put a necessary cleanup in its own commit.
- Do not add APIs, parameters, options or abstractions without a current caller. Do not leave
  commented-out code, debug output, or code for hypothetical future use.
- Follow-up improvements that reviewers or you notice go in a separate PR, not into this one.
- Commits: each one builds and works on its own (`git bisect` must work) and has a single
  purpose. A typical order is a preparatory refactor without behavior change, then the change,
  then tests. A larger feature lands as a series in which each commit adds one capability with
  its tests. The contributor's subjects are short, imperative and prefixed with the module as
  its history does, for example `Part: Preserve attachment references`; a body explains the
  cause and the reason when they are not obvious from the diff.
- Fold review fixes into the commit they belong to instead of adding "address review" commits,
  and push once per review round. Every push reruns CI.

## Architecture and integration

- Evaluate CAD/CAM/CAE changes for engineering meaning as well as software correctness.
  Check the relevant units, coordinate systems, tolerances, geometric assumptions, and
  intended modeling workflow.
- Keep `src/Base` and `src/App` independent of `src/Gui` and workbench modules. App functionality
  must remain usable without the GUI (`FreeCADCmd`). Keep workbench-specific knowledge out of
  common Gui code. If a change needs to cross one of these boundaries, find the hook the lower
  layer already offers (virtual method, extension, signal, observer) instead.
- Workbench dependencies must use established module APIs and respect build options and load
  order. Do not introduce a dependency merely to reuse a small helper; assess its proper owner.
- Let modules own their commands and contribute through existing hooks, extensions, signals,
  or menu/toolbar mechanisms such as `Gui::WorkbenchManipulator`. Account for commands being
  unavailable until their module loads.
- Put behavior where it belongs: geometry and document logic in the App feature, presentation in
  the view provider, interaction in the task panel. View providers do not reach into specific
  task panel widgets. Prefer a virtual method on the right class over a function that switches
  on types. When one module needs special behavior from a core mechanism, add a generic hook in
  core that the module overrides, so core never names the module.
- Shape operations go through `TopoShape` and the Part wrappers. Calling OpenCASCADE directly
  loses the element maps that topological naming depends on.
- Rendering goes through Coin nodes. Do not add direct OpenGL calls elsewhere.
- Build task panels and dialogs from `.ui` files, as the rest of the code base does.
- Trace new functionality through applicable CMake source lists, type initialization, Python
  bindings, command registration, workbench menus/toolbars, resources/icons, and test discovery.
  Follow an analogous feature's actual registration. A missing step usually fails silently. For
  a typical C++ feature the points are `App/CMakeLists.txt` and `init()` in `App<Module>.cpp`,
  `Gui/CMakeLists.txt` (sources and `.ui`) and `init()` in `App<Module>Gui.cpp`, the command in
  `Command.cpp` and its `Create...Commands()` registration, `Workbench.cpp` menus, toolbars and
  task watchers, and the SVG icon with its `.qrc` entry. List them before editing.

## Documents, geometry, and compatibility

- Preserve transaction boundaries, undo/redo, notifications, and recompute scheduling. Use the
  surrounding transaction mechanism (`App::AutoTransaction` or
  `openCommand`/`commitCommand`/`abortCommand` in C++, `openTransaction`/`commitTransaction` in
  Python) and respect document restore. Trace `onChanged()`, `mustExecute()`, and `execute()`
  to avoid recursion and dependency cycles.
- Preserve persisted property names, types, units, defaults, and enumeration meanings.
  `App::PropertyEnumeration` stores an index: do not reorder or remove entries; deprecate in
  place as `PartDesign::Pad` does. Use the existing migration and restore hooks
  (`handleChangedPropertyName`, `onDocumentRestored`) when a schema change is necessary, and
  cover older documents. Options that exclude each other are one enumeration property, not
  several booleans; replacing booleans needs a migration and a compatibility shim for Python.
- Preserve the Python API used by extensions. If a breaking change is unavoidable, identify
  affected callers and addons, minimize the break, and document the replacement as required by
  CONTRIBUTING.md. Keeping a function name while changing its signature or return type is a
  break. Bindings are generated from `.pyi` files; edit those and the `*PyImp.cpp`, not the
  output.
- For placements, distinguish local coordinates, parent Part/Body transforms, and linked
  instances. Use link-aware resolution such as
  `App::GeoFeature::getGlobalPlacement(target, root, sub)` with the required root and subobject
  context. Check recompute invalidation and expression evaluation order when geometry depends
  on another object's placement. A forced full recompute does not prove correct dependencies.
- Preserve document, container, link, and subelement paths when resolving references. Distinguish
  the selected instance from its source and the tree's presentation from the dependency graph.
- Preserve topological naming metadata when rerouting references. An edge or face number is
  not stable identity. Do not repair unresolved references by guessing among ambiguous matches;
  identify the limits of a local fix and keep broader recovery outside its scope unless requested.
- Treat preference defaults, explicitly saved values, and legacy migrations separately.
  Inspect preference-loading code rather than assuming a `.ui` default controls behavior.
  Define each default once and read it from there; do not repeat the literal at every use.

## GUI behavior and lifetime

- Establish the owning document, view, object, and edit session. Callbacks and cleanup must
  act on that owner after the active document changes. Resolve GUI objects safely after
  document/view closure; review Qt ownership and Python wrapper lifetime.
- Connect document and application signals with `fastsignals::scoped_connection` members (or
  the observer classes) so they disconnect with their owner.
- React to the signal or event that reports a state change. Do not work around ordering with
  timers, application-wide event filters or global flags; they break when the dialog closes,
  the document is deleted, or the expected event never arrives.
- Make activation and cleanup safe for repeated calls where required by the lifecycle.
  Python GUI code may be reached before its workbench initializes; use established lazy
  loading and initialization mechanisms without introducing GUI imports into App paths.
- Keep command eligibility (`isActive()`) and execution consistent. Check preselection,
  selection consumed during dialog construction, and keyboard focus when hiding or replacing
  task widgets.
- Do not persist temporary workbench/edit-mode visibility as the user's toolbar preference.
  Use existing quantity widgets and verify unit initialization and binding order.
- Consider platform menu roles, keyboard interaction, translated text, scaling, and themes
  for affected UI. Use SVG icons and stylesheets that themes can override, not hand-drawn icons
  or inline colors. In Coin code, read a custom `SoElement` only in actions where it is enabled
  (or check `isElementEnabled()`), and check existing pixel-ratio handling before adding
  scaling, since some modules already scale by `devicePixelRatio`.

## Lean, non-defensive code

Write the code the problem needs and nothing more. Check conditions that can really occur
(user input, document restore, lifecycle, optional geometry), not ones the surrounding code
already rules out. Every guard, fallback and `try` block must correspond to a real case you can
name. Before adding one, read the callee: if its implementation cannot return null or throw for
this input, or the only caller already guarantees the value, do not guard. Reviewers ask "when
is this expected to fail?" for each one.

Python: use `hasattr`/`getattr` only where an attribute can genuinely be missing, such as
adding a property to objects from older files. Add missing properties once on restore, then
access them directly everywhere else:

```python
# Avoid: guards for a property the object always has, and a silent fallback
def execute(self, obj):
    if hasattr(obj, "Parameterization"):
        factor = getattr(obj, "Parameterization", 1.0)

# Prefer: migrate once (see draftobjects/bspline.py), then rely on the property
def onDocumentRestored(self, obj):
    super().onDocumentRestored(obj)
    if not hasattr(obj, "Parameterization"):
        tip = QT_TRANSLATE_NOOP("App::Property", "Parameterization factor")
        obj.addProperty("App::PropertyFloat", "Parameterization", "Draft", tip, locked=True)
        obj.Parameterization = 1.0

def execute(self, obj):
    factor = obj.Parameterization
```

- Check object types with `obj.isDerivedFrom("Part::Feature")`, not by probing attributes. If
  an object must have an attribute, access it and let an `AttributeError` surface.
- Catch the specific exception you handle (`Part.OCCError`, `ValueError`), never a bare
  `except:` or `except Exception: pass`. Let unexpected errors surface.
- Use context managers or `try`/`finally` for cleanup that must happen.
- No `print()` in committed code: use `FreeCAD.Console.PrintMessage/PrintWarning/PrintError` or
  the module's logger (for example `Path.Log`).

C++: rely on invariants the code already establishes, release state with RAII, and let errors
propagate to the layer that reports them:

```cpp
// Avoid: re-checking a member the constructor guarantees, swallowing errors,
// and manual state reset that is skipped on early return or exception
void TaskFoo::apply()
{
    if (!feature) {
        return;
    }
    updating = true;
    try {
        feature->recomputeFeature();
    }
    catch (...) {
    }
    updating = false;
}

// Prefer
void TaskFoo::apply()
{
    Base::StateLocker lock(updating);
    feature->recomputeFeature();
}
```

- RAII helpers that exist: `Base::StateLocker`, `Base::ObjectStatusLocker`, `Base::BitsetLocker`,
  `Base::ScopeGuard`, `App::AutoTransaction`, `QSignalBlocker`, `qScopeGuard`, and
  `Gui::CoinPtr` for Coin node references.
- Ownership is explicit: `std::unique_ptr` (or `std::shared_ptr` when shared) for owning
  pointers, raw pointers or references only for non-owning access, and a reference instead of a
  pointer when null is not a valid value. Use `FC_DISABLE_COPY_MOVE` and related macros for
  special members. In Python C API code, release every new reference on every path. Do not let
  exceptions escape destructors.
- Do not hardcode what the application already provides: version numbers, paths and branding
  (`App::Application::Config()`), colors, fonts and sizes (themes, stylesheets, preferences).
- Capture lambda variables explicitly, not with `[&]`; capture pointers by value.
- Declare a checked pointer in the `if` statement (`if (auto* vp = ...)`) so it cannot be used
  unchecked later.
- No empty `catch` blocks (clang-tidy `bugprone-empty-catch`). In `*PyImp.cpp`, wrap the body in
  `PY_TRY` / `PY_CATCH` (or `PY_CATCH_OCC`) so C++ exceptions become Python exceptions, and
  report argument errors with `PyErr_SetString` and `return nullptr` instead of throwing.
- Do not add compatibility branches for toolchain or library versions the build no longer
  supports, and do not use APIs newer than the oldest versions it still supports (see CMake and
  the CI environments). Put platform-specific code behind the `FC_OS_*` macros.
- Name tolerances and constants: `Precision::Confusion()` (OCC) or `Base::Precision` in code
  without OCC, `Base::toRadians()`, and named `constexpr` values instead of literals.
- Duplicate logic or lambdas become one named function. Do not keep duplication with a
  comment explaining why removing it would be risky; restructure it.

## C++ conventions

- Follow `.clang-format`, `.pre-commit-config.yaml`, and the surrounding module's conventions.
  C++ uses four spaces, no tabs, and a 100-column limit. Format new and substantially rewritten
  code only. Preserve include order and grouping (`SortIncludes` is off); do not mass-sort
  includes or reformat unrelated code. Headers use `#pragma once`. Preserve existing line
  endings; some `.cpp` and `.ui` files use CRLF, so never rewrite files in text mode from scripts.
- Use the C++ standard selected by CMake. Prefer initialized members, `const` correctness,
  `auto*` and `auto&`, `nullptr`, early returns with the main path least indented, and
  `std::ranges`/`<algorithm>` where they improve readability. Logic that belongs to a class goes
  in a named private method rather than a large unnamed block or an anonymous namespace helper.
  Prefer `std::optional` or a small result struct to out parameters, and an enum or flags to
  boolean parameters. Choose containers for realistic sizes: a small `std::vector` often beats
  a `std::set`. Use `emplace_back`, `const auto&` and moves to avoid copies.
- Prefer the standard library over boost in new code (`std::format`, `starts_with`,
  `std::numbers::pi`, `std::filesystem`); the project is reducing its boost dependency.
- Names state intent: no single-letter names outside loops and math, booleans read as
  predicates (`hasDirection`), and a name for every non-obvious expression.
- Include what you use, and remove includes and imports the change leaves unused.

Use FreeCAD's typed helpers instead of manual casts and type checks:

```cpp
// Avoid
if (obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
    auto feature = dynamic_cast<Part::Feature*>(obj);
    auto vp = dynamic_cast<ViewProviderFoo*>(Gui::Application::Instance->getViewProvider(obj));
    gp_Pnt pnt(pos.x, pos.y, pos.z);
}

// Prefer
if (auto* feature = freecad_cast<Part::Feature*>(obj)) {
    auto* vp = Gui::Application::Instance->getViewProvider<ViewProviderFoo>(obj);
    auto pnt = Base::convertTo<gp_Pnt>(pos);
}
```

- `obj->isDerivedFrom<T>()` when the object is not used afterwards, `freecad_cast<T*>(obj)`
  when it is, `qobject_cast` for QObjects. Many getters have a templated variant that does the
  cast; look for one before casting. If it is missing for a call you need repeatedly, add it
  rather than casting at every call site.
- Use the layer's exception and logging facilities (`Base::` exceptions, `FC_THROWM`,
  `Base::Console()`, `FC_ERR`/`FC_WARN`/`FC_LOG`) rather than adding a competing mechanism.
  Exception messages shown to users are translatable and consistent with their siblings.
- Prefer typed Qt signal connections, never `SIGNAL`/`SLOT`. Use `Q_SIGNALS`, `Q_SLOTS`, and
  `Q_EMIT` for the `QT_NO_KEYWORDS` build. Preserve public signatures and established callback
  contracts.
- Literal values are normal in tests. Wrap a C++ test file in
  `// NOLINTBEGIN(readability-magic-numbers, cppcoreguidelines-avoid-magic-numbers)` and the
  matching `NOLINTEND`, as existing tests do, instead of naming every test value.

## Python conventions

- Format with the configured Black rules where `.pre-commit-config.yaml` applies; follow module
  conventions elsewhere. Use only syntax supported by the oldest Python the build accepts (see
  `cMake/FreeCAD_Helpers/SetupPython.cmake` and the CI environments).
- Translation aliases at module top: `translate = FreeCAD.Qt.translate` and
  `QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP`.
- Keep GUI imports out of App code paths. Import at module top; use a lazy import only where
  load order or GUI separation requires it.
- Follow the module's docstring style. Compare numbers explicitly (`== 0`), not with `not`.
- New boolean parameters are keyword-only. Do not change existing public parameters as
  incidental cleanup.

## Comments, headers and generated files

- Explain non-obvious intent, edge cases, workarounds, lifetime, reference paths, coordinate
  spaces, and ordering. Comments read like the surrounding code: no filler and no generated
  phrasing. Do not add comments that restate the code or narrate why an alternative was not
  chosen. No history or status markers (`NEW`, `FIX`, "changed from"), no decorative separator
  lines, no old code kept in comments; git holds the history. Prefer a well-named function over
  a comment that explains a block. Update or remove comments the change makes wrong, keep
  useful explanatory ones, and document public APIs in the Doxygen style the handbook
  describes.
- Keep existing license headers unchanged. The Developers Handbook defines when a new file gets
  a header ([Copyright Headers](https://freecad.github.io/DevelopersHandbook/codeformatting/Copyright-Headers))
  and provides the C++ and Python templates and the SPDX-only forms for other files
  ([Copyright Snippets](https://freecad.github.io/DevelopersHandbook/codeformatting/Copyright-Snippets)).
  The contributor provides the copyright holder; do not invent one.
- Modify generator inputs rather than generated output. Change third-party code, submodules,
  managed translations (`.ts`, `.qm`), visual baselines, or lockfiles only when required by the
  task, following their update process. Do not regenerate or update them to hide unrelated
  failures.

## UI text and translation

- Use established Qt/FreeCAD translation mechanisms with extractable contexts, placeholders,
  plural handling, and translator comments (`//:`) where needed: `tr()` in `Q_OBJECT` classes,
  `QT_TRANSLATE_NOOP` or `QCoreApplication::translate` with a fixed context elsewhere, and
  `Q_DECLARE_TR_FUNCTIONS` for classes in App code that are not QObjects. `QT_TRANSLATE_NOOP`
  only marks a string; it still needs a real translation call where it is shown. Do not
  concatenate translated sentence fragments, put markup inside translatable text, or compute
  contexts that extraction tools cannot recognize; use placeholders and chained `arg()`.
- Apply FEP-0007. Title Case for command names, menu and context menu entries, buttons, window
  and dialog titles, group titles, and transaction names shown in the Undo menu. Sentence case
  for form labels, checkboxes, drop-down entries, tooltips, messages, and other descriptive
  text. Use short, consistent names and user-facing CAD terms (`Part Design`, not `PartDesign`).
- Labels, buttons and titles do not end with a colon, a trailing space or a unit. Use quantity
  widgets for units.
- Tooltips describe the action in third-person present tense. Input hints begin with the
  input placeholder and a short lowercase action (`%1 pick radius`). Use the single `…`
  character only when the action needs further input before it runs (`Save As…`), not merely
  because a dialog opens: `Select Object`, not `Select object…`.
- Property descriptions are distinct for each property so tooltips in the property editor can
  tell them apart.
- Review related strings affected by the change and keep command/dialog terminology aligned.
  Avoid unrelated wording changes that expand the patch and translation workload.

## Tests and validation

- Add focused tests for new behavior and regression coverage for bug fixes when practical.
  Prefer the existing C++ GoogleTest suites under `tests/src`, mirroring the source path, or the
  module's Python test package. A test must exercise what its name claims and fail without the
  fix. Test observable results and relevant unaffected cases, not a copy of the implementation.
  Assertions must be strong enough to catch a wrong result: check the geometry (volume,
  validity, position), not only that a recompute succeeded. Cover the edge cases next to the
  fix, and share setup between cases instead of repeating it.
- Match coverage to risk: save/restore and undo/redo for persisted changes; insertion and
  deletion for dependency rerouting; geometric and non-geometric targets for link commands;
  document switching and closure for task lifetimes. Restore preferences and global state and
  close documents and editors created by tests, also when an assertion fails (`addCleanup`).
  Review Coin snapshot effects when changing rendering.
- Register new tests for both packaging and discovery. Check the module's CMake lists and
  test aggregator; update `FreeCAD.__unit_test__` only when adding a suite that needs it.
- Use the existing configured build and environment. Inspect `pixi.toml`, CMake options, and
  the affected CI workflow instead of assuming tool paths, build directories, or GUI support.
  The Pixi defaults are `pixi run build` and `pixi run test`; prefer the affected targets and
  tests when possible. CTest and Python workbench suites may require separate invocations. Use
  `FreeCADCmd -t <TestModule>` for headless-compatible Python suites; GUI tests need a
  GUI-capable runtime.
- Report commands, outcomes, and remaining validation gaps accurately. Distinguish source
  analysis, compilation, automated tests, manual verification, and CI. Do not claim a test
  passed, a GUI was reproduced, or a PR is validated without supporting evidence.

## Before pushing

Catch locally what CI would otherwise report one push at a time:

- Run the hooks on the changed files: `pixi run pre-commit run --files <files>`. Otherwise
  pre-commit.ci pushes formatting fixups to the PR.
- Build the affected targets with warnings visible and run the affected tests.
- CI also runs clang-tidy (`.clang-tidy`), pylint (`.pylintrc`), codespell and an old-style Qt
  connection check on changed lines. Do not introduce new warnings in lines you touch.
- After changing a `.pyi` binding, run `src/Tools/typing/check-stubs.sh`; CI runs the same
  stub smoke check.
- Confirm each commit builds on its own and the diff contains nothing unrelated.

## Review and delivery

- Review the complete final diff, shared callers, registrations, and adjacent workflows.
  Check for compatibility regressions, ownership errors, unnecessary dependencies, defensive
  code without a real case, duplicated logic, and behavior beyond the agreed scope. For each
  guard, ask whether it can be hit; for each new symbol, whether it is used; for each new
  helper, whether one already exists. Code a reviewer cannot understand on first reading needs
  restructuring, not a longer explanation. A working reproduction is only part of this review.
- After a fix is validated, reassess whether an existing mechanism or simpler design can
  achieve the same behavior with less code. Make necessary improvements within scope without
  introducing unrelated refactoring. If review finds questionable code, fix it rather than
  writing justifications for it.
- For review tasks, prioritize actionable correctness and maintainability findings. Explain
  the trigger and impact with precise code locations; separate uncertainty from confirmed bugs.
  Follow established local style rather than treating every handbook preference as a blocker.
- Inspect actual remotes and the intended target branch. For cherry-picks or rebases, check
  prerequisites, patch equivalence, original authorship, and the resulting diff. Follow
  CONTRIBUTING.md's attribution rules, including co-author trailers where required.
- For CI changes, check affected events, branches, forks, PR/head identity, required checks,
  and result propagation. Avoid duplicate builds, runner-blocking waits, and false success
  when required checks failed, were canceled, or never ran.
- Finish with the change, its reason, validation, and material limitations. Describe partial
  fixes accurately and reserve closing-issue language for issues actually resolved.
