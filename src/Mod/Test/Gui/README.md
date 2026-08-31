# Shared GUI test support

Gui contains the common Python support used by FreeCAD's GUI tests. It is a
small layer over unittest, Qt, and the FreeCAD GUI API. Workbench-specific
helpers should build on it instead of duplicating lifecycle and event-loop
handling.

## Quick start

    from PySide import QtWidgets

    import FreeCAD
    from Gui.TestCase import FreeCADGuiTestCase


    class TestExample(FreeCADGuiTestCase):
        def test_button(self):
            document = FreeCAD.newDocument("GuiExample")
            document.addObject("Part::Feature", "Feature")

            button = self.gui.find_widget(
                QtWidgets.QPushButton,
                object_name="stableButtonName",
                visible_only=True,
            )
            self.assertIsNotNone(button)
            self.gui.click(button, button.rect().center())

FreeCADGuiTestCase creates self.gui for each test. Its teardown exits edit
mode, closes active dialogs, clears selection and preselection, closes
documents created by the test, and processes pending events. Tests should call
the base setUp and tearDown methods.

## Synchronization

Use gui.wait_until for observable state transitions:

    self.gui.wait_until(
        lambda: spin_box.value() == 42,
        timeout_ms=1000,
        description="spin box value",
    )

The predicate is evaluated while Qt events are processed and once more at the
timeout boundary. When a wait fails, GuiHarness.last_wait_diagnostics contains
the current workbench, document, edit object, focus widget, modal, and task
panel.

Use gui.pump only when a short fixed interval is meaningful, such as allowing
an animation to run. Do not use time.sleep in GUI tests because it blocks the
event loop.

## State and lookup

Use the generic ``temporary_preference`` helper for temporary application
settings. All original typed entries for the key are restored on exit:

    from Support import temporary_preference

    with temporary_preference(
        "User parameter:BaseApp/Preferences/Mod/Test",
        "TemporaryInteger",
        42,
    ):
        self.assertEqual(
            FreeCAD.ParamGet(
                "User parameter:BaseApp/Preferences/Mod/Test"
            ).GetInt("TemporaryInteger"),
            42,
        )

Prefer stable selectors in this order: Qt objectName, FreeCAD command or
action identity, widget type with a known parent, accessible properties,
visible translated text, and coordinates as a last resort.

Use gui.find_widget and gui.find_widgets for widget lookup. Use
gui.wait_for_focus when a control may focus an internal child, such as the
line edit inside a spin box. gui.wait_for_task_panel waits for a FreeCAD task
panel.

## Modal commands

For a synchronous command that opens a modal dialog inside its own nested event
loop, use gui.run_with_modal_action:

    state = self.gui.run_with_modal_action(
        lambda: Gui.runCommand("SomeCommand"),
        lambda dialog: dialog.accept(),
    )
    self.assertTrue(state["seen"])

The local watchdog remains active after the deadline so a late modal is
rejected instead of blocking the test. It is stopped automatically when the
operation returns.

## Viewport coordinates

gui.active_view returns a View3D adapter for HiDPI-safe conversion between
FreeCAD's physical viewport coordinates and Qt logical points:

    view = self.gui.active_view()
    view.world_to_screen(FreeCAD.Vector(0, 0, 0))

Use gui.send_mouse, gui.click, gui.right_click, gui.move, and gui.key_click
for input events. View3D intentionally provides coordinate conversion only;
input remains an explicit harness operation.

## C++ QtTest support

The C++ equivalent lives in tests/src/Gui/TestSupport.h under the GuiTest
namespace. It provides ensureGuiApplication, waitUntil, find, and
DocumentGuard. It uses RAII and std::chrono while leaving QtTest assertions
and input primitives available to each test.

Both language APIs share the same principles: wait for state rather than a
fixed delay, prefer stable widget identities, and leave the GUI clean for the
next test. Add a generic helper only when current tests need it or when it is
required for safe cleanup and synchronization.
