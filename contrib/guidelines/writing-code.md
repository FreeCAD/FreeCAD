# Writing maintainable code

[Guide index](README.md) · [Code reuse](code-reuse.md) · [Architecture](architecture.md)

A change is easy to maintain when its reader can identify its purpose, assumptions, ownership,
and effect on existing callers. Fewer lines can help, but removing an essential condition or
reference path makes a short implementation incorrect.

## Establish the contract

Before editing, read the complete function, its class, callers, overrides, and tests. Inspect
history when a condition or ordering looks redundant. Write down the input that fails, the
expected result, and the behavior other inputs must retain. Look at earlier attempts and review
feedback before repeating a solution that was already rejected.

For a regression, follow the linked commit/PR/issue chain, read the intent of earlier changes,
and compare current `main` with the last known-good revision where possible. The
[investigation sequence](reviewing-changes.md#follow-a-regression-back-to-its-intent) explains
how to distinguish evidence from a suspected cause. Ask for a decision if the intended contract
is unclear; do not invent an API or an expected behavior to make the patch possible.

Verify an unfamiliar helper's implementation, not just its name. Determine whether it can
return null, whether it can throw, whether it mutates state, and who owns its return value.
Check namespaces and inherited names explicitly when adding calls across class hierarchies.

For example, a command's enabled-state predicate and execution path should agree about which
objects can be transformed. Fixing just the menu action leaves keyboard or scripted entry paths
inconsistent. The shared selection logic in [CommandDoc.cpp](../../src/Gui/CommandDoc.cpp)
is a useful place to study this kind of contract.

## Make the main path readable

Use names that express domain meaning: a support path, an owning document, a selected instance,
or an insertion context. Split a mixed condition when its intermediate values explain intent.
Prefer early exits for exceptional cases so the normal path remains easy to follow.

The following excerpt needs both values because a captured edit session may have ended:

```python
def reset_edit(document, view_provider):
    gui_document = get_gui_document(document)
    if view_provider is None or gui_document is None:
        return
    if gui_document.getInEdit() is view_provider:
        gui_document.resetEdit()
```

Here `get_gui_document()` is the existing helper in
[draftutils/gui_utils.py](../../src/Mod/Draft/draftutils/gui_utils.py). Checking the identity of
the edited view provider prevents cleanup from canceling a different edit session. Removing
either guard to shorten the function would lose a real lifecycle case.

Extract a named private method when a substantial operation belongs to a class. A local lambda
can express a short operation tied to nearby values. Choose based on responsibility and call
sites, rather than imposing a fixed line count or creating a generic service for one use.

## Guards should describe real cases

Useful guards handle user input, optional geometry, restore of an older document, or an owner
that may have been deleted. A guard is unnecessary when an established invariant already rules
out the condition. Read the callee and callers before deciding which situation applies.

Do not turn a broken invariant into a silent success. For example, falling back to a default
value every time `execute()` reads a missing property can conceal a migration defect. Establish
the property through the owning compatibility mechanism, then use the established schema.
Prefer restore-time migration when the lifecycle supports it. See the
[migration example](documents-and-geometry.md#establish-the-schema-centrally).

Conversely, a document object can genuinely be detached. A `getDocument()` dereference needs
the attachment guarantee appropriate to that path; a null check elsewhere in the class does
not establish it for every caller.

## Let scope restore state

Manual state changes are fragile across exceptions and early returns. For a flag used to block
recursive updates, this excerpt loses the previous value and can leave the flag set:

```cpp
updating = true;
feature->recomputeFeature();
updating = false;
```

When `feature` is a valid borrowed object established by the caller, use the existing scope
helper from [Base/Tools.h](../../src/Base/Tools.h):

```cpp
Base::StateLocker lock(updating);
feature->recomputeFeature();
```

`StateLocker` restores the previous flag value, including when the scope exits exceptionally.
It does not itself prevent entering a recursive operation; that is a separate contract.
The recompute call's result must also be handled when the surrounding operation requires it.

Choose the helper that owns the specific resource: `QSignalBlocker` for Qt signal blocking,
document transactions for undoable mutations, and the existing Coin reference wrapper for
scenegraph references. In Python, use context managers or `try`/`finally` for required cleanup.
Avoid empty exception handlers. Catch an error only where there is a recovery or reporting
decision, and preserve useful failure information.

## Use types and ownership to explain intent

- Initialize state and use `const` where mutation is not needed. Make pointer/reference use
  explicit with `auto*` and `auto&` when using `auto`.
- Use `freecad_cast<T*>` and `isDerivedFrom<T>()` for FreeCAD runtime types, and `qobject_cast`
  for QObjects where appropriate. Inspect templated getters before adding a second cast.
- Prefer a result value or small result struct to unrelated output arguments in new internal
  APIs. Preserve existing public signatures unless changing them is part of the task.
- Make ownership explicit. Qt parent ownership, document ownership, Coin reference counting,
  and Python reference counting have their own contracts; do not put a borrowed object into
  an owning smart pointer. Release Python C API references on every path.
- An enum can clarify a new operation with distinct modes. Replacing persisted booleans or
  changing a Python signature needs compatibility work, not incidental cleanup.

Use standard algorithms when they reveal the operation more clearly than a hand-written loop.
Use moves or emplacement when they remove a real copy; do not mechanically replace every
`push_back`, return, or named temporary. For tiny collections, straightforward linear search
may be clearer and faster than allocating a tree or hash table.

## Respect module and toolchain conventions

Repository configuration defines the supported C++/Python versions and available tools.
Check [.clang-format](../../.clang-format), [.pre-commit-config.yaml](../../.pre-commit-config.yaml),
[.clang-tidy](../../.clang-tidy), and [.pylintrc](../../.pylintrc). Formatter coverage varies by
directory. Format the code being changed without rewriting unrelated code or line endings;
preserve include ordering and grouping. Include what the new implementation uses and remove
includes or imports it makes obsolete.

Keep GUI imports out of Python App paths. Use module-level imports normally and lazy imports
where loading order or optional GUI availability requires them. Match the module's docstrings
and logging facilities. Temporary `print()` statements are not a replacement for useful
diagnostics in shipped code. Do not add compatibility branches for unsupported dependencies.

Prefer typed Qt signal connections and follow the `QT_NO_KEYWORDS` build conventions:
`Q_SIGNALS`, `Q_SLOTS`, and `Q_EMIT` where applicable. Preserve public callback signatures.
Use the layer's existing exception/logging facilities and the established C++/Python exception
conversion at binding boundaries; do not let C++ exceptions escape into Python unchecked.

## Comments, headers, and generated code

Explain facts that the code cannot express well: why a reference retains a container, when a
cleanup can run twice, or which coordinate system an input uses. Preserve useful explanations.
If a block needs a long description of its steps, give the operation a meaningful name first.
Document workarounds with the condition that makes them necessary. Avoid status markers,
commented-out implementations, and comments that merely restate syntax.

Follow the handbook's [header guidance](https://freecad.github.io/DevelopersHandbook/codeformatting/Copyright-Headers)
and [templates](https://freecad.github.io/DevelopersHandbook/codeformatting/Copyright-Snippets).
Preserve existing headers and obtain accurate authorship for new ones. Modify binding specs,
generator inputs, and maintained declarations rather than generated output. Third-party code,
submodules, translation catalogs, baselines, and lockfiles need their own update process.

Before finishing, remove debugging leftovers, inspect the whole diff, and validate against the
original contract using [testing and validation](testing-and-validation.md).
