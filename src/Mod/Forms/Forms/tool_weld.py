# SPDX-License-Identifier: LGPL-2.1-or-later
"""Weld tool controls, preview, and commit/cancel lifecycle."""

import FreeCAD as App
import FreeCADGui as Gui
import Part
from .cage import ControlElementMapper, canonical_subelement_name
from .feedback import MODELING_ERRORS, report_modeling_error
from .operations import weld_boundaries
from .taskpanels import load_panel
from .tool_controller import ToolController


class WeldTool(ToolController):
    def __init__(self, session):
        super().__init__(session)
        self.weld_other = None
        self.weld_other_button = None
        self.weld_other_name = None
        self.weld_selecting_other = False
        self.weld_first_edge = None
        self.weld_second_edge = None
        self.weld_first_name = None
        self.weld_second_name = None
        self.weld_apply_button = None
        self.weld_cancel_button = None
        self.weld_status = None

    def start_weld_tool(self):
        """Open the two-Form boundary Weld task handler."""
        if self.session.cleaned or self.session.has_active_tool() or str(self.session.obj.FormType) == "Forms::Surface":
            return False
        self.session._flush_pending_updates()
        self.session.active_tool = "weld"
        widget = load_panel("TaskFormWeld.ui")
        self.weld_other_button = widget.otherButton
        self.weld_other_name = widget.otherName
        self.weld_first_name = widget.firstEdge
        self.weld_second_name = widget.secondEdge
        self.weld_status = widget.status
        self.weld_apply_button = widget.applyButton
        self.weld_cancel_button = widget.cancelButton
        self.weld_other_button.toggled.connect(self._toggle_weld_other_selection)
        self.weld_apply_button.clicked.connect(self.apply_weld_tool)
        self.weld_cancel_button.clicked.connect(self.stop_weld_tool)
        self.session._clear_editor_selection()
        self.session._show_tool_handler(
            App.Qt.translate("Forms_Weld", "Weld"), widget, "Forms_Weld"
        )
        self.session._suspend_selection_for_tool(disable_selection=False)
        if self.session.selection_gate_added:
            Gui.Selection.removeSelectionGate()
            self.session.selection_gate_added = False
            self.session.selection_gate = None
        self.weld_other_button.setChecked(True)
        Gui.HintManager.show(
            Gui.InputHint(
                App.Qt.translate("Forms_Weld", "%1 cancel weld"),
                Gui.UserInput.KeyEscape,
            )
        )
        Gui.Command.update()
        return True

    def _toggle_weld_other_selection(self, active):
        if not self.session.weld_tool_active:
            return
        self.weld_selecting_other = bool(active)
        if active:
            self.weld_status.setText(
                App.Qt.translate("Forms_Weld", "Select the other Form")
            )
        elif self.weld_other is None:
            self.weld_status.setText(
                App.Qt.translate("Forms_Weld", "No other Form selected")
            )
        else:
            self.weld_status.setText(
                App.Qt.translate("Forms_Weld", "Select one opening edge on each Form")
            )

    def _weld_edge_from_event(self, source, subelement):
        raw = str(subelement or "")
        canonical = canonical_subelement_name(raw)
        if not canonical.startswith("Edge"):
            raise ValueError("Select an edge on a free boundary")
        try:
            element = source.Shape.getElement(raw)
        except (Part.OCCError, RuntimeError, ValueError, IndexError):
            element = source.Shape.getElement(canonical)
        mapper = (
            self.session._control_element_mapper()
            if source is self.session.obj
            else ControlElementMapper(source)
        )
        mapped = tuple(sorted(mapper.indices(element)))
        if len(mapped) != 2:
            raise ValueError("The selected shape edge is not a control edge")
        cage = mapper.cage
        if mapped not in set(cage.boundary_edges):
            raise ValueError("Weld edges must belong to free boundaries")
        return mapped

    def _set_weld_input_from_selection(self, document, object_name, subelement):
        doc = self.session._selection_document(document)
        source = doc.getObject(str(object_name)) if doc is not None else None
        if self.weld_selecting_other:
            if (
                source is None
                or source is self.session.obj
                or not str(getattr(source, "FormType", "")).startswith("Forms::")
                or str(source.FormType) == "Forms::Surface"
                or str(getattr(source, "TypeId", "")) != "Part::FeaturePython"
            ):
                self.weld_status.setText(
                    App.Qt.translate("Forms_Weld", "Select another standalone Form")
                )
                return
            self.weld_other = source
            self.weld_other_name.setText(source.Label)
            self.weld_second_edge = None
            self.weld_second_name.clear()
            self.weld_other_button.setChecked(False)
            self.session.suppress_selection_observer = True
            try:
                Gui.Selection.clearSelection()
            finally:
                self.session.suppress_selection_observer = False
            self.weld_apply_button.setEnabled(False)
            return
        if source not in (self.session.obj, self.weld_other):
            self.weld_status.setText(
                App.Qt.translate("Forms_Weld", "Select an edge on one of the two Forms")
            )
            return
        try:
            edge = self._weld_edge_from_event(source, subelement)
        except (Part.OCCError, RuntimeError, ValueError, IndexError) as error:
            self.weld_status.setText(str(error))
            return
        text = f"{edge[0]} - {edge[1]}"
        if source is self.session.obj:
            self.weld_first_edge = edge
            self.weld_first_name.setText(text)
        else:
            self.weld_second_edge = edge
            self.weld_second_name.setText(text)
        ready = self.weld_first_edge is not None and self.weld_second_edge is not None
        self.weld_apply_button.setEnabled(ready)
        self.weld_status.setText(
            App.Qt.translate("Forms_Weld", "Ready to weld")
            if ready
            else App.Qt.translate("Forms_Weld", "Select the opening edge on the other Form")
        )

    def apply_weld_tool(self):
        if (
            not self.session.weld_tool_active
            or self.weld_other is None
            or self.weld_first_edge is None
            or self.weld_second_edge is None
        ):
            return False
        self.session._flush_pending_updates()
        transaction = self.session._begin_action(App.Qt.translate("Forms_Weld", "Weld Forms"))
        try:
            self.session.suppress_selection_observer = True
            try:
                Gui.Selection.clearSelection()
            finally:
                self.session.suppress_selection_observer = False
            if self.session.edit_backup is not None:
                self.session.edit_backup.capture_removal(self.weld_other)
            weld_boundaries(
                self.session.obj,
                self.weld_first_edge,
                self.weld_other,
                self.weld_second_edge,
            )
            self.session.obj.Document.recompute()
            self.session.cached_control_mapper = None
            self.session.cached_control_mapper_signature = None
            self.session.topology_changed()
        except MODELING_ERRORS as error:
            self.session._finish_action(transaction, commit=False)
            return report_modeling_error(
                App.Qt.translate("Forms_Weld", "Weld Forms"), error, self.weld_status
            )
        except Exception:
            self.session._finish_action(transaction, commit=False)
            raise
        self.session._finish_action(transaction)
        self.stop_weld_tool()
        return True

    def stop_weld_tool(self):
        if not self.session.weld_tool_active:
            return
        self.session.active_tool = None
        self.weld_other = None
        self.weld_selecting_other = False
        self.weld_first_edge = None
        self.weld_second_edge = None
        self.weld_other_button = None
        self.weld_other_name = None
        self.weld_first_name = None
        self.weld_second_name = None
        self.weld_apply_button = None
        self.weld_cancel_button = None
        self.weld_status = None
        self.session._resume_selection_after_tool()
        self.session._install_selection_gate()
        self.session._hide_tool_handler()
        if not self.session.cleaned:
            self.session._show_input_hints()
        self.session.view.redraw()
        Gui.Command.update()
