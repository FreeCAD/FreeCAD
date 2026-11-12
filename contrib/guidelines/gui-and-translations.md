# GUI behavior, language, and translation

[Guide index](README.md) · [Architecture](architecture.md) · [Design proposals](design-proposals.md)

A UI change affects a workflow, not just a widget. Start with the user's task, the state in
which the command is available, and what happens on accept, cancel, failure, and repeated use.
Use the [design guide](https://freecad.github.io/DevelopersHandbook/designguide/) and
[FEP-0007](https://github.com/FreeCAD/FreeCAD-Enhancement-Proposals/blob/master/FEPs/FEP-0007-consistent-language/README.md)
alongside the current implementation.

## Design the interaction before its controls

Identify what the user knows at each step: selected objects, current edit mode, required inputs,
available alternatives, and the result shown. Prefer a familiar FreeCAD interaction over a
hidden gesture with no discoverable alternative. Compare related commands in sibling
workbenches so the same term or icon continues to mean the same operation.

Use other applications as references when useful, but verify their behavior and adapt the idea
to FreeCAD's document and transaction model. A similarity in appearance does not establish
equivalent semantics. Significant workflow changes need the
[appropriate design discussion](design-proposals.md).

Notify the Design Working Group for UI changes and involve it early in substantial interaction
or layout changes. If the UI change also alters standards or CAD workflows, consult the CAD
Working Group as well. Follow the
[working-group and FEP requirements](design-proposals.md#involve-the-responsible-working-groups).

For a preview, define which object owns it, when it updates, and what cancel restores. Distinguish
reference highlighting from the actual candidate result. Test launching with preselection,
changing selection inside the task, accepting, canceling, reopening, and switching documents.
Do not repair preview geometry by changing selection highlighting if they have separate owners.

## Own the document and edit session

The active document is a global UI choice, not a task's permanent owner. During a workbench or
document switch, the destination can become active before the previous workbench finishes
cleanup. Rejecting the currently active task can then cancel another document's operation.

Capture the owning document and relevant edit session at entry, and resolve them safely during
callbacks. Check that they still exist and still identify the session being ended. Do not
retain a GUI wrapper as if its document could never close. Inspect
[DraftGui.py](../../src/Mod/Draft/DraftGui.py) and
[draftutils/gui_utils.py](../../src/Mod/Draft/draftutils/gui_utils.py) for owner-aware cleanup.

Use scoped connections or established observer objects for document/application signals. Qt
connections need their own receiver/lifetime contract. Prefer the event that reports a real
state transition to a timer that assumes the correct order will eventually occur. A local event
filter can be appropriate when a mixin or child widget owns the event; an application-wide
filter should not be the default workaround. Establish which widget actually receives input
before choosing an override or filter.

## Preserve selection, focus, and user choices

Keep a command's `isActive()` checks and execution consistent. The keyboard, menu, toolbar,
and context-menu paths may share one command; fixing just one entry point leaves discrepancies.
Do not accept an object merely because it happens to expose an attribute that resembles the
needed capability. Use the established type and property contract, including read-only state.

Dialog construction can consume preselection to suggest an attachment or initialize input.
Clearing selection earlier or later can therefore alter both support references and focus.
Trace the order before moving a selection step. When hiding a focused task button, inspect
where focus will go; see [TaskView.cpp](../../src/Gui/TaskView/TaskView.cpp).

Toolbar visibility while a workbench is unavailable is temporary state. Persist the user's
choices only from the context that owns them, not after another workbench has hidden the tools.
Check [ToolBarManager.cpp](../../src/Gui/ToolBarManager.cpp) and
[Sketcher's Workbench.cpp](../../src/Mod/Sketcher/Gui/Workbench.cpp) when changing this lifecycle.

## Reuse quantity, preference, and command widgets

Use the existing quantity controls for dimensions rather than adding a unit string to a plain
numeric field. Bind the property through the established mechanism and verify when the unit
is configured. For example, a shortened excerpt from
[PatternPathParametersWidget.cpp](../../src/Mod/Part/Gui/PatternPathParametersWidget.cpp) is:

```cpp
ui->spacing->setUnit(Base::Unit::Length);
ui->startOffset->setUnit(Base::Unit::Length);
ui->endOffset->setUnit(Base::Unit::Length);
ui->spacing->bind(*spacingProperty);
ui->startOffset->bind(*startOffsetProperty);
ui->endOffset->bind(*endOffsetProperty);
```

The property bindings alone do not replace that unit setup. Check the displayed value and
accepted input in another unit system and with expressions where supported. A panel-only
presentation defect usually should not change the persisted property type.

Use a shared command group when its actions already exist. Keep ordering and terminology
aligned across menu, toolbar, and consumers. Do not duplicate group construction in each
workbench when one module already owns the group.

## Apply the language rules in context

FEP-0007 is the reference for source-language UI text. Common applications include:

| Element | Style | Example |
| --- | --- | --- |
| Command, button, menu, dialog title | Title Case and concise naming | `Path Pattern` |
| Form label or option | Sentence case | `Reverse direction` |
| Tooltip | Action described in third-person present tense | `Distributes copies along the selected path` |
| Input hint | Input placeholder, then a brief lowercase action | `%1 choose origin` |

Use consistent names across the command, task title, and related controls. Prefer natural
plurals to `(s)`, avoid developer identifiers in user text, and omit trailing label colons.
Short labels do not need a period; longer explanatory sentences do. Use the single ellipsis
character only where the action's wording calls for missing input, not merely because a dialog
opens. Keep changes to related text within the task's scope to limit translation churn.

## Marking text is not translating it

Use a fixed, extractable context. In a `Q_OBJECT` class, `tr()` uses the class context. Elsewhere,
use the established translation mechanism for that layer. This illustrative pair marks a string
for extraction and translates it at use:

```cpp
const char* label = QT_TRANSLATE_NOOP("ExamplePanel", "Reverse direction");
const QString text = QCoreApplication::translate("ExamplePanel", label);
```

The literal in `QT_TRANSLATE_NOOP` is discoverable; the macro does not perform translation.
Keep the context identical at lookup. For classes that use `Q_DECLARE_TR_FUNCTIONS`, follow
the existing context. In Python, use the module's translation helper or established aliases.

Keep complete sentences and placeholders together so translators can reorder them. Use Qt's
plural-aware mechanism for counts where appropriate; use placeholders for values and markup
instead of splitting a sentence across translated fragments. Add translator comments for
ambiguous terms. Do not edit managed `.ts`/`.qm` output by hand.

## Check accessibility, layout, and rendering

Keep labels associated with their controls, tab order useful, and actions available through
normal keyboard interaction. Do not use color as the only indication of a result or error.
Prefer themed SVG icons and existing palette/style mechanisms to hardcoded colors or metrics.
Use layouts that can accommodate translated text and scaled fonts; a fixed local-language
dialog size is not a layout contract.

Check light/dark themes and relevant scale factors. In Coin code, a custom element must be
enabled for the action that reads it. Check existing device-pixel-ratio handling before adding
another scale factor. A fix that makes one overlay sharp can double-scale another consumer.
Review selection, picking, and snapshots when changing shared nodes.

The human author should inspect the final affected workflow, not only a designer screenshot.
Record before/after UI evidence for the contribution, including cancellation and error states
when those changed. See [testing and validation](testing-and-validation.md) for the distinction
between automated coverage and human verification.
