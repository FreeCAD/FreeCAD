# SPDX-License-Identifier: LGPL-2.1-or-later
"""Straighten tool controls, preview, and commit/cancel lifecycle."""

import FreeCAD as App
import FreeCADGui as Gui
import Part
from .cage import ControlCage, canonical_subelement_name
from .feedback import MODELING_ERRORS, report_modeling_error
from .operations import (
    preview_straighten_control_points,
    preview_straighten_surface_points,
    straighten_control_points,
)
from .taskpanels import load_panel
from .topology import cage_vertex_range
from .tool_controller import ToolController


class StraightenTool(ToolController):
    def __init__(self, session):
        super().__init__(session)
        self.straighten_indices = []
        self.straighten_mode = None
        self.straighten_type = None
        self.straighten_range = None
        self.straighten_references = []
        self.straighten_reference_button = None
        self.straighten_reference_name = None
        self.straighten_reference_widget = None
        self.straighten_reference_selecting = False
        self.straighten_apply_button = None
        self.straighten_cancel_button = None
        self.straighten_preview_status = None
        self.straighten_preview_root = None
        self.straighten_preview_switch = None
        self.straighten_preview_shape = Part.Shape()
        self.straighten_visibility_before = []

    def start_straighten_tool(self, indices):
        """Open Fusion-style Straighten for the selected cage controls."""
        if self.session.cleaned or self.session.has_active_tool() or not set(indices):
            return False
        self.session._flush_pending_updates()
        self.straighten_indices = sorted({int(index) for index in indices})
        self.session.active_tool = "straighten"
        widget = load_panel("TaskFormStraighten.ui")
        self.straighten_mode = widget.directionMode
        for index, value in enumerate(("Fit", "Line", "ParallelLine", "TwoPoints")):
            self.straighten_mode.setItemData(index, value)
        self.straighten_type = widget.straightenType
        self.straighten_type.setItemData(0, "ControlPoints")
        self.straighten_type.setItemData(1, "SurfacePoints")
        self.straighten_range = widget.rangeSelection
        self.straighten_reference_widget = widget.referenceWidget
        self.straighten_reference_button = widget.referenceButton
        self.straighten_reference_name = widget.referenceName
        self.straighten_preview_status = widget.previewStatus
        self.straighten_apply_button = widget.applyButton
        self.straighten_cancel_button = widget.cancelButton
        self.straighten_mode.currentIndexChanged.connect(self._straighten_mode_changed)
        self.straighten_type.currentIndexChanged.connect(self._update_straighten_preview)
        self.straighten_range.toggled.connect(self._update_straighten_preview)
        self.straighten_reference_button.toggled.connect(
            self._toggle_straighten_reference_selection
        )
        self.straighten_apply_button.clicked.connect(self.apply_straighten_tool)
        self.straighten_cancel_button.clicked.connect(self.stop_straighten_tool)
        self.straighten_reference_widget.setVisible(False)
        self.session._clear_editor_selection()
        self.session._show_tool_handler(
            App.Qt.translate("Forms_Edit", "Straighten"), widget, "Forms_Straighten"
        )
        self.session._suspend_selection_for_tool()
        self._start_straighten_preview_visibility()
        self._update_straighten_preview()
        Gui.HintManager.show(
            Gui.InputHint(
                App.Qt.translate("Forms_Edit", "%1 cancel straighten"),
                Gui.UserInput.KeyEscape,
            )
        )
        Gui.Command.update()
        return True

    def _straighten_mode_changed(self, _index=None):
        mode = str(self.straighten_mode.currentData())
        requires_reference = mode != "Fit"
        if self.straighten_reference_selecting:
            self.straighten_reference_button.setChecked(False)
        self.straighten_references = []
        self.straighten_reference_name.clear()
        self.straighten_reference_widget.setVisible(requires_reference)
        if mode == "TwoPoints":
            self.straighten_reference_button.setText(
                App.Qt.translate("Forms_Edit", "Select points")
            )
        else:
            self.straighten_reference_button.setText(
                App.Qt.translate("Forms_Edit", "Select line")
            )
        if requires_reference:
            self.straighten_reference_button.setChecked(True)
        else:
            self._update_straighten_preview()

    def _toggle_straighten_reference_selection(self, active):
        if not self.session.straighten_tool_active:
            return
        self.straighten_reference_selecting = bool(active)
        if active:
            if self.session.selection_gate_added:
                Gui.Selection.removeSelectionGate()
                self.session.selection_gate_added = False
                self.session.selection_gate = None
            self.straighten_reference_name.clear()
            mode = str(self.straighten_mode.currentData())
            prompt = (
                App.Qt.translate("Forms_Edit", "Point selection active")
                if mode == "TwoPoints"
                else App.Qt.translate("Forms_Edit", "Line selection active")
            )
            self.straighten_reference_name.setPlaceholderText(prompt)
            self.straighten_preview_status.setText(
                App.Qt.translate("Forms_Edit", "Select two points")
                if mode == "TwoPoints"
                else App.Qt.translate("Forms_Edit", "Select a straight edge or axis")
            )
        else:
            self.session._install_selection_gate()
            if not self.straighten_references:
                self.straighten_reference_name.setPlaceholderText(
                    App.Qt.translate("Forms_Edit", "No reference selected")
                )

    def _straighten_reference_element(self, document, object_name, subelement):
        doc = self.session._selection_document(document)
        source = doc.getObject(str(object_name))
        raw = str(subelement or "")
        body = source if source is not None and source.isDerivedFrom("PartDesign::Body") else None
        if body is not None:
            for feature in getattr(body, "Group", ()):
                if raw.startswith(f"{feature.Name}."):
                    source = feature
                    raw = raw[len(feature.Name) + 1 :]
                    break
        if source is None or source is self.session.obj:
            raise ValueError("Select reference geometry other than the edited Form")
        return source, canonical_subelement_name(raw)

    def _add_straighten_reference(self, document, object_name, subelement):
        try:
            source, element_name = self._straighten_reference_element(
                document, object_name, subelement
            )
            mode = str(self.straighten_mode.currentData())
            if mode == "TwoPoints":
                _point, element_name = self.session._reference_point(source, element_name)
                reference = (source, element_name)
                if reference not in self.straighten_references:
                    self.straighten_references.append(reference)
                self.straighten_references = self.straighten_references[-2:]
            else:
                _edge, element_name = self.session._reference_edge(source, element_name)
                self.straighten_references = [(source, element_name)]
        except (AttributeError, Part.OCCError, RuntimeError, ValueError) as error:
            self.straighten_preview_status.setText(str(error))
            return
        labels = [
            f"{source.Label} [{name}]" if name else source.Label
            for source, name in self.straighten_references
        ]
        self.straighten_reference_name.setText("; ".join(labels))
        complete = mode != "TwoPoints" or len(self.straighten_references) == 2
        if complete:
            self.straighten_reference_button.setChecked(False)
        self.session.suppress_selection_observer = True
        try:
            Gui.Selection.clearSelection()
        finally:
            self.session.suppress_selection_observer = False
        self._update_straighten_preview()

    def _straighten_reference_line(self):
        mode = str(self.straighten_mode.currentData())
        if mode == "Fit":
            return None
        inverse = self.session._global_placement(self.session.obj).inverse()
        if mode in ("Line", "ParallelLine"):
            if len(self.straighten_references) != 1:
                raise ValueError("Select one straight edge or axis")
            source, element_name = self.straighten_references[0]
            edge, _name = self.session._reference_edge(source, element_name)
            placement = self.session._global_placement(source)
            first = inverse.multVec(placement.multVec(edge.Vertexes[0].Point))
            second = inverse.multVec(placement.multVec(edge.Vertexes[-1].Point))
            direction = second.sub(first)
            origin = None if mode == "ParallelLine" else first
        else:
            if len(self.straighten_references) != 2:
                raise ValueError("Select two reference points")
            points = []
            for source, element_name in self.straighten_references:
                point, _name = self.session._reference_point(source, element_name)
                points.append(
                    inverse.multVec(self.session._global_placement(source).multVec(point))
                )
            origin = points[0]
            direction = points[1].sub(points[0])
        return (
            None if origin is None else (origin.x, origin.y, origin.z),
            (direction.x, direction.y, direction.z),
        )

    def _straighten_target_indices(self):
        if not self.straighten_range.isChecked():
            return self.straighten_indices
        if len(self.straighten_indices) != 2:
            raise ValueError("Range selection requires exactly two selected control points")
        cage = ControlCage.from_object(self.session.obj)
        return cage_vertex_range(
            cage.faces, self.straighten_indices[0], self.straighten_indices[1]
        )

    def _start_straighten_preview_visibility(self):
        self.straighten_visibility_before = []
        base = getattr(self.session.obj, "BaseFeature", None)
        for feature in ([base] if base is not None else []) + [self.session.obj]:
            view_object = getattr(feature, "ViewObject", None)
            if view_object is not None:
                self.straighten_visibility_before.append((feature, bool(view_object.Visibility)))
        self.session.obj.ViewObject.Visibility = False
        if base is not None and getattr(base, "ViewObject", None) is not None:
            base.ViewObject.Visibility = True

    def _restore_straighten_preview_visibility(self):
        for feature, visible in self.straighten_visibility_before:
            if feature.Document is not None:
                feature.ViewObject.Visibility = visible
        self.straighten_visibility_before = []

    def _clear_straighten_preview(self):
        self.session._remove_shape_preview(self.straighten_preview_root)
        self.straighten_preview_root = None
        self.straighten_preview_switch = None
        self.straighten_preview_shape = Part.Shape()

    def _update_straighten_preview(self, _value=None):
        if not self.session.straighten_tool_active:
            return
        try:
            indices = self._straighten_target_indices()
            line = self._straighten_reference_line()
            preview_function = (
                preview_straighten_surface_points
                if str(self.straighten_type.currentData()) == "SurfacePoints"
                else preview_straighten_control_points
            )
            shape = preview_function(self.session.obj, indices, line)
            self._clear_straighten_preview()
            self.straighten_preview_root, self.straighten_preview_switch = (
                self.session._make_shape_preview(shape)
            )
            self.straighten_preview_shape = shape
            self.straighten_preview_status.setText(
                App.Qt.translate("Forms_Edit", "Straighten preview")
            )
            self.straighten_apply_button.setEnabled(True)
        except Exception as error:
            self._clear_straighten_preview()
            self.straighten_preview_status.setText(str(error))
            self.straighten_apply_button.setEnabled(False)
        self.session.view.redraw()

    def apply_straighten_tool(self):
        if not self.session.straighten_tool_active:
            return False
        indices = self._straighten_target_indices()
        line = self._straighten_reference_line()
        surface_points = str(self.straighten_type.currentData()) == "SurfacePoints"
        transaction = self.session._begin_action(
            App.Qt.translate("Forms_Edit", "Straighten form controls")
        )
        try:
            straighten_control_points(self.session.obj, indices, line, surface_points)
            self.session.obj.Document.recompute()
            self.session.topology_changed()
        except MODELING_ERRORS as error:
            self.session._finish_action(transaction, commit=False)
            return report_modeling_error(
                App.Qt.translate("Forms_Straighten", "Straighten"),
                error,
                self.straighten_preview_status,
            )
        except Exception:
            self.session._finish_action(transaction, commit=False)
            raise
        self.session._finish_action(transaction)
        self.stop_straighten_tool()
        return True

    def stop_straighten_tool(self):
        if not self.session.straighten_tool_active:
            return
        if self.straighten_reference_selecting and self.straighten_reference_button is not None:
            self.straighten_reference_button.setChecked(False)
        self._clear_straighten_preview()
        self._restore_straighten_preview_visibility()
        self.session.active_tool = None
        self.straighten_indices = []
        self.straighten_references = []
        self.straighten_mode = None
        self.straighten_type = None
        self.straighten_range = None
        self.straighten_reference_button = None
        self.straighten_reference_name = None
        self.straighten_reference_widget = None
        self.straighten_apply_button = None
        self.straighten_cancel_button = None
        self.straighten_preview_status = None
        self.session._resume_selection_after_tool()
        self.session._hide_tool_handler()
        if not self.session.cleaned:
            self.session._show_input_hints()
        self.session.view.redraw()
        Gui.Command.update()
