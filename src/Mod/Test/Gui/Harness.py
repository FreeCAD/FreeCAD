# SPDX-License-Identifier: LGPL-2.1-or-later

"""FreeCAD-specific lifecycle, synchronization, and input helpers."""

from __future__ import annotations

from collections.abc import Callable
import time
from typing import Any, TypedDict, TypeVar, overload

import FreeCAD

from . import Wait
from .View3D import View3D

FreeCADGui = Wait.FreeCADGui
QtCore = Wait.QtCore
QtGui = Wait.QtGui
QtWidgets = Wait.QtWidgets

WidgetT = TypeVar("WidgetT")


class ModalActionState(TypedDict, total=False):
    """Status returned by GuiHarness.run_with_modal_action."""

    seen: bool
    expired: bool
    error: BaseException


class GuiHarness:
    """GUI services shared by Python unittest tests.

    The harness owns per-test cleanup and provides the primitives used by
    current GUI tests: event processing, state-based waiting, focus and
    task-panel lookup, edit-mode entry, input events, and viewport access.
    """

    def __init__(self) -> None:
        self._documents_before_test: set[str] = set()
        self.last_wait_diagnostics: str = ""

    def set_up(self) -> None:
        """Record initial documents, show the main window, and flush events."""
        self._documents_before_test = set(FreeCAD.listDocuments())
        main_window = FreeCADGui.getMainWindow()
        if main_window is not None:
            main_window.show()
        self.flush_gui()

    def tear_down(self) -> None:
        """Restore GUI state and close documents created by the test."""
        if not Wait.gui_available():
            return

        try:
            self._close_dialog()
            active = FreeCADGui.ActiveDocument
            if active is not None and active.getInEdit() is not None:
                active.resetEdit()
            FreeCADGui.Selection.clearSelection()
            FreeCADGui.Selection.clearPreselection()
            self.flush_gui(80)
        finally:
            for name in list(FreeCAD.listDocuments()):
                if name not in self._documents_before_test:
                    try:
                        FreeCAD.closeDocument(name)
                    except RuntimeError:
                        pass
            try:
                FreeCADGui.Selection.clearSelection()
                FreeCADGui.Selection.clearPreselection()
                self.flush_gui(80)
            except RuntimeError:
                pass

    def pump(self, timeout_ms: int = 50) -> None:
        """Process Qt events for approximately timeout_ms milliseconds."""
        Wait.pump(timeout_ms)

    def flush_gui(self, timeout_ms: int = 0) -> None:
        """Flush pending Qt and FreeCAD GUI work."""
        Wait.flush_gui(timeout_ms)

    def wait_until(
        self,
        predicate: Wait.Predicate,
        timeout_ms: int = 1000,
        step_ms: int = 10,
        description: str | None = None,
    ) -> bool:
        """Wait for a state predicate while continuing to process GUI events."""
        result = Wait.wait_until(predicate, timeout_ms, step_ms)
        if not result:
            self.last_wait_diagnostics = self.diagnostics(description)
        return result

    def enter_edit(
        self,
        document: FreeCAD.Document,
        object_name: str,
        timeout_ms: int = 1000,
    ) -> Any:
        """Enter object_name edit mode and verify the exact target.

        If another object is already being edited, callers must reset that
        session explicitly before requesting a different object.
        """
        if FreeCADGui is None or not Wait.gui_available():
            raise RuntimeError("Cannot enter edit mode without a usable GUI")

        try:
            gui_document = FreeCADGui.getDocument(document.Name)
        except (AttributeError, RuntimeError) as exc:
            raise RuntimeError(f"Cannot find GUI document {document.Name!r}") from exc
        if gui_document is None:
            raise RuntimeError(f"Cannot find GUI document {document.Name!r}")

        current_object_name = self._edited_object_name(gui_document)
        if current_object_name is not None and current_object_name != object_name:
            raise RuntimeError(
                f"Cannot enter edit mode for {document.Name}.{object_name}: "
                f"{document.Name}.{current_object_name} is already being edited"
            )

        gui_document.setEdit(object_name)
        if not self.wait_until(
            lambda: self._edited_object_name(gui_document) == object_name,
            timeout_ms=timeout_ms,
            description=f"edit mode for {document.Name}.{object_name}",
        ):
            raise RuntimeError(f"Timed out entering edit mode for {document.Name}.{object_name}")
        self.flush_gui()
        return gui_document

    @staticmethod
    def _edited_object_name(gui_document: Any) -> str | None:
        """Return the document-object name currently being edited."""
        edit_view_provider = gui_document.getInEdit()
        if edit_view_provider is None:
            return None
        edit_object = getattr(edit_view_provider, "Object", None)
        return getattr(edit_object, "Name", None)

    def main_window(self) -> Any | None:
        """Return the FreeCAD main window, if available."""
        return FreeCADGui.getMainWindow() if Wait.gui_available() else None

    def find_widget(
        self,
        widget_type: type[WidgetT],
        parent: Any | None = None,
        object_name: str | None = None,
        visible_only: bool = False,
    ) -> WidgetT | None:
        """Find the first matching descendant of parent."""
        if parent is None:
            parent = self.main_window()
        if parent is None:
            return None

        if object_name is not None:
            widget = parent.findChild(widget_type, object_name)
            if widget is not None and (not visible_only or widget.isVisible()):
                return widget
            return None

        widgets = parent.findChildren(widget_type)
        return next(
            (widget for widget in widgets if not visible_only or widget.isVisible()),
            None,
        )

    def find_widgets(
        self,
        widget_type: type[WidgetT],
        parent: Any | None = None,
        visible_only: bool = False,
    ) -> list[WidgetT]:
        """Return all matching descendants, optionally visible ones."""
        if parent is None:
            parent = self.main_window()
        if parent is None:
            return []
        widgets = parent.findChildren(widget_type)
        return [widget for widget in widgets if not visible_only or widget.isVisible()]

    @overload
    def focused_widget(
        self,
        widget_type: type[WidgetT],
        parent: Any | None = None,
    ) -> WidgetT | None: ...

    @overload
    def focused_widget(
        self,
        widget_type: None = None,
        parent: Any | None = None,
    ) -> Any | None: ...

    def focused_widget(
        self,
        widget_type: type[WidgetT] | None = None,
        parent: Any | None = None,
    ) -> Any | None:
        """Return the focused widget or its matching widget ancestor."""
        app = Wait.qt_application()
        widget = app.focusWidget() if app is not None else None

        while widget is not None:
            matches_type = widget_type is None or isinstance(widget, widget_type)
            matches_parent = parent is None or widget is parent or parent.isAncestorOf(widget)
            if matches_type and matches_parent:
                return widget
            widget = widget.parentWidget()
        return None

    @overload
    def wait_for_focus(
        self,
        widget_type: type[WidgetT],
        parent: Any | None = None,
        timeout_ms: int = 1000,
    ) -> WidgetT | None: ...

    @overload
    def wait_for_focus(
        self,
        widget_type: None = None,
        parent: Any | None = None,
        timeout_ms: int = 1000,
    ) -> Any | None: ...

    def wait_for_focus(
        self,
        widget_type: type[WidgetT] | None = None,
        parent: Any | None = None,
        timeout_ms: int = 1000,
    ) -> Any | None:
        """Wait until a widget or matching ancestor owns keyboard focus."""
        found: list[Any] = []

        def find() -> bool:
            widget = self.focused_widget(widget_type, parent)
            if widget is not None:
                found.append(widget)
                return True
            return False

        if self.wait_until(find, timeout_ms, description="focus change"):
            return found[0]
        return None

    def active_task_panel(self) -> Any | None:
        """Return FreeCAD's active task panel, if one is open."""
        return FreeCADGui.Control.activeTaskDialog()

    def wait_for_task_panel(self, timeout_ms: int = 1000) -> Any | None:
        """Wait for and return FreeCAD's active task panel."""
        found: list[Any] = []

        def find() -> bool:
            panel = self.active_task_panel()
            if panel is not None:
                found.append(panel)
                return True
            return False

        if self.wait_until(find, timeout_ms, description="task panel"):
            return found[0]
        return None

    def _active_modal(self) -> Any | None:
        app = Wait.qt_application()
        return app.activeModalWidget() if app is not None else None

    def _close_dialog(self) -> None:
        """Close an active FreeCAD task dialog or modal widget."""
        if FreeCADGui.Control.activeDialog() is not None:
            FreeCADGui.Control.closeDialog()
            self.flush_gui(80)

        modal = self._active_modal()
        if modal is not None:
            modal.reject()
            self.flush_gui(80)

    def run_with_modal_action(
        self,
        operation: Callable[[], object],
        action: Callable[[Any], None],
        delay_ms: int = 0,
        timeout_ms: int = 1000,
    ) -> ModalActionState:
        """Run a synchronous operation while safely handling one modal.

        A local timer watches the operation's nested event loop. It remains
        active after the deadline so a late modal is rejected instead of
        blocking the test. The timer is always stopped when operation returns.
        """
        state: ModalActionState = {"seen": False, "expired": False}
        deadline = time.monotonic() + max(0, timeout_ms) / 1000.0
        not_before = time.monotonic() + max(0, delay_ms) / 1000.0
        stopped = False
        timer = QtCore.QTimer()
        timer.setInterval(10)

        def stop() -> None:
            nonlocal stopped
            if stopped:
                return
            stopped = True
            timer.stop()
            timer.deleteLater()

        def check_modal() -> None:
            if stopped:
                return
            now = time.monotonic()
            if now < not_before:
                return

            dialog = self._active_modal()
            if dialog is not None:
                if now >= deadline:
                    state["expired"] = True
                    try:
                        dialog.reject()
                    except (AttributeError, RuntimeError):
                        pass
                    return

                state["seen"] = True
                try:
                    action(dialog)
                except Exception as exc:
                    state["error"] = exc
                    try:
                        dialog.reject()
                    except (AttributeError, RuntimeError):
                        pass
                finally:
                    stop()
            elif now >= deadline:
                state["expired"] = True

        timer.timeout.connect(check_modal)
        timer.start()
        try:
            operation()
        finally:
            if not state["seen"] and time.monotonic() >= deadline:
                state["expired"] = True
            stop()
        return state

    def active_view(self) -> View3D:
        """Return a HiDPI-aware adapter for the active FreeCAD view."""
        return View3D(FreeCADGui.ActiveDocument.ActiveView)

    @staticmethod
    def _focus_for_click(widget: Any) -> None:
        window = widget.window()
        if window is not None:
            window.activateWindow()

        focus_target = widget
        while focus_target is not None:
            if focus_target.focusPolicy() != QtCore.Qt.NoFocus:
                focus_target.setFocus(QtCore.Qt.MouseFocusReason)
                break
            focus_target = focus_target.parentWidget()

    def click(self, widget: Any, pos: Any) -> None:
        """Send a left click to widget and process deferred work."""
        self._focus_for_click(widget)
        self.send_mouse(
            widget,
            QtCore.QEvent.MouseButtonPress,
            pos,
            QtCore.Qt.LeftButton,
            QtCore.Qt.LeftButton,
        )
        self.send_mouse(
            widget,
            QtCore.QEvent.MouseButtonRelease,
            pos,
            QtCore.Qt.LeftButton,
            QtCore.Qt.NoButton,
        )
        self.pump(120)

    def right_click(self, widget: Any, pos: Any) -> None:
        """Send a right click to widget and process deferred work."""
        self._focus_for_click(widget)
        self.send_mouse(
            widget,
            QtCore.QEvent.MouseButtonPress,
            pos,
            QtCore.Qt.RightButton,
            QtCore.Qt.RightButton,
        )
        self.send_mouse(
            widget,
            QtCore.QEvent.MouseButtonRelease,
            pos,
            QtCore.Qt.RightButton,
            QtCore.Qt.NoButton,
        )
        self.pump(120)

    def move(self, widget: Any, pos: Any) -> None:
        """Move the pointer to pos in widget and flush hover work."""
        self.send_mouse(
            widget, QtCore.QEvent.MouseMove, pos, QtCore.Qt.NoButton, QtCore.Qt.NoButton
        )
        self.pump(80)

    @staticmethod
    def send_mouse(widget: Any, event_type: Any, pos: Any, button: Any, buttons: Any) -> None:
        """Send one synthetic Qt mouse event to widget."""
        global_pos = widget.mapToGlobal(pos)
        event = QtGui.QMouseEvent(
            event_type,
            pos,
            global_pos,
            button,
            buttons,
            QtCore.Qt.NoModifier,
        )
        QtWidgets.QApplication.sendEvent(widget, event)

    @staticmethod
    def key_click(widget: Any, key: int, text: str = "") -> None:
        """Send a synthetic key press/release pair to widget."""
        press = QtGui.QKeyEvent(QtCore.QEvent.KeyPress, key, QtCore.Qt.NoModifier, text)
        release = QtGui.QKeyEvent(QtCore.QEvent.KeyRelease, key, QtCore.Qt.NoModifier, text)
        QtWidgets.QApplication.sendEvent(widget, press)
        QtWidgets.QApplication.sendEvent(widget, release)

    @staticmethod
    def clamp_to_widget(widget: Any, pos: Any, margin: int = 10) -> Any:
        """Clamp a Qt point inside widget while preserving margin."""
        rect = widget.rect()
        return QtCore.QPoint(
            max(margin, min(pos.x(), rect.right() - margin)),
            max(margin, min(pos.y(), rect.bottom() - margin)),
        )

    def diagnostics(self, waiting_for: str | None = None) -> str:
        """Return a readable snapshot of the current GUI state."""
        lines: list[str] = []
        if waiting_for:
            lines.append(f"Waiting for: {waiting_for}")
        try:
            lines.append(f"Workbench: {FreeCADGui.activeWorkbench().name()}")
        except (AttributeError, RuntimeError):
            lines.append("Workbench: unavailable")
        try:
            document = FreeCAD.ActiveDocument
            lines.append(f"Document: {document.Name if document else 'none'}")
        except (AttributeError, RuntimeError):
            lines.append("Document: unavailable")
        try:
            edit = FreeCADGui.ActiveDocument.getInEdit() if FreeCADGui.ActiveDocument else None
            lines.append(f"Edit object: {edit.Name if edit else 'none'}")
        except (AttributeError, RuntimeError):
            lines.append("Edit object: unavailable")
        try:
            focus = QtWidgets.QApplication.focusWidget()
            lines.append(f"Focus widget: {focus.objectName() if focus else 'none'}")
        except (AttributeError, RuntimeError):
            lines.append("Focus widget: unavailable")
        modal = self._active_modal()
        lines.append(f"Active modal: {type(modal).__name__ if modal else 'none'}")
        try:
            task_panel = self.active_task_panel()
            lines.append(f"Task panel: {type(task_panel).__name__ if task_panel else 'none'}")
        except (AttributeError, RuntimeError):
            lines.append("Task panel: unavailable")
        return "\n".join(lines)
