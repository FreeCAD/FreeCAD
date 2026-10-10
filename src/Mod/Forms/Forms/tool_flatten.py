# SPDX-License-Identifier: LGPL-2.1-or-later
"""Flatten tool controls, preview, and commit/cancel lifecycle."""

import FreeCAD as App
import FreeCADGui as Gui
import Part
from .cage import canonical_subelement_name
from .feedback import MODELING_ERRORS, report_modeling_error
from .operations import flatten_control_points, preview_flatten_control_points
from .taskpanels import load_panel
from .tool_controller import ToolController


class FlattenTool(ToolController):
    def __init__(self, session):
        super().__init__(session)
        self.flatten_indices = []
        self.flatten_mode = None
        self.flatten_reference = None
        self.flatten_reference_button = None
        self.flatten_reference_name = None
        self.flatten_reference_widget = None
        self.flatten_reference_selecting = False
        self.flatten_apply_button = None
        self.flatten_cancel_button = None
        self.flatten_preview_status = None
        self.flatten_preview_root = None
        self.flatten_preview_switch = None
        self.flatten_preview_shape = Part.Shape()
        self.flatten_visibility_before = []

    def start_flatten_tool(self, indices):
        """Open Flatten for the selected controls without changing the Form."""
        if self.session.cleaned or self.session.has_active_tool() or len(set(indices)) < 3:
            return False
        self.session._flush_pending_updates()
        self.flatten_indices = sorted({int(index) for index in indices})
        self.session.active_tool = "flatten"
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
        self.session._clear_editor_selection()
        self.session._show_tool_handler(
            App.Qt.translate("Forms_Edit", "Flatten"), widget, "Forms_Flatten"
        )
        self.session._suspend_selection_for_tool()
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
        if not self.session.flatten_tool_active:
            return
        self.flatten_reference_selecting = bool(active)
        if active:
            if self.session.selection_gate_added:
                Gui.Selection.removeSelectionGate()
                self.session.selection_gate_added = False
                self.session.selection_gate = None
            self.flatten_reference_name.clear()
            self.flatten_reference_name.setPlaceholderText(
                App.Qt.translate("Forms_Edit", "Face selection active")
            )
            self.flatten_preview_status.setText(
                App.Qt.translate("Forms_Edit", "Select a planar face or plane")
            )
        else:
            self.session._install_selection_gate()
            if self.flatten_reference is None:
                self.flatten_reference_name.setPlaceholderText(
                    App.Qt.translate("Forms_Edit", "No face selected")
                )

    def _flatten_reference_from_event(self, document, object_name, subelement):
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
        self.session.suppress_selection_observer = True
        try:
            Gui.Selection.clearSelection()
        finally:
            self.session.suppress_selection_observer = False
        self._update_flatten_preview()

    def _flatten_plane(self):
        mode = str(self.flatten_mode.currentData())
        if mode == "BestFit":
            return None
        if mode == "Reference":
            if self.flatten_reference is None:
                raise ValueError("Select a reference face or plane")
            source, face_name = self.flatten_reference
            source_placement = self.session._global_placement(source)
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
            inverse = self.session._global_placement(self.session.obj).inverse()
            local_normal = inverse.Rotation.multVec(normal)
            return (
                None,
                (local_normal.x, local_normal.y, local_normal.z),
            )
        inverse = self.session._global_placement(self.session.obj).inverse()
        local_origin = inverse.multVec(origin)
        local_normal = inverse.Rotation.multVec(normal)
        return (
            (local_origin.x, local_origin.y, local_origin.z),
            (local_normal.x, local_normal.y, local_normal.z),
        )

    def _start_flatten_preview_visibility(self):
        self.flatten_visibility_before = []
        base = getattr(self.session.obj, "BaseFeature", None)
        for feature in ([base] if base is not None else []) + [self.session.obj]:
            view_object = getattr(feature, "ViewObject", None)
            if view_object is not None:
                self.flatten_visibility_before.append((feature, bool(view_object.Visibility)))
        self.session.obj.ViewObject.Visibility = False
        if base is not None and getattr(base, "ViewObject", None) is not None:
            base.ViewObject.Visibility = True

    def _restore_flatten_preview_visibility(self):
        for feature, visible in self.flatten_visibility_before:
            if feature.Document is not None:
                feature.ViewObject.Visibility = visible
        self.flatten_visibility_before = []

    def _set_flatten_preview_shape(self, shape):
        self._clear_flatten_preview()
        self.flatten_preview_root, self.flatten_preview_switch = self.session._make_shape_preview(shape)
        self.flatten_preview_shape = shape

    def _clear_flatten_preview(self):
        self.session._remove_shape_preview(self.flatten_preview_root)
        self.flatten_preview_root = None
        self.flatten_preview_switch = None
        self.flatten_preview_shape = Part.Shape()

    def _update_flatten_preview(self, _index=None):
        if not self.session.flatten_tool_active:
            return
        try:
            shape = preview_flatten_control_points(
                self.session.obj, self.flatten_indices, self._flatten_plane()
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
        self.session.view.redraw()

    def apply_flatten_tool(self):
        if not self.session.flatten_tool_active:
            return False
        plane = self._flatten_plane()
        transaction = self.session._begin_action(
            App.Qt.translate("Forms_Edit", "Flatten form controls")
        )
        try:
            flatten_control_points(self.session.obj, self.flatten_indices, plane)
            self.session.obj.Document.recompute()
            self.session.topology_changed()
        except MODELING_ERRORS as error:
            self.session._finish_action(transaction, commit=False)
            return report_modeling_error(
                App.Qt.translate("Forms_Flatten", "Flatten"),
                error,
                self.flatten_preview_status,
            )
        except Exception:
            self.session._finish_action(transaction, commit=False)
            raise
        self.session._finish_action(transaction)
        self.stop_flatten_tool()
        return True

    def stop_flatten_tool(self):
        if not self.session.flatten_tool_active:
            return
        if self.flatten_reference_selecting and self.flatten_reference_button is not None:
            self.flatten_reference_button.setChecked(False)
        self._clear_flatten_preview()
        self._restore_flatten_preview_visibility()
        self.session.active_tool = None
        self.flatten_indices = []
        self.flatten_reference = None
        self.flatten_mode = None
        self.flatten_reference_button = None
        self.flatten_reference_name = None
        self.flatten_reference_widget = None
        self.flatten_apply_button = None
        self.flatten_cancel_button = None
        self.flatten_preview_status = None
        self.session._resume_selection_after_tool()
        self.session._hide_tool_handler()
        if not self.session.cleaned:
            self.session._show_input_hints()
        self.session.view.redraw()
        Gui.Command.update()
