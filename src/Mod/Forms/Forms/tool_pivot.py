# SPDX-License-Identifier: LGPL-2.1-or-later
"""Pivot tool controls, preview, and commit/cancel lifecycle."""

import FreeCAD as App
import FreeCADGui as Gui
from PySide import QtCore, QtWidgets
from pivy import coin
from .tool_controller import ToolController


class PivotTool(ToolController):
    def __init__(self, session):
        super().__init__(session)
        self.pivot_tool_callback = None
        self.pivot_tool_mouse_callback = None
        self.pivot_snap_point = None
        self.pivot_previous_selection_filter = None
        self.pivot_selection_snapshot = None
        self.pivot_pick_pending = False

    def start_set_pivot_tool(self):
        """Pick a temporary transform origin without changing the selection."""
        if self.session.cleaned or self.session.has_active_tool() or not self.session.selected:
            return False
        self.session._flush_pending_updates()
        self.session.active_tool = "set_pivot"
        self.pivot_snap_point = None
        self.pivot_pick_pending = False
        self.pivot_selection_snapshot = [
            (selection.Object, tuple(selection.SubElementNames))
            for selection in Gui.Selection.getSelectionEx()
        ]
        self.pivot_previous_selection_filter = self.session.selection_filter.currentIndex()
        all_index = self.session.selection_filter.findData("All")
        if all_index >= 0:
            blocker = QtCore.QSignalBlocker(self.session.selection_filter)
            self.session.selection_filter.setCurrentIndex(all_index)
            del blocker
            self.session._install_selection_gate()
        self.session.set_pivot_button.setEnabled(False)

        widget = QtWidgets.QWidget()
        layout = QtWidgets.QVBoxLayout(widget)
        layout.setContentsMargins(0, 0, 0, 0)
        message = QtWidgets.QLabel(
            App.Qt.translate(
                "Forms_SetPivot",
                "Click a snapped point to place the transform pivot. The current selection "
                "will be preserved.",
            ),
            widget,
        )
        message.setWordWrap(True)
        layout.addWidget(message)
        self.session._show_tool_handler(
            App.Qt.translate("Forms_SetPivot", "Set Pivot"),
            widget,
            "Forms_SetPivot",
        )
        # Keep native picking enabled as a fallback. Some navigation styles
        # consume the mouse event before a Pivy callback can handle it; the
        # selection observer then supplies the picked 3D point and restores
        # the original selection immediately afterward.
        self.session._suspend_selection_for_tool(hide_dragger=False, disable_selection=False)
        try:
            if not hasattr(Gui, "Snapper"):
                from draftguitools.gui_snapper import Snapper

                Gui.Snapper = Snapper()
            if hasattr(self.session.view, "activateToolHandler"):
                self.session.view.activateToolHandler("Forms_Pointer_SetPivot")
            self.pivot_tool_callback = self.session.view.addEventCallback(
                "SoEvent", self._pivot_tool_event
            )
            self.pivot_tool_mouse_callback = self.session.view.addEventCallbackPivy(
                coin.SoMouseButtonEvent.getClassTypeId(),
                self._pivot_tool_mouse_event,
            )
        except Exception:
            self.stop_set_pivot_tool()
            raise
        Gui.HintManager.show(
            Gui.InputHint(
                App.Qt.translate("Forms_SetPivot", "%1 place the transform pivot"),
                Gui.UserInput.MouseLeft,
            ),
            Gui.InputHint(
                App.Qt.translate("Forms_SetPivot", "%1 cancel setting the pivot"),
                Gui.UserInput.MouseRight,
            ),
        )
        Gui.Command.update()
        return True

    def stop_set_pivot_tool(self):
        """Dismiss the temporary pivot picker without changing its selection."""
        if (
            not self.session.pivot_tool_active
            and self.pivot_tool_callback is None
            and self.pivot_tool_mouse_callback is None
        ):
            return
        self.session.active_tool = None
        if self.pivot_tool_callback is not None:
            try:
                self.session.view.removeEventCallback("SoEvent", self.pivot_tool_callback)
            except (AttributeError, RuntimeError):
                pass
            self.pivot_tool_callback = None
        if self.pivot_tool_mouse_callback is not None:
            try:
                self.session.view.removeEventCallbackPivy(
                    coin.SoMouseButtonEvent.getClassTypeId(),
                    self.pivot_tool_mouse_callback,
                )
            except (AttributeError, RuntimeError):
                pass
            self.pivot_tool_mouse_callback = None
        if hasattr(Gui, "Snapper"):
            try:
                Gui.Snapper.off()
            except (AttributeError, RuntimeError):
                pass
        if hasattr(self.session.view, "deactivateToolHandler"):
            try:
                self.session.view.deactivateToolHandler()
            except (AttributeError, RuntimeError):
                pass
        self.pivot_snap_point = None
        self.pivot_pick_pending = False
        self.session._resume_selection_after_tool()
        if self.pivot_previous_selection_filter is not None:
            blocker = QtCore.QSignalBlocker(self.session.selection_filter)
            self.session.selection_filter.setCurrentIndex(self.pivot_previous_selection_filter)
            del blocker
            self.pivot_previous_selection_filter = None
            self.session._install_selection_gate()
        self._restore_pivot_selection()
        self.session.set_pivot_button.setEnabled(bool(self.session.selected))
        self.session._hide_tool_handler()
        if not self.session.cleaned:
            self.session._show_input_hints()
        self.session.view.redraw()
        Gui.Command.update()

    def _snap_pivot_point(self, position):
        if not hasattr(Gui, "Snapper"):
            return None
        # This Pivy build exposes SbVec2s.getValue() as a raw SWIG pointer.
        # Indexing the vector is portable and also avoids the same conversion
        # in Draft Snapper, which expects an ordinary two-item tuple here.
        if isinstance(position, coin.SbVec2s):
            position = (int(position[0]), int(position[1]))
        if isinstance(position, (tuple, list)) and len(position) != 2:
            return None
        try:
            point = Gui.Snapper.snap(position, active=True, noTracker=False)
            info = Gui.Snapper.snapInfo
        except (AttributeError, RuntimeError, TypeError, ValueError):
            return None
        if point is None or not info or not info.get("Object"):
            tracker = getattr(Gui.Snapper, "tracker", None)
            if tracker is not None:
                tracker.off()
            return None
        # Snapper reports document-global coordinates, while the dragger is a
        # child of this ViewProvider's local scene graph.
        return self.session._global_placement(self.session.obj).inverse().multVec(App.Vector(point))

    def _pivot_selection_added(self, position):
        """Accept a native pick while preserving the edit selection."""
        if self.pivot_pick_pending:
            return
        try:
            point = App.Vector(
                float(position[0]),
                float(position[1]),
                float(position[2]),
            )
        except (IndexError, TypeError, ValueError):
            return
        point = self.session._global_placement(self.session.obj).inverse().multVec(point)
        self.pivot_pick_pending = True
        self.session._later(0, lambda picked=point: self._complete_pivot_pick(picked))

    def _restore_pivot_selection(self):
        snapshot = self.pivot_selection_snapshot
        if snapshot is None:
            return
        self.pivot_selection_snapshot = None
        self.session.suppress_selection_observer = True
        try:
            Gui.Selection.clearSelection()
            for obj, subelements in snapshot:
                if subelements:
                    Gui.Selection.addSelection(obj, list(subelements))
                else:
                    Gui.Selection.addSelection(obj)
        finally:
            self.session.suppress_selection_observer = False

    def _complete_pivot_pick(self, point):
        if self.session.cleaned or not self.session.pivot_tool_active:
            return
        self.session.base_points = self.session._all_control_points()
        self.session.base_center = App.Vector(point)
        self.session.syncing = True
        self.session.dragger.translation.setValue(point.x, point.y, point.z)
        self.session.dragger.planarScaleFactor.setValue(1.0, 1.0, 1.0)
        self.session.syncing = False
        self.session._update_dragger_scale()
        self.stop_set_pivot_tool()

    def _pivot_tool_event(self, info):
        if self.session.cleaned or not self.session.pivot_tool_active:
            return
        event_type = info.get("Type")
        if event_type == "SoKeyboardEvent" and info.get("State") == "DOWN":
            if str(info.get("Key", "")).upper() == "ESCAPE":
                self.stop_set_pivot_tool()
            return
        if event_type == "SoLocation2Event":
            self.pivot_snap_point = self._snap_pivot_point(tuple(info.get("Position", ())))

    def _pivot_tool_mouse_event(self, event_callback):
        """Own pivot-picking clicks so they never alter the selection."""
        if self.session.cleaned or not self.session.pivot_tool_active:
            return
        event = event_callback.getEvent()
        if event.getState() != coin.SoButtonEvent.DOWN:
            return
        button = event.getButton()
        if button == coin.SoMouseButtonEvent.BUTTON2:
            event_callback.setHandled()
            self.session._later(0, self.stop_set_pivot_tool)
            return
        if button != coin.SoMouseButtonEvent.BUTTON1:
            return
        event_callback.setHandled()
        position = event.getPosition()
        point = self._snap_pivot_point(position)
        if point is None:
            point = self.pivot_snap_point
        if point is None:
            return
        self._complete_pivot_pick(point)
