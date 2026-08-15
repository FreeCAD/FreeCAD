# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2026 AstoCAD     <hello@astocad.com>                     *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

"""Interactive topology-tool workflows used by the Forms editor."""

import math

import FreeCAD as App
import FreeCADGui as Gui
import Part
from PySide import QtCore, QtWidgets
from pivy import coin

from .cage import ControlCage, ControlElementMapper, canonical_subelement_name
from .feedback import MODELING_ERRORS, report_modeling_error
from .operations import (
    flatten_control_points,
    insert_edge_loop,
    insert_edge_on_face,
    insert_point_face_target,
    insert_point_edges,
    local_insert_target,
    preview_flatten_control_points,
    preview_straighten_control_points,
    preview_straighten_surface_points,
    straighten_control_points,
    subdivide_faces,
    unweld_segment,
    weld_boundaries,
)
from .placement import global_placement
from .taskpanels import load_panel
from .topology import cage_edge_loop, cage_vertex_range


class FormEditToolsMixin:
    """Tool panels, previews, and commits layered onto ``FormEditSession``."""

    def _selected_control_points(self):
        targets = self._selected_control_targets()
        selected = set()
        for _kind, indices, _anchor in targets:
            selected.update(indices)
        return sorted(selected)

    def _selected_control_edges(self):
        return {
            tuple(sorted(indices))
            for kind, indices, _anchor in self._selected_control_targets(respect_symmetry=False)
            if kind == "Edge" and len(indices) == 2
        }

    def start_straighten_tool(self, indices):
        """Open Fusion-style Straighten for the selected cage controls."""
        if self.cleaned or self.has_active_tool() or not set(indices):
            return False
        self._flush_pending_updates()
        self.straighten_indices = sorted({int(index) for index in indices})
        self.active_tool = "straighten"
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
        self._clear_editor_selection()
        self._show_tool_handler(
            App.Qt.translate("Forms_Edit", "Straighten"), widget, "Forms_Straighten"
        )
        self._suspend_selection_for_tool()
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
        if not self.straighten_tool_active:
            return
        self.straighten_reference_selecting = bool(active)
        if active:
            if self.selection_gate_added:
                Gui.Selection.removeSelectionGate()
                self.selection_gate_added = False
                self.selection_gate = None
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
            self._install_selection_gate()
            if not self.straighten_references:
                self.straighten_reference_name.setPlaceholderText(
                    App.Qt.translate("Forms_Edit", "No reference selected")
                )

    def _straighten_reference_element(self, document, object_name, subelement):
        doc = self._selection_document(document)
        source = doc.getObject(str(object_name))
        raw = str(subelement or "")
        body = source if source is not None and source.isDerivedFrom("PartDesign::Body") else None
        if body is not None:
            for feature in getattr(body, "Group", ()):
                if raw.startswith(f"{feature.Name}."):
                    source = feature
                    raw = raw[len(feature.Name) + 1 :]
                    break
        if source is None or source is self.obj:
            raise ValueError("Select reference geometry other than the edited Form")
        return source, canonical_subelement_name(raw)

    def _reference_edge(self, source, element_name):
        shape = getattr(source, "Shape", None)
        if shape is None or shape.isNull():
            raise ValueError("Select a straight edge or axis")
        if element_name.startswith("Edge"):
            edge = shape.getElement(element_name)
        elif shape.ShapeType == "Edge":
            edge = shape
            element_name = ""
        elif len(shape.Edges) == 1:
            edge = shape.Edges[0]
            element_name = "Edge1"
        else:
            raise ValueError("Select one straight edge or axis")
        if "Line" not in str(getattr(edge.Curve, "TypeId", "")):
            raise ValueError("Straighten reference must be a straight line")
        return edge, element_name

    def _reference_point(self, source, element_name):
        shape = getattr(source, "Shape", None)
        if element_name.startswith("Vertex") and shape is not None:
            return shape.getElement(element_name).Point, element_name
        if shape is not None and not shape.isNull():
            if shape.ShapeType == "Vertex":
                return shape.Point, ""
            if len(shape.Vertexes) == 1:
                return shape.Vertexes[0].Point, "Vertex1"
        if "Point" in str(getattr(source, "TypeId", "")):
            return App.Vector(), ""
        raise ValueError("Select a vertex or datum point")

    def _add_straighten_reference(self, document, object_name, subelement):
        try:
            source, element_name = self._straighten_reference_element(
                document, object_name, subelement
            )
            mode = str(self.straighten_mode.currentData())
            if mode == "TwoPoints":
                _point, element_name = self._reference_point(source, element_name)
                reference = (source, element_name)
                if reference not in self.straighten_references:
                    self.straighten_references.append(reference)
                self.straighten_references = self.straighten_references[-2:]
            else:
                _edge, element_name = self._reference_edge(source, element_name)
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
        self.suppress_selection_observer = True
        try:
            Gui.Selection.clearSelection()
        finally:
            self.suppress_selection_observer = False
        self._update_straighten_preview()

    def _straighten_reference_line(self):
        mode = str(self.straighten_mode.currentData())
        if mode == "Fit":
            return None
        inverse = self._global_placement(self.obj).inverse()
        if mode in ("Line", "ParallelLine"):
            if len(self.straighten_references) != 1:
                raise ValueError("Select one straight edge or axis")
            source, element_name = self.straighten_references[0]
            edge, _name = self._reference_edge(source, element_name)
            placement = self._global_placement(source)
            first = inverse.multVec(placement.multVec(edge.Vertexes[0].Point))
            second = inverse.multVec(placement.multVec(edge.Vertexes[-1].Point))
            direction = second.sub(first)
            origin = None if mode == "ParallelLine" else first
        else:
            if len(self.straighten_references) != 2:
                raise ValueError("Select two reference points")
            points = []
            for source, element_name in self.straighten_references:
                point, _name = self._reference_point(source, element_name)
                points.append(
                    inverse.multVec(self._global_placement(source).multVec(point))
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
        cage = ControlCage.from_object(self.obj)
        return cage_vertex_range(
            cage.faces, self.straighten_indices[0], self.straighten_indices[1]
        )

    def _start_straighten_preview_visibility(self):
        self.straighten_visibility_before = []
        base = getattr(self.obj, "BaseFeature", None)
        for feature in ([base] if base is not None else []) + [self.obj]:
            view_object = getattr(feature, "ViewObject", None)
            if view_object is not None:
                self.straighten_visibility_before.append((feature, bool(view_object.Visibility)))
        self.obj.ViewObject.Visibility = False
        if base is not None and getattr(base, "ViewObject", None) is not None:
            base.ViewObject.Visibility = True

    def _restore_straighten_preview_visibility(self):
        for feature, visible in self.straighten_visibility_before:
            if feature.Document is not None:
                feature.ViewObject.Visibility = visible
        self.straighten_visibility_before = []

    def _clear_straighten_preview(self):
        self._remove_shape_preview(self.straighten_preview_root)
        self.straighten_preview_root = None
        self.straighten_preview_switch = None
        self.straighten_preview_shape = Part.Shape()

    def _update_straighten_preview(self, _value=None):
        if not self.straighten_tool_active:
            return
        try:
            indices = self._straighten_target_indices()
            line = self._straighten_reference_line()
            preview_function = (
                preview_straighten_surface_points
                if str(self.straighten_type.currentData()) == "SurfacePoints"
                else preview_straighten_control_points
            )
            shape = preview_function(self.obj, indices, line)
            self._clear_straighten_preview()
            self.straighten_preview_root, self.straighten_preview_switch = (
                self._make_shape_preview(shape)
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
        self.view.redraw()

    def apply_straighten_tool(self):
        if not self.straighten_tool_active:
            return False
        indices = self._straighten_target_indices()
        line = self._straighten_reference_line()
        surface_points = str(self.straighten_type.currentData()) == "SurfacePoints"
        transaction = self._begin_action(
            App.Qt.translate("Forms_Edit", "Straighten form controls")
        )
        try:
            straighten_control_points(self.obj, indices, line, surface_points)
            self.obj.Document.recompute()
            self.topology_changed()
        except MODELING_ERRORS as error:
            self._finish_action(transaction, commit=False)
            return report_modeling_error(
                App.Qt.translate("Forms_Straighten", "Straighten"),
                error,
                self.straighten_preview_status,
            )
        except Exception:
            self._finish_action(transaction, commit=False)
            raise
        self._finish_action(transaction)
        self.stop_straighten_tool()
        return True

    def stop_straighten_tool(self):
        if not self.straighten_tool_active:
            return
        if self.straighten_reference_selecting and self.straighten_reference_button is not None:
            self.straighten_reference_button.setChecked(False)
        self._clear_straighten_preview()
        self._restore_straighten_preview_visibility()
        self.active_tool = None
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
        self._resume_selection_after_tool()
        self._hide_tool_handler()
        if not self.cleaned:
            self._show_input_hints()
        self.view.redraw()
        Gui.Command.update()

    def start_flatten_tool(self, indices):
        """Open Flatten for the selected controls without changing the Form."""
        if self.cleaned or self.has_active_tool() or len(set(indices)) < 3:
            return False
        self._flush_pending_updates()
        self.flatten_indices = sorted({int(index) for index in indices})
        self.active_tool = "flatten"
        widget = load_panel("TaskFormFlatten.ui")
        self.flatten_mode = widget.flattenMode
        for index, value in enumerate(("BestFit", "XY", "XZ", "YZ", "Reference")):
            self.flatten_mode.setItemData(index, value)
        self.flatten_reference_widget = widget.referenceWidget
        self.flatten_reference_button = widget.referenceButton
        self.flatten_reference_name = widget.referenceName
        self.flatten_preview_status = widget.previewStatus
        self.flatten_apply_button = widget.applyButton
        self.flatten_cancel_button = widget.cancelButton
        self.flatten_mode.currentIndexChanged.connect(self._flatten_mode_changed)
        self.flatten_reference_button.toggled.connect(self._toggle_flatten_reference_selection)
        self.flatten_apply_button.clicked.connect(self.apply_flatten_tool)
        self.flatten_cancel_button.clicked.connect(self.stop_flatten_tool)
        self.flatten_reference_widget.setVisible(False)
        self._clear_editor_selection()
        self._show_tool_handler(
            App.Qt.translate("Forms_Edit", "Flatten"), widget, "Forms_Flatten"
        )
        self._suspend_selection_for_tool()
        self._start_flatten_preview_visibility()
        self._update_flatten_preview()
        Gui.HintManager.show(
            Gui.InputHint(
                App.Qt.translate("Forms_Edit", "%1 cancel flatten"),
                Gui.UserInput.KeyEscape,
            )
        )
        Gui.Command.update()
        return True

    def _flatten_mode_changed(self, _index=None):
        reference = str(self.flatten_mode.currentData()) == "Reference"
        self.flatten_reference_widget.setVisible(reference)
        if not reference and self.flatten_reference_selecting:
            self.flatten_reference_button.setChecked(False)
        if reference and self.flatten_reference is None:
            self.flatten_reference_button.setChecked(True)
        else:
            self._update_flatten_preview()

    def _toggle_flatten_reference_selection(self, active):
        if not self.flatten_tool_active:
            return
        self.flatten_reference_selecting = bool(active)
        if active:
            if self.selection_gate_added:
                Gui.Selection.removeSelectionGate()
                self.selection_gate_added = False
                self.selection_gate = None
            self.flatten_reference_name.clear()
            self.flatten_reference_name.setPlaceholderText(
                App.Qt.translate("Forms_Edit", "Face selection active")
            )
            self.flatten_preview_status.setText(
                App.Qt.translate("Forms_Edit", "Select a planar face or plane")
            )
        else:
            self._install_selection_gate()
            if self.flatten_reference is None:
                self.flatten_reference_name.setPlaceholderText(
                    App.Qt.translate("Forms_Edit", "No face selected")
                )

    @staticmethod
    def _selection_document(document):
        return document if hasattr(document, "getObject") else App.getDocument(str(document))

    def _flatten_reference_from_event(self, document, object_name, subelement):
        doc = self._selection_document(document)
        source = doc.getObject(str(object_name))
        raw = str(subelement or "")
        body = source if source is not None and source.isDerivedFrom("PartDesign::Body") else None
        if body is not None:
            for feature in getattr(body, "Group", ()):
                if raw.startswith(f"{feature.Name}."):
                    source = feature
                    raw = raw[len(feature.Name) + 1 :]
                    break
        if source is None or source is self.obj:
            raise ValueError("Select a face or plane other than the edited Form")
        canonical = canonical_subelement_name(raw)
        shape = getattr(source, "Shape", None)
        face = None
        if canonical.startswith("Face") and shape is not None:
            face = shape.getElement(canonical)
        elif shape is not None and not shape.isNull() and len(shape.Faces) == 1:
            face = shape.Faces[0]
            canonical = "Face1"
        if face is not None:
            if not face.Surface.isPlanar():
                raise ValueError("Flatten reference must be planar")
            return source, canonical
        if "Plane" in str(getattr(source, "TypeId", "")):
            return source, ""
        raise ValueError("Select a planar face or plane")

    def _set_flatten_reference_from_selection(self, document, object_name, subelement):
        try:
            source, face_name = self._flatten_reference_from_event(
                document, object_name, subelement
            )
        except (AttributeError, Part.OCCError, RuntimeError, ValueError) as error:
            self.flatten_preview_status.setText(str(error))
            return
        self.flatten_reference = (source, face_name)
        label = source.Label
        self.flatten_reference_name.setText(
            f"{label} [{face_name}]" if face_name else label
        )
        self.flatten_reference_button.setChecked(False)
        self.suppress_selection_observer = True
        try:
            Gui.Selection.clearSelection()
        finally:
            self.suppress_selection_observer = False
        self._update_flatten_preview()

    @staticmethod
    def _global_placement(obj):
        return global_placement(obj)

    def _flatten_plane(self):
        mode = str(self.flatten_mode.currentData())
        if mode == "BestFit":
            return None
        if mode == "Reference":
            if self.flatten_reference is None:
                raise ValueError("Select a reference face or plane")
            source, face_name = self.flatten_reference
            source_placement = self._global_placement(source)
            if face_name:
                face = source.Shape.getElement(face_name)
                origin = source_placement.multVec(face.CenterOfMass)
                u_value, v_value = face.Surface.parameter(face.CenterOfMass)
                normal = source_placement.Rotation.multVec(face.normalAt(u_value, v_value))
            else:
                origin = source_placement.Base
                normal = source_placement.Rotation.multVec(App.Vector(0, 0, 1))
        else:
            normal = {
                "XY": App.Vector(0, 0, 1),
                "XZ": App.Vector(0, 1, 0),
                "YZ": App.Vector(1, 0, 0),
            }[mode]
            inverse = self._global_placement(self.obj).inverse()
            local_normal = inverse.Rotation.multVec(normal)
            return (
                None,
                (local_normal.x, local_normal.y, local_normal.z),
            )
        inverse = self._global_placement(self.obj).inverse()
        local_origin = inverse.multVec(origin)
        local_normal = inverse.Rotation.multVec(normal)
        return (
            (local_origin.x, local_origin.y, local_origin.z),
            (local_normal.x, local_normal.y, local_normal.z),
        )

    def _start_flatten_preview_visibility(self):
        self.flatten_visibility_before = []
        base = getattr(self.obj, "BaseFeature", None)
        for feature in ([base] if base is not None else []) + [self.obj]:
            view_object = getattr(feature, "ViewObject", None)
            if view_object is not None:
                self.flatten_visibility_before.append((feature, bool(view_object.Visibility)))
        self.obj.ViewObject.Visibility = False
        if base is not None and getattr(base, "ViewObject", None) is not None:
            base.ViewObject.Visibility = True

    def _restore_flatten_preview_visibility(self):
        for feature, visible in self.flatten_visibility_before:
            if feature.Document is not None:
                feature.ViewObject.Visibility = visible
        self.flatten_visibility_before = []

    def _set_flatten_preview_shape(self, shape):
        self._clear_flatten_preview()
        self.flatten_preview_root, self.flatten_preview_switch = self._make_shape_preview(shape)
        self.flatten_preview_shape = shape

    def _clear_flatten_preview(self):
        self._remove_shape_preview(self.flatten_preview_root)
        self.flatten_preview_root = None
        self.flatten_preview_switch = None
        self.flatten_preview_shape = Part.Shape()

    def _update_flatten_preview(self, _index=None):
        if not self.flatten_tool_active:
            return
        try:
            shape = preview_flatten_control_points(
                self.obj, self.flatten_indices, self._flatten_plane()
            )
            self._set_flatten_preview_shape(shape)
            self.flatten_preview_status.setText(
                App.Qt.translate("Forms_Edit", "Flatten preview")
            )
            self.flatten_apply_button.setEnabled(True)
        except Exception as error:
            self._clear_flatten_preview()
            self.flatten_preview_status.setText(str(error))
            self.flatten_apply_button.setEnabled(False)
        self.view.redraw()

    def apply_flatten_tool(self):
        if not self.flatten_tool_active:
            return False
        plane = self._flatten_plane()
        transaction = self._begin_action(
            App.Qt.translate("Forms_Edit", "Flatten form controls")
        )
        try:
            flatten_control_points(self.obj, self.flatten_indices, plane)
            self.obj.Document.recompute()
            self.topology_changed()
        except MODELING_ERRORS as error:
            self._finish_action(transaction, commit=False)
            return report_modeling_error(
                App.Qt.translate("Forms_Flatten", "Flatten"),
                error,
                self.flatten_preview_status,
            )
        except Exception:
            self._finish_action(transaction, commit=False)
            raise
        self._finish_action(transaction)
        self.stop_flatten_tool()
        return True

    def stop_flatten_tool(self):
        if not self.flatten_tool_active:
            return
        if self.flatten_reference_selecting and self.flatten_reference_button is not None:
            self.flatten_reference_button.setChecked(False)
        self._clear_flatten_preview()
        self._restore_flatten_preview_visibility()
        self.active_tool = None
        self.flatten_indices = []
        self.flatten_reference = None
        self.flatten_mode = None
        self.flatten_reference_button = None
        self.flatten_reference_name = None
        self.flatten_reference_widget = None
        self.flatten_apply_button = None
        self.flatten_cancel_button = None
        self.flatten_preview_status = None
        self._resume_selection_after_tool()
        self._hide_tool_handler()
        if not self.cleaned:
            self._show_input_hints()
        self.view.redraw()
        Gui.Command.update()

    def start_weld_tool(self):
        """Open the two-Form boundary Weld task handler."""
        if self.cleaned or self.has_active_tool() or str(self.obj.FormType) == "Forms::Surface":
            return False
        self._flush_pending_updates()
        self.active_tool = "weld"
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
        self._clear_editor_selection()
        self._show_tool_handler(
            App.Qt.translate("Forms_Weld", "Weld"), widget, "Forms_Weld"
        )
        self._suspend_selection_for_tool(disable_selection=False)
        if self.selection_gate_added:
            Gui.Selection.removeSelectionGate()
            self.selection_gate_added = False
            self.selection_gate = None
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
        if not self.weld_tool_active:
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
            self._control_element_mapper()
            if source is self.obj
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
        doc = self._selection_document(document)
        source = doc.getObject(str(object_name)) if doc is not None else None
        if self.weld_selecting_other:
            if (
                source is None
                or source is self.obj
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
            self.suppress_selection_observer = True
            try:
                Gui.Selection.clearSelection()
            finally:
                self.suppress_selection_observer = False
            self.weld_apply_button.setEnabled(False)
            return
        if source not in (self.obj, self.weld_other):
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
        if source is self.obj:
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
            not self.weld_tool_active
            or self.weld_other is None
            or self.weld_first_edge is None
            or self.weld_second_edge is None
        ):
            return False
        self._flush_pending_updates()
        transaction = self._begin_action(App.Qt.translate("Forms_Weld", "Weld Forms"))
        try:
            self.suppress_selection_observer = True
            try:
                Gui.Selection.clearSelection()
            finally:
                self.suppress_selection_observer = False
            weld_boundaries(
                self.obj,
                self.weld_first_edge,
                self.weld_other,
                self.weld_second_edge,
            )
            self.obj.Document.recompute()
            self.cached_control_mapper = None
            self.cached_control_mapper_signature = None
            self.topology_changed()
        except MODELING_ERRORS as error:
            self._finish_action(transaction, commit=False)
            return report_modeling_error(
                App.Qt.translate("Forms_Weld", "Weld Forms"), error, self.weld_status
            )
        except Exception:
            self._finish_action(transaction, commit=False)
            raise
        self._finish_action(transaction)
        self.stop_weld_tool()
        return True

    def stop_weld_tool(self):
        if not self.weld_tool_active:
            return
        self.active_tool = None
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
        self._resume_selection_after_tool()
        self._install_selection_gate()
        self._hide_tool_handler()
        if not self.cleaned:
            self._show_input_hints()
        self.view.redraw()
        Gui.Command.update()

    def start_match_tool(self, obj, edges, support):
        """Open the Match task handler after its opening and support are selected."""
        if self.cleaned or self.has_active_tool() or obj != self.obj:
            return False
        self._flush_pending_updates()
        self.match_inputs = (obj, set(edges), support)
        self.active_tool = "match"

        widget = load_panel("TaskFormMatch.ui")
        self.match_mode = widget.continuityMode
        for index, value in enumerate(("AdjacentFaces", "SelectedFace", "Connected")):
            self.match_mode.setItemData(index, value)
        current = (
            "Connected"
            if str(self.obj.MatchContinuity) == "Connected"
            else str(getattr(self.obj, "MatchTangentMode", "AdjacentFaces"))
        )
        self.match_mode.setCurrentIndex(max(self.match_mode.findData(current), 0))
        self.match_mode.currentIndexChanged.connect(self._update_match_preview)

        self.match_preview_status = widget.previewStatus
        self.match_apply_button = widget.applyButton
        self.match_cancel_button = widget.cancelButton
        self.match_apply_button.clicked.connect(self.apply_match_tool)
        self.match_cancel_button.clicked.connect(self.stop_match_tool)
        self._clear_editor_selection()
        self._show_tool_handler(
            App.Qt.translate("Forms_Edit", "Match"),
            widget,
            "Forms_Match",
        )
        self._suspend_selection_for_tool()
        self._start_match_preview_visibility()
        self._update_match_preview()
        Gui.HintManager.show(
            Gui.InputHint(
                App.Qt.translate("Forms_Edit", "%1 cancel match"),
                Gui.UserInput.KeyEscape,
            )
        )
        Gui.Command.update()
        return True

    def _start_match_preview_visibility(self):
        self.match_visibility_before = []
        base = getattr(self.obj, "BaseFeature", None)
        features = ([base] if base is not None else []) + [self.obj]
        for feature in features:
            view_object = getattr(feature, "ViewObject", None)
            if view_object is not None:
                self.match_visibility_before.append((feature, bool(view_object.Visibility)))
        self.obj.ViewObject.Visibility = False
        if base is not None and getattr(base, "ViewObject", None) is not None:
            base.ViewObject.Visibility = True

    def _restore_match_preview_visibility(self):
        for feature, visible in self.match_visibility_before:
            if feature.Document is not None:
                feature.ViewObject.Visibility = visible
        self.match_visibility_before = []

    def _set_match_preview_shape(self, shape):
        self._clear_match_preview()
        self.match_preview_root, self.match_preview_switch = self._make_shape_preview(shape)
        self.match_preview_shape = shape

    def _make_shape_preview(self, shape):
        """Add a standard translucent Forms operation preview to the view."""
        root = coin.SoSeparator()
        switch = coin.SoSwitch()
        preview = coin.SoSeparator()
        material = coin.SoMaterial()
        material.diffuseColor = (0.0, 1.0, 0.6)
        material.emissiveColor = (0.0, 0.18, 0.1)
        material.transparency = 0.65
        material.setOverride(True)
        draw_style = coin.SoDrawStyle()
        draw_style.lineWidth = 2.0
        draw_style.setOverride(True)
        source = coin.SoInput()
        source.setBuffer(shape.writeInventor())
        geometry = coin.SoDB.readAll(source)
        preview.addChild(material)
        preview.addChild(draw_style)
        preview.addChild(geometry)
        switch.addChild(preview)
        switch.whichChild = coin.SO_SWITCH_ALL
        root.addChild(switch)
        self.view.getSceneGraph().addChild(root)
        return root, switch

    def _remove_shape_preview(self, root):
        if root is not None:
            try:
                self.view.getSceneGraph().removeChild(root)
            except (AttributeError, RuntimeError):
                pass

    def _clear_match_preview(self):
        self._remove_shape_preview(self.match_preview_root)
        self.match_preview_root = None
        self.match_preview_switch = None
        self.match_preview_shape = Part.Shape()

    def _update_match_preview(self, _index=None):
        if not self.match_tool_active or self.match_inputs is None:
            return
        from .additive import preview_match_shape

        obj, edges, support = self.match_inputs
        mode = str(self.match_mode.currentData())
        continuity = "Connected" if mode == "Connected" else "Tangent"
        tangent_mode = (
            str(getattr(obj, "MatchTangentMode", "AdjacentFaces")) if mode == "Connected" else mode
        )
        try:
            shape = preview_match_shape(obj, edges, support, continuity, tangent_mode)
            self._set_match_preview_shape(shape)
            self.match_preview_status.setText(
                App.Qt.translate("Forms_Edit", "Match preview")
            )
            self.match_apply_button.setEnabled(True)
        except Exception as error:
            self._clear_match_preview()
            self.match_preview_status.setText(str(error))
            self.match_apply_button.setEnabled(False)
        self.view.redraw()

    def apply_match_tool(self):
        """Apply the selected Match continuity mode as one undoable action."""
        if not self.match_tool_active or self.match_inputs is None:
            return False
        from .additive import match_boundary

        obj, edges, support = self.match_inputs
        mode = str(self.match_mode.currentData())
        continuity = "Connected" if mode == "Connected" else "Tangent"
        tangent_mode = (
            str(getattr(obj, "MatchTangentMode", "AdjacentFaces")) if mode == "Connected" else mode
        )
        transaction = self._begin_action(App.Qt.translate("Forms_Edit", "Match form opening"))
        try:
            match_boundary(obj, edges, support, continuity, tangent_mode)
            obj.Document.recompute()
            self.topology_changed()
        except MODELING_ERRORS as error:
            self._finish_action(transaction, commit=False)
            return report_modeling_error(
                App.Qt.translate("Forms_Match", "Match"),
                error,
                self.match_preview_status,
            )
        except Exception:
            self._finish_action(transaction, commit=False)
            raise
        self._finish_action(transaction)
        self.stop_match_tool()
        return True

    def stop_match_tool(self):
        """Dismiss Match without ending the surrounding Form edit."""
        if not self.match_tool_active:
            return
        self._clear_match_preview()
        self._restore_match_preview_visibility()
        self.active_tool = None
        self.match_inputs = None
        self.match_mode = None
        self.match_apply_button = None
        self.match_cancel_button = None
        self.match_preview_status = None
        self._resume_selection_after_tool()
        self._hide_tool_handler()
        if not self.cleaned:
            self._show_input_hints()
        self.view.redraw()
        Gui.Command.update()

    def start_thicken_tool(self):
        """Start a debounced, cage-native Thicken preview."""
        if self.cleaned or self.has_active_tool():
            return False
        cage = ControlCage.from_object(self.obj)
        if cage.is_closed:
            raise ValueError("Thicken currently requires an open Form surface")
        if getattr(self.obj, "LocalEdgeInserts", ()) or str(
            getattr(self.obj, "TMeshData", "") or ""
        ):
            raise ValueError("Thicken does not yet support local edge inserts")
        self._flush_pending_updates()
        self._clear_editor_selection()
        self.thicken_original_cage = cage
        self.thicken_original_mode = str(self.obj.CageMode)
        self.thicken_transaction_open = self._begin_action(
            App.Qt.translate("Forms_Edit", "Thicken form")
        )
        self.active_tool = "thicken"
        widget = self._create_thicken_tool_widget()
        self._show_tool_handler(
            App.Qt.translate("Forms_Edit", "Thicken"),
            widget,
            "Forms_Thicken",
        )
        self._suspend_selection_for_tool()
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
        bounds = self.obj.Shape.BoundBox
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
        if self.thicken_tool_active and not self.cleaned:
            self.thicken_update_timer.start()

    def _apply_thicken_preview(self):
        if self.cleaned or not self.thicken_tool_active or self.thicken_original_cage is None:
            return False
        distance = float(self.thicken_distance.value())
        self.thicken_apply_button.setEnabled(abs(distance) > 1.0e-9)
        if abs(distance) <= 1.0e-9:
            return False
        try:
            cage = self.thicken_original_cage.thickened(distance, sharp=True)
            cage.write(self.obj)
            self.obj.CageMode = "Editable"
            self.obj.touch()
            self.obj.Document.recompute()
            self._set_parametric_state(False)
            self._sync_dimension_properties()
        except MODELING_ERRORS as error:
            self.thicken_apply_button.setEnabled(False)
            return report_modeling_error(
                App.Qt.translate("Forms_Thicken", "Thicken"), error
            )
        return True

    def stop_thicken_tool(self, apply=False):
        """Apply or restore the temporary Thicken result."""
        if not self.thicken_tool_active:
            return
        self.thicken_update_timer.stop()
        try:
            if apply:
                self._apply_thicken_preview()
            elif self.thicken_original_cage is not None:
                self.thicken_original_cage.write(self.obj)
                self.obj.CageMode = self.thicken_original_mode
                self.obj.touch()
                self.obj.Document.recompute()
        except Exception:
            self._finish_action(self.thicken_transaction_open, commit=False)
            self.thicken_transaction_open = False
            raise
        self._finish_action(self.thicken_transaction_open, commit=apply)
        self.thicken_transaction_open = False
        self.active_tool = None
        self.thicken_original_cage = None
        self.thicken_original_mode = None
        self._hide_tool_handler()
        self.thicken_distance = None
        self.thicken_apply_button = None
        self.thicken_cancel_button = None
        self._resume_selection_after_tool()
        if not self.cleaned:
            self._set_parametric_state(self.obj.CageMode == "Parametric")
            self._configure_symmetry(apply=bool(self.obj.Symmetric))
            self._sync_dimension_properties()
            self._update_dimension_gizmos()
            self._show_input_hints()
        self.view.redraw()

    def start_set_pivot_tool(self):
        """Pick a temporary transform origin without changing the selection."""
        if self.cleaned or self.has_active_tool() or not self.selected:
            return False
        self._flush_pending_updates()
        self.active_tool = "set_pivot"
        self.pivot_snap_point = None
        self.pivot_pick_pending = False
        self.pivot_selection_snapshot = [
            (selection.Object, tuple(selection.SubElementNames))
            for selection in Gui.Selection.getSelectionEx()
        ]
        self.pivot_previous_selection_filter = self.selection_filter.currentIndex()
        all_index = self.selection_filter.findData("All")
        if all_index >= 0:
            blocker = QtCore.QSignalBlocker(self.selection_filter)
            self.selection_filter.setCurrentIndex(all_index)
            del blocker
            self._install_selection_gate()
        self.set_pivot_button.setEnabled(False)

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
        self._show_tool_handler(
            App.Qt.translate("Forms_SetPivot", "Set Pivot"),
            widget,
            "Forms_SetPivot",
        )
        # Keep native picking enabled as a fallback. Some navigation styles
        # consume the mouse event before a Pivy callback can handle it; the
        # selection observer then supplies the picked 3D point and restores
        # the original selection immediately afterward.
        self._suspend_selection_for_tool(hide_dragger=False, disable_selection=False)
        try:
            if not hasattr(Gui, "Snapper"):
                from draftguitools.gui_snapper import Snapper

                Gui.Snapper = Snapper()
            if hasattr(self.view, "activateToolHandler"):
                self.view.activateToolHandler("Forms_Pointer_SetPivot")
            self.pivot_tool_callback = self.view.addEventCallback(
                "SoEvent", self._pivot_tool_event
            )
            self.pivot_tool_mouse_callback = self.view.addEventCallbackPivy(
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
            not self.pivot_tool_active
            and self.pivot_tool_callback is None
            and self.pivot_tool_mouse_callback is None
        ):
            return
        self.active_tool = None
        if self.pivot_tool_callback is not None:
            try:
                self.view.removeEventCallback("SoEvent", self.pivot_tool_callback)
            except (AttributeError, RuntimeError):
                pass
            self.pivot_tool_callback = None
        if self.pivot_tool_mouse_callback is not None:
            try:
                self.view.removeEventCallbackPivy(
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
        if hasattr(self.view, "deactivateToolHandler"):
            try:
                self.view.deactivateToolHandler()
            except (AttributeError, RuntimeError):
                pass
        self.pivot_snap_point = None
        self.pivot_pick_pending = False
        self._resume_selection_after_tool()
        if self.pivot_previous_selection_filter is not None:
            blocker = QtCore.QSignalBlocker(self.selection_filter)
            self.selection_filter.setCurrentIndex(self.pivot_previous_selection_filter)
            del blocker
            self.pivot_previous_selection_filter = None
            self._install_selection_gate()
        self._restore_pivot_selection()
        self.set_pivot_button.setEnabled(bool(self.selected))
        self._hide_tool_handler()
        if not self.cleaned:
            self._show_input_hints()
        self.view.redraw()
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
        return self._global_placement(self.obj).inverse().multVec(App.Vector(point))

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
        point = self._global_placement(self.obj).inverse().multVec(point)
        self.pivot_pick_pending = True
        QtCore.QTimer.singleShot(0, lambda picked=point: self._complete_pivot_pick(picked))

    def _restore_pivot_selection(self):
        snapshot = self.pivot_selection_snapshot
        if snapshot is None:
            return
        self.pivot_selection_snapshot = None
        self.suppress_selection_observer = True
        try:
            Gui.Selection.clearSelection()
            for obj, subelements in snapshot:
                if subelements:
                    Gui.Selection.addSelection(obj, list(subelements))
                else:
                    Gui.Selection.addSelection(obj)
        finally:
            self.suppress_selection_observer = False

    def _complete_pivot_pick(self, point):
        if self.cleaned or not self.pivot_tool_active:
            return
        self.base_points = self._all_control_points()
        self.base_center = App.Vector(point)
        self.syncing = True
        self.dragger.translation.setValue(point.x, point.y, point.z)
        self.dragger.planarScaleFactor.setValue(1.0, 1.0, 1.0)
        self.syncing = False
        self._update_dragger_scale()
        self.stop_set_pivot_tool()

    def _pivot_tool_event(self, info):
        if self.cleaned or not self.pivot_tool_active:
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
        if self.cleaned or not self.pivot_tool_active:
            return
        event = event_callback.getEvent()
        if event.getState() != coin.SoButtonEvent.DOWN:
            return
        button = event.getButton()
        if button == coin.SoMouseButtonEvent.BUTTON2:
            event_callback.setHandled()
            QtCore.QTimer.singleShot(0, self.stop_set_pivot_tool)
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

    def start_insert_edge_tool(self):
        """Start the repeatable hover-preview Insert Edge handler."""
        widget = load_panel("TaskFormInsertEdge.ui")
        is_surface = str(self.obj.FormType) == "Forms::Surface"
        widget.surfaceMessage.setVisible(is_surface)
        widget.wholeLoop.setVisible(not is_surface)
        if not is_surface:
            self.insert_whole_loop = widget.wholeLoop
            self.insert_whole_loop.toggled.connect(self._insert_loop_changed)
        return self._start_surface_tool(
            "insert_edge",
            App.Qt.translate("Forms_Edit", "Insert Edge"),
            widget,
            "Forms_InsertEdge",
            "Forms_Pointer_InsertEdge",
        )

    def start_subdivide_tool(self):
        """Start the repeatable hover-preview Subdivide handler."""
        widget = load_panel("TaskFormSubdivide.ui")
        self.subdivide_u = widget.uSubdivisions
        self.subdivide_v = widget.vSubdivisions
        for axis, spinbox in (("u", self.subdivide_u), ("v", self.subdivide_v)):
            spinbox.setValue(self.subdivide_last_counts[axis])
            spinbox.valueChanged.connect(
                lambda value, selected_axis=axis: self._subdivision_count_changed(
                    selected_axis, value
                )
            )
        return self._start_surface_tool(
            "subdivide",
            App.Qt.translate("Forms_Edit", "Subdivide"),
            widget,
            "Forms_Subdivide",
            "Forms_Pointer_Subdivide",
        )

    def start_insert_point_tool(self):
        """Start the free-position, polyline-style Insert Point handler."""
        if str(self.obj.FormType) == "Forms::Surface":
            return False
        widget = QtWidgets.QWidget()
        layout = QtWidgets.QVBoxLayout(widget)
        layout.setContentsMargins(0, 0, 0, 0)
        message = QtWidgets.QLabel(
            App.Qt.translate(
                "Forms_InsertPoint",
                "Click points on control edges. Consecutive points create an edge across "
                "their common face. Right-click commits the chain; right-click again exits.",
            ),
            widget,
        )
        message.setWordWrap(True)
        layout.addWidget(message)
        return self._start_surface_tool(
            "insert_point",
            App.Qt.translate("Forms_InsertPoint", "Insert Point"),
            widget,
            "Forms_InsertPoint",
            "Forms_Pointer_InsertPoint",
        )

    def start_unweld_tool(self):
        """Start the one-click, whole-segment Unweld handler."""
        if str(self.obj.FormType) == "Forms::Surface":
            return False
        widget = QtWidgets.QWidget()
        layout = QtWidgets.QVBoxLayout(widget)
        layout.setContentsMargins(0, 0, 0, 0)
        message = QtWidgets.QLabel(
            App.Qt.translate(
                "Forms_Unweld",
                "Hover a separating segment on a closed Form, then click to split the "
                "Form into two surfaces along the highlighted segment.",
            ),
            widget,
        )
        message.setWordWrap(True)
        layout.addWidget(message)
        separate_forms = QtWidgets.QCheckBox(
            App.Qt.translate("Forms_Unweld", "Separate Forms"), widget
        )
        separate_forms.setChecked(True)
        separate_forms.setToolTip(
            App.Qt.translate(
                "Forms_Unweld",
                "Create a second document object instead of keeping both surfaces in one Form",
            )
        )
        layout.addWidget(separate_forms)
        started = self._start_surface_tool(
            "unweld",
            App.Qt.translate("Forms_Unweld", "Unweld"),
            widget,
            "Forms_Unweld",
            "Forms_Pointer_InsertEdge",
        )
        self.unweld_separate_forms = separate_forms if started else None
        return started

    def _start_surface_tool(self, tool, title, widget, command_name, cursor_icon):
        if self.cleaned or self.has_active_tool():
            widget.deleteLater()
            return False
        self._flush_pending_updates()
        self._clear_editor_selection()
        self.active_tool = tool
        self.surface_tool_cursor_icon = cursor_icon
        self.insert_orientation = 0
        self.insert_point_chain = []
        self.insert_point_hover = None
        self.unweld_segment_edges = None
        self.unweld_hover_edge = None
        self.unweld_separate_forms = None
        self.surface_cursor_position = None
        self._show_tool_handler(title, widget, command_name)
        if tool in ("insert_point", "unweld"):
            self._install_selection_gate()
        # Insert Point intentionally keeps viewer picking enabled: its hover
        # state is driven by FreeCAD's native edge preselection. The Pivy
        # mouse callback still owns clicks, so this does not create selections.
        self._suspend_selection_for_tool(
            disable_selection=tool not in ("insert_point", "unweld")
        )
        self._create_surface_preview()
        self._reset_surface_tool_cache()
        try:
            if hasattr(self.view, "activateToolHandler"):
                self.view.activateToolHandler(self.surface_tool_cursor_icon)
            self.surface_tool_callback = self.view.addEventCallback(
                "SoEvent", self._surface_tool_event
            )
            self.surface_tool_mouse_callback = self.view.addEventCallbackPivy(
                coin.SoMouseButtonEvent.getClassTypeId(),
                self._surface_tool_mouse_event,
            )
        except Exception:
            self.stop_surface_tool()
            raise
        self._show_surface_tool_hints()
        return True

    def stop_insert_edge_tool(self):
        """Dismiss Insert Edge without ending the surrounding Form edit."""
        if self.insert_tool_active:
            self.stop_surface_tool()

    def stop_subdivide_tool(self):
        if self.subdivide_tool_active:
            self.stop_surface_tool()

    def stop_surface_tool(self):
        """Dismiss a hover topology handler without ending Form edit."""
        if (
            not self.surface_tool_active
            and self.surface_tool_callback is None
            and self.surface_tool_mouse_callback is None
        ):
            return
        self.active_tool = None
        self.surface_tool_cursor_icon = None
        if self.surface_tool_callback is not None:
            try:
                self.view.removeEventCallback("SoEvent", self.surface_tool_callback)
            except (AttributeError, RuntimeError):
                pass
            self.surface_tool_callback = None
        if self.surface_tool_mouse_callback is not None:
            try:
                self.view.removeEventCallbackPivy(
                    coin.SoMouseButtonEvent.getClassTypeId(),
                    self.surface_tool_mouse_callback,
                )
            except (AttributeError, RuntimeError):
                pass
            self.surface_tool_mouse_callback = None
        if hasattr(self.view, "deactivateToolHandler"):
            try:
                self.view.deactivateToolHandler()
            except (AttributeError, RuntimeError):
                pass
        self._resume_selection_after_tool()
        self._install_selection_gate()
        self._hide_tool_handler()
        self._clear_surface_preview()
        self.surface_cursor_position = None
        self.subdivide_u = None
        self.subdivide_v = None
        self.insert_whole_loop = None
        self.insert_point_chain = []
        self.insert_point_hover = None
        self.unweld_segment_edges = None
        self.unweld_hover_edge = None
        self.unweld_separate_forms = None
        self._reset_surface_tool_cache()
        if not self.cleaned:
            self._show_input_hints()
        self.view.redraw()
        # Command IsActive() depends on active_tool. Refresh it immediately;
        # otherwise the command manager waits for the next GUI/selection event.
        Gui.Command.update()

    def _insert_loop_changed(self, _checked):
        if self.insert_tool_active and self.surface_cursor_position is not None:
            self._update_insert_preview(self.surface_cursor_position)

    def _create_surface_preview(self):
        if self.surface_preview_switch is not None:
            return
        self.surface_preview_switch = coin.SoSwitch()
        preview = coin.SoSeparator()
        # Preview geometry is drawn in the editor's annotation graph, which is
        # still traversed by Coin's ray picker.  Without an explicit pick
        # style, the thick overlay line steals native preselection from the
        # Form edge beneath it and makes hover oscillate between unrelated
        # BRep edge numbers.
        pick_style = coin.SoPickStyle()
        pick_style.style = coin.SoPickStyle.UNPICKABLE
        material = coin.SoMaterial()
        material.diffuseColor = (0.1, 1.0, 0.2)
        material.emissiveColor = (0.1, 0.8, 0.15)
        draw_style = coin.SoDrawStyle()
        draw_style.lineWidth = 4.0
        self.surface_preview_coordinates = coin.SoCoordinate3()
        self.surface_preview_lines = coin.SoLineSet()
        preview.addChild(pick_style)
        preview.addChild(material)
        preview.addChild(draw_style)
        preview.addChild(self.surface_preview_coordinates)
        preview.addChild(self.surface_preview_lines)
        self.surface_preview_switch.addChild(preview)
        self.surface_preview_switch.whichChild = coin.SO_SWITCH_NONE
        self.root.addChild(self.surface_preview_switch)

    def _clear_surface_preview(self):
        self.surface_hover_face = None
        self.surface_preview_key = None
        if self.surface_preview_coordinates is not None:
            self.surface_preview_coordinates.point.setNum(0)
        if self.surface_preview_lines is not None:
            self.surface_preview_lines.numVertices.setNum(0)
        if self.surface_preview_switch is not None:
            self.surface_preview_switch.whichChild = coin.SO_SWITCH_NONE

    def _reset_surface_tool_cache(self):
        self.surface_tool_cache_mapper = None
        self.surface_tool_control_points = None
        self.surface_tool_shape_faces = {}
        self.surface_tool_hover_faces = {}

    def _ensure_surface_tool_cache(self, mapper):
        if mapper is self.surface_tool_cache_mapper:
            return
        self.surface_tool_cache_mapper = mapper
        self.surface_tool_control_points = self._control_surface_points()
        self.surface_tool_shape_faces = {}
        self.surface_tool_hover_faces = {}

    def _set_surface_preview_curves(self, curves):
        points = [(point.x, point.y, point.z) for curve in curves for point in curve]
        counts = [len(curve) for curve in curves]
        self.surface_preview_coordinates.point.setNum(len(points))
        self.surface_preview_coordinates.point.setValues(0, len(points), points)
        self.surface_preview_lines.numVertices.setNum(len(counts))
        self.surface_preview_lines.numVertices.setValues(0, len(counts), counts)
        self.surface_preview_switch.whichChild = coin.SO_SWITCH_ALL

    def _show_surface_tool_hints(self):
        if self.unweld_tool_active:
            Gui.HintManager.show(
                Gui.InputHint(
                    App.Qt.translate("Forms_Unweld", "%1 unweld the highlighted segment"),
                    Gui.UserInput.MouseLeft,
                ),
                Gui.InputHint(
                    App.Qt.translate("Forms_Unweld", "%1 cancel unweld"),
                    Gui.UserInput.MouseRight,
                ),
            )
            return
        if self.insert_point_tool_active:
            Gui.HintManager.show(
                Gui.InputHint(
                    App.Qt.translate("Forms_InsertPoint", "%1 place a point on an edge"),
                    Gui.UserInput.MouseLeft,
                ),
                Gui.InputHint(
                    App.Qt.translate("Forms_InsertPoint", "%1 commit the chain or exit"),
                    Gui.UserInput.MouseRight,
                ),
            )
            return
        action = (
            App.Qt.translate("Forms_Edit", "%1 insert the preview edge")
            if self.insert_tool_active
            else App.Qt.translate("Forms_Edit", "%1 subdivide the hovered face")
        )
        switch = (
            App.Qt.translate("Forms_Edit", "%1 switch edge direction")
            if self.insert_tool_active
            else App.Qt.translate("Forms_Edit", "%1 swap U and V counts")
        )
        Gui.HintManager.show(
            Gui.InputHint(action, Gui.UserInput.MouseLeft),
            Gui.InputHint(switch, Gui.UserInput.KeyM),
            Gui.InputHint(
                App.Qt.translate("Forms_Edit", "%1 finish topology tool"),
                Gui.UserInput.MouseRight,
            ),
        )

    def _subdivision_count_changed(self, axis, value):
        spinbox = self.subdivide_u if axis == "u" else self.subdivide_v
        if spinbox is None:
            return
        previous = self.subdivide_last_counts[axis]
        allowed = (1, 2, 4, 8, 16)
        if value not in allowed:
            candidates = (
                [candidate for candidate in allowed if candidate > previous]
                if value > previous
                else [candidate for candidate in allowed if candidate < previous]
            )
            value = (
                (min(candidates) if value > previous else max(candidates))
                if candidates
                else previous
            )
            spinbox.blockSignals(True)
            spinbox.setValue(value)
            spinbox.blockSignals(False)
        self.subdivide_last_counts[axis] = value
        if self.subdivide_tool_active and self.surface_cursor_position is not None:
            self._update_subdivide_preview(self.surface_cursor_position)

    def toggle_surface_tool_orientation(self):
        if not self.surface_tool_active:
            return False
        if self.insert_point_tool_active or self.unweld_tool_active:
            return False
        if self.insert_tool_active:
            self.insert_orientation = 1 - self.insert_orientation
        else:
            u_value = self.subdivide_u.value()
            v_value = self.subdivide_v.value()
            self.subdivide_u.setValue(v_value)
            self.subdivide_v.setValue(u_value)
        if self.surface_cursor_position is not None:
            self._update_surface_tool_preview(self.surface_cursor_position)
        return True

    def _surface_tool_event(self, info):
        if self.cleaned or not self.surface_tool_active:
            return
        event_type = info.get("Type")
        if event_type == "SoKeyboardEvent" and info.get("State") == "DOWN":
            key = str(info.get("Key", "")).upper()
            if key == "M":
                self.toggle_surface_tool_orientation()
            elif key == "ESCAPE":
                self.stop_surface_tool()
            return
        if event_type == "SoLocation2Event":
            if self.insert_point_tool_active:
                self._restore_surface_tool_cursor()
            self.surface_cursor_position = tuple(info.get("Position", ()))
            # Unweld is driven exclusively by FreeCAD's native preselection.
            # Re-picking by screen position here made its wire flicker between
            # location events even while the native edge stayed preselected.
            if not self.unweld_tool_active:
                self._update_surface_tool_preview(self.surface_cursor_position)
            return

    def _surface_tool_mouse_event(self, event_callback):
        """Own mouse clicks while a topology handler is running."""
        if self.cleaned or not self.surface_tool_active:
            return
        event = event_callback.getEvent()
        if event.getState() != coin.SoButtonEvent.DOWN:
            return
        button = event.getButton()
        if button == coin.SoMouseButtonEvent.BUTTON2:
            # The dictionary callback cannot mark an event handled, which let
            # the navigation style open its context menu after dismissal.
            event_callback.setHandled()
            if self.insert_point_tool_active and self.insert_point_chain:
                self._commit_insert_point_chain()
            else:
                QtCore.QTimer.singleShot(0, self.stop_surface_tool)
        elif button == coin.SoMouseButtonEvent.BUTTON1:
            event_callback.setHandled()
            if self.insert_point_tool_active:
                self._append_insert_point()
            else:
                self._commit_surface_tool_preview()

    def _hovered_insert_point(self, position):
        """Return ``(edge, fraction, surface point)`` under the cursor."""
        info = self.view.getObjectInfo(tuple(position)) if len(position) == 2 else None
        try:
            mapper = self._control_element_mapper()
            self._ensure_surface_tool_cache(mapper)
        except (ValueError, RuntimeError):
            return None
        names = []
        picked = None
        try:
            preselection = Gui.Selection.getPreselection()
            object_name = str(getattr(preselection, "ObjectName", "") or "")
            document_name = str(
                getattr(preselection, "DocumentName", "") or self.obj.Document.Name
            )
            for raw_name in getattr(preselection, "SubElementNames", ()):
                form_name = self._form_selection_subelement(
                    document_name, object_name, raw_name
                )
                if form_name is not None:
                    names.append(form_name)
            picked_points = tuple(getattr(preselection, "PickedPoints", ()) or ())
            if picked_points:
                picked = self._global_placement(self.obj).inverse().multVec(
                    App.Vector(picked_points[0])
                )
        except (AttributeError, RuntimeError, TypeError, ValueError):
            pass
        # getObjectInfo remains a fallback for navigation styles which update
        # the mouse callback just before publishing their preselection object.
        if info:
            info_names = []
            for raw_name in (info.get("SubName"), info.get("Component")):
                form_name = self._form_selection_subelement(
                    info.get("Document", self.obj.Document.Name),
                    info.get("Object", ""),
                    raw_name,
                )
                if form_name is not None:
                    info_names.append(form_name)
            names.extend(info_names)
            if (
                info_names
                and all(axis in info for axis in ("x", "y", "z"))
            ):
                try:
                    picked = self._global_placement(self.obj).inverse().multVec(
                        App.Vector(float(info["x"]), float(info["y"]), float(info["z"]))
                    )
                except (TypeError, ValueError):
                    pass
        shape_edge = None
        mapped = ()
        for name in names:
            if not canonical_subelement_name(name).startswith("Edge"):
                continue
            for candidate in (str(name), canonical_subelement_name(name)):
                try:
                    element = self.obj.Shape.getElement(candidate)
                    indices = mapper.indices(element)
                except (Part.OCCError, RuntimeError, ValueError, IndexError):
                    continue
                if element.ShapeType == "Edge" and len(indices) == 2:
                    shape_edge, mapped = element, tuple(indices)
                    break
            if shape_edge is not None:
                break
        if shape_edge is None:
            return None
        edge = tuple(sorted(mapped))
        valid = (
            set(mapper.mesh.atomic_edges())
            if mapper.mesh is not None
            else set(mapper.cage.edge_counts())
        )
        if edge not in valid:
            return None
        try:
            first_parameter = float(shape_edge.FirstParameter)
            last_parameter = float(shape_edge.LastParameter)
            parameter = (
                float(shape_edge.Curve.parameter(picked))
                if picked is not None
                else (first_parameter + last_parameter) * 0.5
            )
            parameter = min(max(parameter, first_parameter), last_parameter)
            local_fraction = (parameter - first_parameter) / (
                last_parameter - first_parameter
            )
            refined_range = mapper.refined_edge_parameter_range(shape_edge)
            refined_match = refined_range is not None and refined_range[0] == edge
            if refined_match:
                _mapped_edge, range_start, range_end = refined_range
                fraction = range_start + (range_end - range_start) * local_fraction
            else:
                start = shape_edge.valueAt(first_parameter)
                end = shape_edge.valueAt(last_parameter)
                first_control = self.surface_tool_control_points[edge[0]]
                reverse = first_control.sub(end).Length < first_control.sub(start).Length
                fraction = 1.0 - local_fraction if reverse else local_fraction
            midpoint_on_edge = not refined_match or (
                min(range_start, range_end) - 1.0e-9
                <= 0.5
                <= max(range_start, range_end) + 1.0e-9
            )
            if midpoint_on_edge and abs(fraction - 0.5) <= 0.075:
                fraction = 0.5
                if refined_match:
                    local_fraction = (0.5 - range_start) / (range_end - range_start)
                    parameter = first_parameter + (
                        last_parameter - first_parameter
                    ) * local_fraction
                else:
                    parameter = (first_parameter + last_parameter) * 0.5
            point = shape_edge.valueAt(parameter)
        except (Part.OCCError, RuntimeError, TypeError, ValueError, ZeroDivisionError):
            return None
        return edge, max(1.0e-5, min(1.0 - 1.0e-5, fraction)), App.Vector(point)

    def _update_insert_point_preview(self, position):
        self.insert_point_hover = self._hovered_insert_point(position)
        if self.insert_point_hover is not None and self.insert_point_chain:
            previous_edge = self.insert_point_chain[-1][0]
            try:
                mapper = self._control_element_mapper()
                insert_point_face_target(
                    mapper.mesh or mapper.cage,
                    previous_edge,
                    self.insert_point_hover[0],
                )
            except (RuntimeError, TypeError, ValueError):
                # Perpendicular and otherwise incompatible edges are not
                # point targets: show only the already accepted chain.
                self.insert_point_hover = None
        points = [record[2] for record in self.insert_point_chain]
        if self.insert_point_hover is not None:
            points.append(self.insert_point_hover[2])
        if not points:
            self._clear_surface_preview()
        else:
            # A one-point curve is legal in the state model but SoLineSet does
            # not display it. A tiny cross keeps the first snap visible.
            curves = [points] if len(points) > 1 else []
            if len(points) == 1:
                scale = max(self.obj.Shape.BoundBox.DiagonalLength * 0.004, 1.0e-4)
                point = points[0]
                curves = [
                    [point.add(App.Vector(-scale, 0, 0)), point.add(App.Vector(scale, 0, 0))],
                    [point.add(App.Vector(0, -scale, 0)), point.add(App.Vector(0, scale, 0))],
                ]
            self._set_surface_preview_curves(curves)
        self.view.redraw()

    def _update_unweld_preview(self, edge):
        """Highlight the complete separating edge loop under the pointer."""
        preview_key = ("unweld", tuple(edge))
        if (
            self.surface_preview_key == preview_key
            and self.surface_preview_switch.whichChild.getValue() != coin.SO_SWITCH_NONE
        ):
            return
        self.unweld_segment_edges = None
        try:
            cage = ControlCage.from_object(self.obj)
            if (
                getattr(self.obj, "LocalEdgeInserts", ())
                or str(getattr(self.obj, "TMeshData", "") or "")
                or getattr(self.obj, "DissolvedEdges", ())
            ):
                raise ValueError("Unweld requires an all-quad base control cage")
            if not cage.is_closed:
                raise ValueError("Unweld currently requires a closed Form")
            segment = tuple(cage_edge_loop(cage.faces, edge))
            # Do the same validation as commit so a visually accepted preview
            # can never turn into a non-separating or partial cut on click.
            cage.split_along_edges(segment)
            mapper = self._control_element_mapper()
            segment_set = set(segment)
            curves = []
            for shape_edge in self.obj.Shape.Edges:
                mapped = tuple(sorted(mapper.indices(shape_edge)))
                if len(mapped) == 2 and mapped in segment_set:
                    curves.append(
                        [App.Vector(point) for point in shape_edge.discretize(Number=17)]
                    )
        except (Part.OCCError, RuntimeError, TypeError, ValueError, IndexError):
            self._clear_surface_preview()
            self.view.redraw()
            return
        if not curves:
            self._clear_surface_preview()
            self.view.redraw()
            return
        self.unweld_segment_edges = segment
        self._set_surface_preview_curves(curves)
        self.surface_preview_key = preview_key
        self.view.redraw()

    def _commit_unweld_preview(self):
        if not self.unweld_tool_active or not self.unweld_segment_edges:
            return False
        self._flush_pending_updates()
        transaction = self._begin_action(App.Qt.translate("Forms_Unweld", "Unweld Form"))
        try:
            unweld_segment(
                self.obj,
                self.unweld_segment_edges,
                separate_forms=(
                    self.unweld_separate_forms is not None
                    and self.unweld_separate_forms.isChecked()
                ),
            )
            self.obj.Document.recompute()
            self.cached_control_mapper = None
            self.cached_control_mapper_signature = None
            self._reset_surface_tool_cache()
            self._clear_editor_selection(clear_preselection=True)
            self.topology_changed()
            self._clear_surface_preview()
            self.view.redraw()
        except MODELING_ERRORS as error:
            self._finish_action(transaction, commit=False)
            return report_modeling_error(
                App.Qt.translate("Forms_Unweld", "Unweld Form"), error
            )
        except Exception:
            self._finish_action(transaction, commit=False)
            raise
        self._finish_action(transaction)
        QtCore.QTimer.singleShot(0, self.stop_surface_tool)
        return True

    def _append_insert_point(self):
        if not self.insert_point_tool_active or self.insert_point_hover is None:
            return False
        edge, fraction, point = self.insert_point_hover
        if self.insert_point_chain:
            previous_edge, previous_fraction, _previous_point = self.insert_point_chain[-1]
            try:
                mapper = self._control_element_mapper()
                insert_point_face_target(
                    mapper.mesh or mapper.cage,
                    previous_edge,
                    edge,
                )
            except (RuntimeError, TypeError, ValueError):
                return False
            if previous_edge == edge and math.isclose(previous_fraction, fraction, abs_tol=1.0e-6):
                return False
        self.insert_point_chain.append((edge, fraction, App.Vector(point)))
        if self.surface_cursor_position is not None:
            self._update_insert_point_preview(self.surface_cursor_position)
        return True

    def _commit_insert_point_chain(self):
        if not self.insert_point_tool_active or not self.insert_point_chain:
            return False
        if len(self.insert_point_chain) < 2:
            self.insert_point_chain = []
            self.insert_point_hover = None
            self._clear_surface_preview()
            self.view.redraw()
            return True
        self._flush_pending_updates()
        transaction = self._begin_action(App.Qt.translate("Forms_InsertPoint", "Insert points"))
        try:
            insert_point_edges(
                self.obj,
                [(edge, fraction) for edge, fraction, _point in self.insert_point_chain],
            )
            self.obj.Document.recompute()
            self._set_parametric_state(False)
            self._clear_editor_selection(clear_preselection=True)
            self._sync_dimension_properties()
            self._update_dimension_gizmos()
            self._reset_surface_tool_cache()
            self.insert_point_chain = []
            self.insert_point_hover = None
            self._clear_surface_preview()
            self._queue_tool_selection_clear()
            self.view.redraw()
        except MODELING_ERRORS as error:
            self._finish_action(transaction, commit=False)
            return report_modeling_error(
                App.Qt.translate("Forms_InsertPoint", "Insert points"), error
            )
        except Exception:
            self._finish_action(transaction, commit=False)
            raise
        self._finish_action(transaction)
        return True

    def _hovered_control_face(self, position):
        info = self.view.getObjectInfo(tuple(position)) if len(position) == 2 else None
        if not info or str(info.get("Object", "")) != self.obj.Name:
            return None
        names = [info.get("SubName"), info.get("Component")]
        cache_key = tuple(str(name or "") for name in names)
        if cache_key in self.surface_tool_hover_faces:
            return self.surface_tool_hover_faces[cache_key]
        try:
            mapper = self._control_element_mapper()
            self._ensure_surface_tool_cache(mapper)
        except (ValueError, RuntimeError):
            return None
        for name in names:
            canonical = canonical_subelement_name(name)
            if not canonical.startswith("Face"):
                continue
            for candidate in (str(name), canonical):
                try:
                    element = self.obj.Shape.getElement(candidate)
                    if str(self.obj.FormType) == "Forms::Surface" and any(
                        element.isSame(face) for face in mapper.form_surface_faces
                    ):
                        return 0
                    mapped = mapper.indices(element)
                    face_index = mapper.face_id(mapped)
                    if face_index is not None:
                        self.surface_tool_hover_faces[cache_key] = face_index
                        return face_index
                except (Part.OCCError, RuntimeError, ValueError, IndexError):
                    continue
        self.surface_tool_hover_faces[cache_key] = None
        return None

    def _shape_face_for_control_face(self, face_index, mapper):
        self._ensure_surface_tool_cache(mapper)
        if face_index in self.surface_tool_shape_faces:
            return self.surface_tool_shape_faces[face_index]
        if str(self.obj.FormType) == "Forms::Surface":
            result = mapper.form_surface_faces[0] if mapper.form_surface_faces else None
            self.surface_tool_shape_faces[face_index] = result
            return result
        target_face = mapper.mesh.faces[face_index] if mapper.mesh is not None else None
        target = frozenset(
            target_face.boundary
            if target_face is not None
            else ControlCage.from_object(self.obj).faces[face_index]
        )
        for shape_face in mapper.shape.Faces:
            mapped = frozenset(mapper.indices(shape_face))
            if mapped == target or (
                target_face is not None and mapped == frozenset(target_face.corners)
            ):
                self.surface_tool_shape_faces[face_index] = shape_face
                return shape_face
        self.surface_tool_shape_faces[face_index] = None
        return None

    @staticmethod
    def _form_surface_domain(mapper):
        if not mapper.form_surface_faces:
            return None
        ranges = [face.ParameterRange for face in mapper.form_surface_faces]
        return (
            mapper.form_surface_faces[0].Surface,
            min(bounds[0] for bounds in ranges),
            max(bounds[1] for bounds in ranges),
            min(bounds[2] for bounds in ranges),
            max(bounds[3] for bounds in ranges),
        )

    def _insert_curve_points(self, shape_face, cage_face, insert_edge, surface_points=None):
        if hasattr(cage_face, "sides"):
            edge_position = next(
                index
                for index, side in enumerate(cage_face.sides)
                if any(
                    tuple(sorted((start, end))) == insert_edge for start, end in zip(side, side[1:])
                )
            )
            corners = cage_face.corners
        else:
            corners = cage_face
            edge_position = next(
                index
                for index, start in enumerate(corners)
                if tuple(sorted((start, corners[(index + 1) % 4]))) == insert_edge
            )
        cross_edges = (
            (corners[(edge_position + 1) % 4], corners[(edge_position + 2) % 4]),
            (corners[(edge_position + 3) % 4], corners[edge_position]),
        )
        surface_points = surface_points or self._control_surface_points()
        anchors = [
            surface_points[first].add(surface_points[second]).multiply(0.5)
            for first, second in cross_edges
        ]
        u_min, u_max, v_min, v_max = shape_face.ParameterRange
        surface = shape_face.Surface
        count = 17
        candidates = [
            [
                surface.value((u_min + u_max) * 0.5, v_min + (v_max - v_min) * i / (count - 1))
                for i in range(count)
            ],
            [
                surface.value(u_min + (u_max - u_min) * i / (count - 1), (v_min + v_max) * 0.5)
                for i in range(count)
            ],
        ]

        def endpoint_cost(points):
            direct = points[0].sub(anchors[0]).Length + points[-1].sub(anchors[1]).Length
            reverse = points[0].sub(anchors[1]).Length + points[-1].sub(anchors[0]).Length
            return min(direct, reverse)

        return min(candidates, key=endpoint_cost)

    @staticmethod
    def _whole_loop_data(topology, face_index, parallel_edge):
        """Resolve the cage ring whose inserted seam is parallel to an edge."""
        face = topology.faces[face_index]
        edge_position = next(
            index
            for index, start in enumerate(face)
            if tuple(sorted((start, face[(index + 1) % 4]))) == parallel_edge
        )
        # ControlCage.insert_edge_ring() splits its ring edges and joins their
        # new points. Seed it with an adjacent edge so the resulting seam is
        # parallel to the handler preview/selected side.
        ring_start = tuple(sorted((face[(edge_position + 1) % 4], face[(edge_position + 2) % 4])))
        ring_edges = set(topology.edge_ring(ring_start))
        target_faces = tuple(
            candidate_index
            for candidate_index, candidate in enumerate(topology.faces)
            if any(
                tuple(sorted((start, candidate[(edge_index + 1) % 4]))) in ring_edges
                for edge_index, start in enumerate(candidate)
            )
        )
        return ring_start, ring_edges, target_faces

    def _update_insert_preview(self, position):
        face_index = self._hovered_control_face(position)
        if face_index is None:
            self._clear_surface_preview()
            self.view.redraw()
            return
        whole_loop = bool(self.insert_whole_loop is not None and self.insert_whole_loop.isChecked())
        preview_key = (
            face_index,
            self.insert_orientation,
            whole_loop,
        )
        if (
            preview_key == self.surface_preview_key
            and self.surface_preview_switch.whichChild.getValue() != coin.SO_SWITCH_NONE
        ):
            return
        self._clear_surface_preview()
        if str(self.obj.FormType) == "Forms::Surface":
            try:
                mapper = self._control_element_mapper()
                domain = self._form_surface_domain(mapper)
                if domain is None:
                    return
                surface, u_min, u_max, v_min, v_max = domain
                count = 17
                if self.insert_orientation == 0:
                    curve = [
                        surface.value(
                            (u_min + u_max) * 0.5,
                            v_min + (v_max - v_min) * index / (count - 1),
                        )
                        for index in range(count)
                    ]
                else:
                    curve = [
                        surface.value(
                            u_min + (u_max - u_min) * index / (count - 1),
                            (v_min + v_max) * 0.5,
                        )
                        for index in range(count)
                    ]
            except (Part.OCCError, RuntimeError, ValueError, IndexError):
                self.view.redraw()
                return
            self._set_surface_preview_curves([curve])
            self.surface_preview_key = preview_key
            self.surface_hover_face = face_index
            self.view.redraw()
            return
        try:
            mapper = self._control_element_mapper()
            topology = mapper.mesh or ControlCage.from_object(self.obj)
            insert_edge, target_faces, _resolved_side = local_insert_target(
                topology,
                face_index,
                self.insert_orientation,
                "left",
            )
            ring_edges = None
            if whole_loop:
                if mapper.mesh is not None:
                    raise ValueError("Whole-loop insertion currently requires an all-quad cage")
                _ring_start, ring_edges, target_faces = self._whole_loop_data(
                    topology, face_index, insert_edge
                )
            curves = []
            for target_index in target_faces:
                shape_face = self._shape_face_for_control_face(target_index, mapper)
                if shape_face is not None:
                    cage_face = (
                        mapper.mesh.faces[target_index]
                        if mapper.mesh is not None
                        else topology.faces[target_index]
                    )
                    preview_edge = insert_edge
                    if ring_edges is not None:
                        preview_edge = next(
                            tuple(sorted((start, cage_face[(edge_index + 1) % 4])))
                            for edge_index, start in enumerate(cage_face)
                            if tuple(sorted((start, cage_face[(edge_index + 1) % 4])))
                            not in ring_edges
                        )
                    curves.append(
                        self._insert_curve_points(
                            shape_face,
                            cage_face,
                            preview_edge,
                            self.surface_tool_control_points,
                        )
                    )
        except (Part.OCCError, RuntimeError, ValueError, IndexError):
            self.view.redraw()
            return
        if not curves:
            self.view.redraw()
            return
        self._set_surface_preview_curves(curves)
        self.surface_preview_key = preview_key
        self.surface_hover_face = face_index
        self.view.redraw()

    def _update_subdivide_preview(self, position):
        self._clear_surface_preview()
        face_index = self._hovered_control_face(position)
        if face_index is None:
            self.view.redraw()
            return
        try:
            mapper = self._control_element_mapper()
            u_count = self.subdivide_u.value()
            v_count = self.subdivide_v.value()
            if str(self.obj.FormType) == "Forms::Surface":
                domain = self._form_surface_domain(mapper)
                if domain is None:
                    return
                surface, u_min, u_max, v_min, v_max = domain
            else:
                shape_face = self._shape_face_for_control_face(face_index, mapper)
                if shape_face is None:
                    return
                u_min, u_max, v_min, v_max = shape_face.ParameterRange
                surface = shape_face.Surface
            sample_count = 17
            curves = []
            for division in range(1, u_count):
                parameter = u_min + (u_max - u_min) * division / u_count
                curves.append(
                    [
                        surface.value(
                            parameter,
                            v_min + (v_max - v_min) * index / (sample_count - 1),
                        )
                        for index in range(sample_count)
                    ]
                )
            for division in range(1, v_count):
                parameter = v_min + (v_max - v_min) * division / v_count
                curves.append(
                    [
                        surface.value(
                            u_min + (u_max - u_min) * index / (sample_count - 1),
                            parameter,
                        )
                        for index in range(sample_count)
                    ]
                )
        except (Part.OCCError, RuntimeError, ValueError, IndexError):
            self.view.redraw()
            return
        if not curves:
            self.view.redraw()
            return
        points = [(point.x, point.y, point.z) for curve in curves for point in curve]
        counts = [len(curve) for curve in curves]
        self.surface_preview_coordinates.point.setValues(0, len(points), points)
        self.surface_preview_lines.numVertices.setValues(0, len(counts), counts)
        self.surface_preview_switch.whichChild = coin.SO_SWITCH_ALL
        self.surface_hover_face = face_index
        self.view.redraw()

    def _update_surface_tool_preview(self, position):
        if self.insert_tool_active:
            self._update_insert_preview(position)
        elif self.insert_point_tool_active:
            self._update_insert_point_preview(position)
        elif self.subdivide_tool_active:
            self._update_subdivide_preview(position)

    def _commit_surface_tool_preview(self):
        if self.insert_tool_active:
            return self._commit_insert_preview()
        if self.subdivide_tool_active:
            return self._commit_subdivide_preview()
        if self.unweld_tool_active:
            return self._commit_unweld_preview()
        return False

    def _commit_insert_preview(self):
        if self.cleaned or not self.insert_tool_active or self.surface_hover_face is None:
            return False
        self._flush_pending_updates()
        transaction = self._begin_action(App.Qt.translate("Forms_Edit", "Insert form edge"))
        try:
            if str(self.obj.FormType) == "Forms::Surface":
                property_name = "USegments" if self.insert_orientation == 0 else "VSegments"
                self._set_form_surface_segments(
                    int(self.obj.USegments) + (property_name == "USegments"),
                    int(self.obj.VSegments) + (property_name == "VSegments"),
                )
            else:
                whole_loop = bool(
                    self.insert_whole_loop is not None and self.insert_whole_loop.isChecked()
                )
                if whole_loop:
                    mapper = self._control_element_mapper()
                    if mapper.mesh is not None:
                        raise ValueError("Whole-loop insertion currently requires an all-quad cage")
                    topology = ControlCage.from_object(self.obj)
                    insert_edge, _targets, _side = local_insert_target(
                        topology,
                        self.surface_hover_face,
                        self.insert_orientation,
                        "left",
                    )
                    ring_start, _ring_edges, _faces = self._whole_loop_data(
                        topology, self.surface_hover_face, insert_edge
                    )
                    insert_edge_loop(self.obj, ring_start)
                else:
                    insert_edge_on_face(
                        self.obj,
                        self.surface_hover_face,
                        self.insert_orientation,
                        "left",
                    )
            self.obj.Document.recompute()
            self._reset_surface_tool_cache()
            self._set_parametric_state(False)
            self._clear_editor_selection(clear_preselection=True)
            self._sync_dimension_properties()
            self._update_dimension_gizmos()
            self._clear_surface_preview()
            self._queue_tool_selection_clear()
            self.view.redraw()
        except MODELING_ERRORS as error:
            self._finish_action(transaction, commit=False)
            report_modeling_error(App.Qt.translate("Forms_InsertEdge", "Insert Edge"), error)
            self._clear_surface_preview()
            self.view.redraw()
            return False
        except Exception:
            self._finish_action(transaction, commit=False)
            raise
        self._finish_action(transaction)
        return True

    def _commit_subdivide_preview(self):
        if self.cleaned or not self.subdivide_tool_active or self.surface_hover_face is None:
            return False
        self._flush_pending_updates()
        transaction = self._begin_action(App.Qt.translate("Forms_Edit", "Subdivide form face"))
        try:
            if str(self.obj.FormType) == "Forms::Surface":
                self._set_form_surface_segments(
                    int(self.obj.USegments) * self.subdivide_u.value(),
                    int(self.obj.VSegments) * self.subdivide_v.value(),
                )
            else:
                subdivide_faces(
                    self.obj,
                    (self.surface_hover_face,),
                    self.subdivide_u.value(),
                    self.subdivide_v.value(),
                )
            self.obj.Document.recompute()
            self._reset_surface_tool_cache()
            self._set_parametric_state(False)
            self._clear_editor_selection(clear_preselection=True)
            self._sync_dimension_properties()
            self._update_dimension_gizmos()
            self._clear_surface_preview()
            self._queue_tool_selection_clear()
            self.view.redraw()
        except MODELING_ERRORS as error:
            self._finish_action(transaction, commit=False)
            report_modeling_error(App.Qt.translate("Forms_Subdivide", "Subdivide"), error)
            self._clear_surface_preview()
            self.view.redraw()
            return False
        except Exception:
            self._finish_action(transaction, commit=False)
            raise
        self._finish_action(transaction)
        return True

    def _set_form_surface_segments(self, u_segments, v_segments):
        """Resize the regular control grid used by a filled Part Design face."""
        u_segments = int(u_segments)
        v_segments = int(v_segments)
        if self.obj.CageMode == "Parametric":
            self.obj.USegments = u_segments
            self.obj.VSegments = v_segments
            self.obj.Document.recompute()
            for name, value in (
                ("USegments", u_segments),
                ("VSegments", v_segments),
            ):
                blocker = QtCore.QSignalBlocker(self.parameter_widgets[name])
                self.parameter_widgets[name].setValue(value)
                del blocker
            return
        if u_segments > int(self.obj.USegments):
            self._increase_segments("USegments", u_segments)
        if v_segments > int(self.obj.VSegments):
            self._increase_segments("VSegments", v_segments)

    def _deferred_clear_tool_selection(self):
        if not self.cleaned and self.surface_tool_active:
            self._clear_editor_selection(clear_preselection=True)

    def _restore_surface_tool_cursor(self):
        """Reapply the active topology tool cursor after viewer event routing."""
        if (
            self.cleaned
            or not self.surface_tool_active
            or not self.surface_tool_cursor_icon
            or not hasattr(self.view, "activateToolHandler")
        ):
            return
        try:
            self.view.activateToolHandler(self.surface_tool_cursor_icon)
        except (AttributeError, RuntimeError):
            pass

    def _finish_surface_tool_click(self):
        """Restore transient viewer state after applying a topology operation."""
        self._deferred_clear_tool_selection()
        self._restore_surface_tool_cursor()

    def _queue_tool_selection_clear(self):
        """Restore tool state after the viewer finishes routing its click."""
        # Recompute and the navigation style finish at different points in the
        # event loop.  Each can replace the custom handler cursor, particularly
        # after Insert Point commits on right-click, so restore after both the
        # immediate click dispatch and the later view-provider redraw.
        for delay in (0, 50, 200):
            QtCore.QTimer.singleShot(delay, self._finish_surface_tool_click)


__all__ = ["FormEditToolsMixin"]
