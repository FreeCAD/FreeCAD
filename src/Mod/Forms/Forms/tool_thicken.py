# SPDX-License-Identifier: LGPL-2.1-or-later
"""Thicken tool controls, preview, and commit/cancel lifecycle."""

import FreeCAD as App
import FreeCADGui as Gui
from .capabilities import require_base_topology
from .cage import ControlCage
from .feedback import MODELING_ERRORS, report_modeling_error
from .taskpanels import load_panel
from PySide import QtCore
from .tool_controller import ToolController


class ThickenTool(ToolController):
    def __init__(self, session):
        super().__init__(session)
        self.thicken_original_cage = None
        self.thicken_original_mode = None
        self.thicken_transaction_open = False
        self.thicken_distance = None
        self.thicken_apply_button = None
        self.thicken_cancel_button = None
        self.thicken_update_timer = QtCore.QTimer()
        self.thicken_update_timer.setSingleShot(True)
        self.thicken_update_timer.setInterval(300)
        self.thicken_update_timer.timeout.connect(self._apply_thicken_preview)

    def start_thicken_tool(self):
        """Start a debounced, cage-native Thicken preview."""
        if self.session.cleaned or self.session.has_active_tool():
            return False
        require_base_topology(self.session.obj, "Thicken")
        cage = ControlCage.from_object(self.session.obj)
        if cage.is_closed:
            raise ValueError("Thicken currently requires an open Form surface")
        if getattr(self.session.obj, "LocalEdgeInserts", ()) or str(
            getattr(self.session.obj, "TMeshData", "") or ""
        ):
            raise ValueError("Thicken does not yet support local edge inserts")
        self.session._flush_pending_updates()
        self.session._clear_editor_selection()
        self.thicken_original_cage = cage
        self.thicken_original_mode = str(self.session.obj.CageMode)
        self.thicken_transaction_open = self.session._begin_action(
            App.Qt.translate("Forms_Edit", "Thicken form")
        )
        self.session.active_tool = "thicken"
        widget = self._create_thicken_tool_widget()
        self.session._show_tool_handler(
            App.Qt.translate("Forms_Edit", "Thicken"),
            widget,
            "Forms_Thicken",
        )
        self.session._suspend_selection_for_tool()
        Gui.HintManager.show(
            Gui.InputHint(
                App.Qt.translate("Forms_Edit", "%1 cancel thicken"),
                Gui.UserInput.KeyEscape,
            )
        )
        try:
            self._apply_thicken_preview()
        except Exception:
            self.stop_thicken_tool(apply=False)
            raise
        return True

    def _create_thicken_tool_widget(self):
        widget = load_panel("TaskFormThicken.ui")
        self.thicken_distance = widget.thickness
        bounds = self.session.obj.Shape.BoundBox
        default_thickness = max(bounds.XLength, bounds.YLength, bounds.ZLength) * 0.1
        self.thicken_distance.setValue(max(default_thickness, 0.1))
        self.thicken_distance.valueChanged.connect(self._queue_thicken_preview)
        widget.reverseButton.clicked.connect(self._reverse_thicken_direction)
        self.thicken_apply_button = widget.applyButton
        self.thicken_cancel_button = widget.cancelButton
        self.thicken_apply_button.clicked.connect(lambda: self.stop_thicken_tool(apply=True))
        self.thicken_cancel_button.clicked.connect(lambda: self.stop_thicken_tool(apply=False))
        return widget

    def _reverse_thicken_direction(self):
        if self.thicken_distance is not None:
            self.thicken_distance.setValue(-self.thicken_distance.value())

    def _queue_thicken_preview(self, _value):
        if self.session.thicken_tool_active and not self.session.cleaned:
            self.thicken_update_timer.start()

    def _apply_thicken_preview(self):
        if self.session.cleaned or not self.session.thicken_tool_active or self.thicken_original_cage is None:
            return False
        distance = float(self.thicken_distance.value())
        self.thicken_apply_button.setEnabled(abs(distance) > 1.0e-9)
        if abs(distance) <= 1.0e-9:
            return False
        try:
            cage = self.thicken_original_cage.thickened(distance, sharp=True)
            cage.write(self.session.obj)
            self.session.obj.CageMode = "Editable"
            self.session.obj.touch()
            self.session.obj.Document.recompute()
            self.session._set_parametric_state(False)
            self.session._sync_dimension_properties()
        except MODELING_ERRORS as error:
            self.thicken_apply_button.setEnabled(False)
            return report_modeling_error(
                App.Qt.translate("Forms_Thicken", "Thicken"), error
            )
        return True

    def stop_thicken_tool(self, apply=False):
        """Apply or restore the temporary Thicken result."""
        if not self.session.thicken_tool_active:
            return
        self.thicken_update_timer.stop()
        try:
            if apply:
                self._apply_thicken_preview()
            elif self.thicken_original_cage is not None:
                self.thicken_original_cage.write(self.session.obj)
                self.session.obj.CageMode = self.thicken_original_mode
                self.session.obj.touch()
                self.session.obj.Document.recompute()
        except Exception:
            self.session._finish_action(self.thicken_transaction_open, commit=False)
            self.thicken_transaction_open = False
            raise
        self.session._finish_action(self.thicken_transaction_open, commit=apply)
        self.thicken_transaction_open = False
        self.session.active_tool = None
        self.thicken_original_cage = None
        self.thicken_original_mode = None
        self.session._hide_tool_handler()
        self.thicken_distance = None
        self.thicken_apply_button = None
        self.thicken_cancel_button = None
        self.session._resume_selection_after_tool()
        if not self.session.cleaned:
            self.session._set_parametric_state(self.session.obj.CageMode == "Parametric")
            self.session._configure_symmetry(apply=bool(self.session.obj.Symmetric))
            self.session._sync_dimension_properties()
            self.session._update_dimension_gizmos()
            self.session._show_input_hints()
        self.session.view.redraw()
