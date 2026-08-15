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

"""Interactive control-cage editing for Forms objects."""

import time

import FreeCAD as App
import FreeCADGui as Gui
import Part
from PySide import QtCore, QtGui, QtWidgets
from pivy import coin

from .cage import (
    ControlCage,
    ControlElementMapper,
    canonical_subelement_name,
    control_surface_points,
)
from .edit_tools import FormEditToolsMixin
from .feedback import MODELING_ERRORS, report_modeling_error
from .interaction import FormKeyFilter, FormSelectionGate
from .symmetry import control_pairs, reflected
from .taskpanels import bind_edit_panel, bind_tool_panel
from .toolbar import set_forms_toolbar_mode
from .topology import (
    box_control_cage,
    cage_edge_selection_range,
    cage_edge_loop,
    cage_edges,
    cage_face_selection_range,
    cage_vertex_selection_range,
    connected_edge_component,
    cylinder_control_cage,
    face_control_cage,
    quadball_control_cage,
    resize_structured_cage,
)

PRIMITIVE_PARAMETERS = {
    "Forms::Box": (
        ("Length", "Length", "length"),
        ("Width", "Width", "length"),
        ("Height", "Height", "length"),
        ("XSegments", "X segments", "integer"),
        ("YSegments", "Y segments", "integer"),
        ("ZSegments", "Z segments", "integer"),
    ),
    "Forms::Cylinder": (
        ("Radius", "Radius", "length"),
        ("Height", "Height", "length"),
        ("SideSegments", "Segments per quadrant", "integer"),
        ("HeightSegments", "Height segments", "integer"),
    ),
    "Forms::Quadball": (("Radius", "Radius", "length"), ("Segments", "Segments", "integer")),
    "Forms::Sphere": (
        ("Radius", "Radius", "length"),
        ("LongitudeSegments", "Longitude segments", "integer"),
        ("LatitudeSegments", "Latitude segments", "integer"),
    ),
    "Forms::Pipe": (
        ("Diameter", "Global diameter", "length"),
        ("SectionSegments", "Section density (8 sides per level)", "integer"),
        ("PathSamples", "Segments per edge", "integer"),
    ),
    "Forms::Face": (
        ("Length", "Length", "length"),
        ("Width", "Width", "length"),
        ("XSegments", "X segments", "integer"),
        ("YSegments", "Y segments", "integer"),
    ),
    "Forms::Surface": (
        ("USegments", "U segments", "integer"),
        ("VSegments", "V segments", "integer"),
    ),
    "Forms::Torus": (
        ("MajorRadius", "Major radius", "length"),
        ("MinorRadius", "Minor radius", "length"),
        ("MajorSegments", "Major segments per quadrant", "integer"),
        ("MinorSegments", "Minor segments per quadrant", "integer"),
    ),
    "Forms::Tube": (
        ("OuterRadius", "Outer radius", "length"),
        ("InnerRadius", "Inner radius", "length"),
        ("Height", "Height", "length"),
        ("SideSegments", "Segments per quadrant", "integer"),
        ("HeightSegments", "Height segments", "integer"),
    ),
}

_active_session = None


def active_form_session(obj=None):
    """Return the live Forms task session, optionally restricted to *obj*."""
    session = _active_session
    if session is None or session.cleaned:
        return None
    return session if obj is None or session.obj == obj else None


def finish_active_form_session():
    """Accept and close the live Forms editor, irrespective of active document."""
    session = active_form_session()
    if session is None:
        return False
    if session.document_edit:
        gui_document = Gui.getDocument(session.obj.Document.Name)
        gui_document.resetEdit()
    else:
        session.accept()
    return True


class FormEditSession(FormEditToolsMixin):
    """Edit cage points using FreeCAD's standard transform dragger."""

    def __init__(self, obj, document_edit=False, creation_transaction=False):
        self.obj = obj
        self.document_edit = document_edit
        self.creation_transaction = creation_transaction
        self.view_object = obj.ViewObject
        self.view = Gui.activeDocument().activeView()
        self.edit_backup = None
        self.editing_cancelled = False
        self.profile_edit_shape_owned = False
        self.viewer = None
        self.previous_pick_radius = None
        self.previous_point_size = None
        self.root = coin.SoAnnotation()
        self.selected = []
        self.whole_form_selected = False
        self.dragger = None
        self.dragger_switch = None
        self.dragger_callbacks = []
        self.last_added_edge = None
        self.range_selection_anchors = {}
        self.range_selection_generation = 0
        self.dimension_gizmos = {}
        self.dimension_gizmo_switches = {}
        self.dimension_gizmo_callbacks = []
        self.base_dimension_gizmo_frames = {}
        self.dimension_base_points = []
        self.dimension_base_value = 0.0
        self.pending_parameter_changes = {}
        self.pending_sharpness = None
        self.cached_control_mapper = None
        self.cached_control_mapper_signature = None
        self.cached_element_targets = {}
        self.current_sharpness_targets = (set(), set())
        self.current_sharpness_restore_targets = (set(), set(), set())
        self.parameter_update_timer = QtCore.QTimer()
        self.parameter_update_timer.setSingleShot(True)
        self.parameter_update_timer.setInterval(300)
        self.parameter_update_timer.timeout.connect(self._apply_pending_parameter_changes)
        self.sharpness_update_timer = QtCore.QTimer()
        self.sharpness_update_timer.setSingleShot(True)
        self.sharpness_update_timer.setInterval(300)
        self.sharpness_update_timer.timeout.connect(self._apply_pending_sharpness)
        self.selection_observer_added = False
        self.document_observer_added = False
        self.key_filter = None
        self.selection_gate = None
        self.selection_gate_added = False
        self.selection_sync_generation = 0
        self.dragger_reveal_deadline = 0.0
        self.suppress_selection_observer = False
        self.camera_sensor = None
        self.camera_orientation_sensor = None
        self.camera = None
        self.dragger_scale_node = None
        self.syncing = False
        self.cleaned = False
        self.base_points = []
        self.base_center = App.Vector()
        self.base_dragger_rotation = App.Rotation()
        self.base_form_placement = App.Placement()
        self.base_object_placement = App.Placement()
        self.pending_form_placement = None
        self.pending_control_points = None
        self.whole_form_motion_preview = False
        self.alt_extrude_face_indices = set()
        self.alt_extrude_boundary_edges = set()
        self.extruded_top_faces = set()
        self.extruded_outer_edges = set()
        self.active_tool = None
        self.surface_tool_callback = None
        self.surface_tool_mouse_callback = None
        self.surface_tool_cursor_icon = None
        self.pivot_tool_callback = None
        self.pivot_tool_mouse_callback = None
        self.pivot_snap_point = None
        self.pivot_previous_selection_filter = None
        self.pivot_selection_snapshot = None
        self.pivot_pick_pending = False
        self.surface_hover_face = None
        self.surface_cursor_position = None
        self.insert_orientation = 0
        self.insert_whole_loop = None
        self.insert_point_chain = []
        self.insert_point_hover = None
        self.unweld_segment_edges = None
        self.unweld_hover_edge = None
        self.unweld_separate_forms = None
        self.surface_preview_switch = None
        self.surface_preview_coordinates = None
        self.surface_preview_lines = None
        self.surface_preview_key = None
        self.surface_tool_cache_mapper = None
        self.surface_tool_control_points = None
        self.surface_tool_shape_faces = {}
        self.surface_tool_hover_faces = {}
        self.tool_handler_panel = None
        self.tool_handler_layout = None
        self.tool_handler_widget = None
        self.subdivide_u = None
        self.subdivide_v = None
        self.surface_tangent = None
        self.match_inputs = None
        self.match_mode = None
        self.match_apply_button = None
        self.match_cancel_button = None
        self.match_preview_status = None
        self.match_preview_root = None
        self.match_preview_switch = None
        self.match_preview_shape = Part.Shape()
        self.match_visibility_before = []
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
        self.subdivide_last_counts = {"u": 2, "v": 2}
        self.thicken_original_cage = None
        self.thicken_original_mode = None
        self.thicken_transaction_open = False
        self.thicken_distance = None
        self.thicken_apply_button = None
        self.thicken_cancel_button = None
        self.dragger_transaction_open = False
        self.dimension_transaction_open = False
        self.tool_previous_selection_enabled = None
        self.thicken_update_timer = QtCore.QTimer()
        self.thicken_update_timer.setSingleShot(True)
        self.thicken_update_timer.setInterval(300)
        self.thicken_update_timer.timeout.connect(self._apply_thicken_preview)
        self.symmetry_pairs = []
        self.symmetry_plane_points = []
        self.symmetry_center = 0.0
        self.form = self._create_panel()

    def _create_panel(self):
        panel = bind_edit_panel(self)
        panel.setWindowIcon(
            QtGui.QIcon(App.getResourceDir() + "Mod/Forms/Resources/icons/Forms_Workbench.svg")
        )
        self.parameter_kinds = {}
        for property_name, _label, kind in PRIMITIVE_PARAMETERS[str(self.obj.FormType)]:
            value = getattr(self.obj, property_name)
            widget = self.parameter_widgets[property_name]
            widget.setValue(value.Value if kind == "length" else int(value))
            widget.valueChanged.connect(
                lambda new_value, name=property_name: self._queue_primitive_parameter_change(
                    name, new_value
                )
            )
            self.parameter_kinds[property_name] = kind
        self._set_parametric_state(self.obj.CageMode == "Parametric")
        self._populate_pipe_segment_table()

        self.symmetric.setChecked(bool(getattr(self.obj, "Symmetric", False)))
        self.symmetric.toggled.connect(self._symmetry_changed)

        for index, plane in enumerate(("XY", "XZ", "YZ")):
            self.symmetry_plane.setItemData(index, plane)
        current_plane = str(getattr(self.obj, "SymmetryPlane", "YZ"))
        self.symmetry_plane.setCurrentIndex(self.symmetry_plane.findData(current_plane))
        self.symmetry_plane.setEnabled(self.symmetric.isChecked())
        self.symmetry_plane.currentIndexChanged.connect(self._symmetry_plane_changed)

        is_surface = str(self.obj.FormType) == "Forms::Surface"
        self.surface_tangent.setVisible(is_surface)
        if is_surface:
            self.surface_tangent.setChecked(str(self.obj.Continuity) == "Tangent")
            self.surface_tangent.toggled.connect(self._surface_continuity_changed)

        for index, value in enumerate(("Point", "Edge", "Face", "All")):
            self.selection_filter.setItemData(index, value)
        self.selection_filter.setCurrentIndex(self.selection_filter.findData("All"))
        self.selection_filter.currentIndexChanged.connect(self._selection_filter_changed)
        self.set_pivot_button.setIcon(self._command_icon("Forms_SetPivot"))
        self.set_pivot_button.setEnabled(False)
        self.set_pivot_button.clicked.connect(self.start_set_pivot_tool)
        self.sharpness_slider.valueChanged.connect(self._queue_selected_sharpness)
        self.sharpness_slider.sliderPressed.connect(self._sharpness_slider_pressed)
        self.sharpness_slider.sliderReleased.connect(self._sharpness_slider_released)
        self.sharpness_spin.valueChanged.connect(self._queue_selected_sharpness)

        for index, value in enumerate(("Global", "View", "Selection")):
            self.coordinate_space.setItemData(index, value)
        self.coordinate_space.currentIndexChanged.connect(self._coordinate_space_changed)
        self.tool_handler_panel = bind_tool_panel(self)
        return [panel, self.tool_handler_panel]

    def _set_parametric_state(self, parametric):
        structured = parametric or self._has_structured_topology()
        for name, widget in self.parameter_widgets.items():
            is_segment = self.parameter_kinds[name] == "integer"
            widget.setVisible(True)
            label = self.parameter_labels.get(name)
            if label is not None:
                label.setVisible(True)
                label.setEnabled(not is_segment or structured)
            widget.setEnabled(
                parametric
                if str(self.obj.FormType) == "Forms::Pipe"
                else (not is_segment or structured)
            )
            if is_segment and not parametric:
                widget.setMinimum(int(getattr(self.obj, name)))
        if self.segment_header is not None:
            self.segment_header.setVisible(True)
            self.segment_header.setEnabled(structured)
        if self.pipe_segment_table is not None:
            self.pipe_segment_table.setEnabled(parametric)

    def _populate_pipe_segment_table(self):
        table = getattr(self, "pipe_segment_table", None)
        if table is None:
            return
        from .pipe import decode_segment_overrides, decode_segment_sample_overrides
        diameter_overrides = decode_segment_overrides(self.obj.SegmentDiameters)
        sample_overrides = decode_segment_sample_overrides(self.obj.SegmentSamples)
        keys = list(self.obj.PipeSegmentKeys)
        descriptions = list(self.obj.PipeSegments)
        table.setRowCount(len(keys))
        for row, key in enumerate(keys):
            item = QtWidgets.QTableWidgetItem(
                descriptions[row] if row < len(descriptions) else str(key)
            )
            item.setFlags(item.flags() & ~QtCore.Qt.ItemIsEditable)
            table.setItem(row, 0, item)
            diameter_spin = QtWidgets.QDoubleSpinBox(table)
            diameter_spin.setRange(0.0, 1000000.0)
            diameter_spin.setSpecialValueText(App.Qt.translate("Forms_Pipe", "Global"))
            diameter_spin.setValue(diameter_overrides.get(str(key), 0.0))
            diameter_spin.editingFinished.connect(
                lambda segment=str(key), editor=diameter_spin: self._pipe_diameter_changed(
                    segment, editor
                )
            )
            table.setCellWidget(row, 1, diameter_spin)
            sample_spin = QtWidgets.QSpinBox(table)
            sample_spin.setRange(0, 100)
            sample_spin.setSpecialValueText(App.Qt.translate("Forms_Pipe", "Global"))
            sample_spin.setValue(sample_overrides.get(str(key), 0))
            sample_spin.editingFinished.connect(
                lambda segment=str(key), editor=sample_spin: self._pipe_samples_changed(
                    segment, editor
                )
            )
            table.setCellWidget(row, 2, sample_spin)

    def _pipe_diameter_changed(self, key, spin):
        if self.cleaned:
            return
        from .pipe import set_segment_diameter
        transaction = self._begin_action(App.Qt.translate("Forms_Pipe", "Change segment diameter"))
        try:
            set_segment_diameter(self.obj, key, spin.value())
            self.obj.Document.recompute()
        except MODELING_ERRORS as error:
            self._finish_action(transaction, commit=False)
            report_modeling_error(App.Qt.translate("Forms_Pipe", "Change diameter"), error)
            return
        except Exception:
            self._finish_action(transaction, commit=False)
            raise
        self._finish_action(transaction)

    def _pipe_samples_changed(self, key, spin):
        if self.cleaned:
            return
        from .pipe import set_segment_samples
        transaction = self._begin_action(
            App.Qt.translate("Forms_Pipe", "Change segment samples")
        )
        try:
            set_segment_samples(self.obj, key, spin.value())
            self.obj.Document.recompute()
        except MODELING_ERRORS as error:
            self._finish_action(transaction, commit=False)
            report_modeling_error(App.Qt.translate("Forms_Pipe", "Change samples"), error)
            return
        except Exception:
            self._finish_action(transaction, commit=False)
            raise
        self._finish_action(transaction)

    def _has_structured_topology(self):
        """Return whether primitive segment controls still describe this cage."""
        if str(getattr(self.obj, "TMeshData", "") or ""):
            return False
        form_type = str(self.obj.FormType)
        if form_type == "Forms::Box":
            _points, faces = box_control_cage(
                self.obj.Length.Value,
                self.obj.Width.Value,
                self.obj.Height.Value,
                self.obj.XSegments,
                self.obj.YSegments,
                self.obj.ZSegments,
            )
        elif form_type == "Forms::Cylinder":
            _points, faces = cylinder_control_cage(
                self.obj.Radius.Value,
                self.obj.Height.Value,
                self.obj.SideSegments,
                self.obj.HeightSegments,
            )
        elif form_type == "Forms::Quadball":
            _points, faces = quadball_control_cage(self.obj.Radius.Value, self.obj.Segments)
        elif form_type == "Forms::Face":
            _points, faces = face_control_cage(
                self.obj.Length.Value,
                self.obj.Width.Value,
                self.obj.XSegments,
                self.obj.YSegments,
            )
        elif form_type == "Forms::Surface":
            u_segments = int(self.obj.USegments)
            v_segments = int(self.obj.VSegments)
            row = u_segments + 1
            faces = [
                (
                    v_index * row + u_index,
                    v_index * row + u_index + 1,
                    (v_index + 1) * row + u_index + 1,
                    (v_index + 1) * row + u_index,
                )
                for v_index in range(v_segments)
                for u_index in range(u_segments)
            ]
        else:
            return False
        current = [tuple(int(index) for index in face.split()) for face in self.obj.ControlFaces]
        return current == faces

    def topology_changed(self):
        """Refresh task controls after an external topology command."""
        if self.cleaned:
            return
        self._set_parametric_state(False)
        self._sync_dimension_properties()
        self._update_dimension_gizmos()
        self._populate_pipe_segment_table()

    def has_active_tool(self):
        """Return whether a temporary interaction handler owns the edit session."""
        return self.active_tool is not None

    @property
    def insert_tool_active(self):
        return self.active_tool == "insert_edge"

    @property
    def subdivide_tool_active(self):
        return self.active_tool == "subdivide"

    @property
    def insert_point_tool_active(self):
        return self.active_tool == "insert_point"

    @property
    def unweld_tool_active(self):
        return self.active_tool == "unweld"

    @property
    def surface_tool_active(self):
        return self.active_tool in ("insert_edge", "insert_point", "subdivide", "unweld")

    @property
    def thicken_tool_active(self):
        return self.active_tool == "thicken"

    @property
    def match_tool_active(self):
        return self.active_tool == "match"

    @property
    def flatten_tool_active(self):
        return self.active_tool == "flatten"

    @property
    def weld_tool_active(self):
        return self.active_tool == "weld"

    @property
    def straighten_tool_active(self):
        return self.active_tool == "straighten"

    @property
    def pivot_tool_active(self):
        return self.active_tool == "set_pivot"

    def _show_tool_handler(self, title, widget, command_name):
        """Replace the content of FreeCAD's secondary tool TaskBox."""
        while self.tool_handler_layout.count():
            item = self.tool_handler_layout.takeAt(0)
            previous = item.widget()
            if previous is not None:
                previous.setParent(None)
        self.tool_handler_widget = widget
        self.tool_handler_panel.setWindowTitle(title)
        icon = self._command_icon(command_name)
        self.tool_handler_panel.setWindowIcon(icon)
        widget.setWindowIcon(icon)
        self.tool_handler_layout.addWidget(widget)
        self.tool_handler_panel.setVisible(True)
        container = self._tool_handler_container()
        if container is not None:
            # The TaskBox copies the child window title when the dialog opens.
            # Updating the child later therefore does not update its header.
            # Its header title is a QSint ActionLabel (a QToolButton).
            for header in container.findChildren(QtWidgets.QToolButton):
                if self._qt_property_text(header, "class") == "header":
                    header.setText(title)
                    header.setIcon(icon)
                    break
            container.setVisible(True)

    @staticmethod
    def _command_icon(command_name):
        command = Gui.Command.get(command_name)
        actions = command.getAction() if command is not None else []
        return actions[0].icon() if actions else QtGui.QIcon()

    @staticmethod
    def _qt_property_text(widget, name):
        value = widget.property(name)
        if isinstance(value, QtCore.QByteArray):
            return bytes(value).decode("utf-8", errors="replace")
        return str(value)

    def _tool_handler_container(self):
        widget = self.tool_handler_panel.parentWidget()
        while widget is not None:
            if "TaskBox" in widget.metaObject().className():
                return widget
            widget = widget.parentWidget()
        return None

    def _hide_tool_handler(self):
        self.tool_handler_panel.setVisible(False)
        container = self._tool_handler_container()
        if container is not None:
            container.setVisible(False)
        if self.tool_handler_widget is not None:
            self.tool_handler_widget.setParent(None)
            self.tool_handler_widget.deleteLater()
            self.tool_handler_widget = None

    def _suspend_selection_for_tool(self, hide_dragger=True, disable_selection=True):
        self.selection_filter.setEnabled(False)
        if hide_dragger and self.dragger_switch is not None:
            self.dragger_switch.whichChild = coin.SO_SWITCH_NONE
        if self.viewer is None or not disable_selection:
            self.tool_previous_selection_enabled = None
            return
        try:
            self.tool_previous_selection_enabled = self.viewer.isSelectionEnabled()
            self.viewer.setSelectionEnabled(False)
        except (AttributeError, RuntimeError):
            self.tool_previous_selection_enabled = None

    def _resume_selection_after_tool(self):
        self.selection_filter.setEnabled(True)
        if self.viewer is not None and self.tool_previous_selection_enabled is not None:
            try:
                self.viewer.setSelectionEnabled(self.tool_previous_selection_enabled)
            except (AttributeError, RuntimeError):
                pass
        self.tool_previous_selection_enabled = None

    def _begin_action(self, label):
        """Open one undo transaction for an action on an existing Form."""
        document = self.obj.Document
        if not self.document_edit or document.getBookedTransactionID() != 0:
            return False
        document.openTransaction(label)
        return True

    def _finish_action(self, opened, commit=True):
        if not opened:
            return
        document = self.obj.Document
        if document.getBookedTransactionID() == 0:
            return
        if commit:
            document.commitTransaction()
        else:
            document.abortTransaction()

    def _queue_primitive_parameter_change(self, property_name, value):
        if self.cleaned:
            return
        self.pending_parameter_changes[property_name] = value
        self.parameter_update_timer.start()

    def _apply_pending_parameter_changes(self):
        if self.cleaned or not self.pending_parameter_changes:
            return
        changes = self.pending_parameter_changes
        self.pending_parameter_changes = {}
        transaction = self._begin_action(App.Qt.translate("Forms_Edit", "Change form parameters"))
        try:
            for property_name, value in changes.items():
                self._apply_primitive_parameter_change(property_name, value)
        except MODELING_ERRORS as error:
            self._finish_action(transaction, commit=False)
            self._sync_dimension_properties()
            report_modeling_error(App.Qt.translate("Forms_Edit", "Change parameters"), error)
            return
        except Exception:
            self._finish_action(transaction, commit=False)
            raise
        self._finish_action(transaction)

    def _apply_primitive_parameter_change(self, property_name, value):
        if self.cleaned:
            return
        if self.obj.CageMode == "Editable":
            if self.parameter_kinds[property_name] == "integer":
                self._increase_segments(property_name, int(value))
            else:
                self._scale_dimension(property_name, float(value))
            return
        setattr(self.obj, property_name, value)
        self.obj.Document.recompute()
        self._clear_editor_selection()
        self._configure_symmetry(apply=bool(self.obj.Symmetric))
        self._update_dimension_gizmos()

    def _dimension_value(self, property_name):
        bounds = self.obj.Shape.BoundBox
        form_type = str(self.obj.FormType)
        radial = [
            (point.x * point.x + point.y * point.y) ** 0.5 for point in self.obj.ControlPoints
        ]
        if property_name == "Length":
            return bounds.XLength
        if property_name == "Width":
            return bounds.YLength
        if property_name == "Height":
            return bounds.ZLength
        if property_name == "Radius":
            if form_type == "Forms::Cylinder":
                return max(bounds.XLength, bounds.YLength) / 2.0
            return max(bounds.XLength, bounds.YLength, bounds.ZLength) / 2.0
        if property_name == "MajorRadius" and radial:
            return (max(radial) + min(radial)) / 2.0
        if property_name == "MinorRadius" and radial:
            return (max(radial) - min(radial)) / 2.0
        if property_name == "OuterRadius" and radial:
            return max(radial)
        if property_name == "InnerRadius" and radial:
            return min(radial)
        raise ValueError(f"Unsupported dimension property: {property_name}")

    def _dimension_axes(self, property_name):
        if property_name == "Length":
            return (0,)
        if property_name == "Width":
            return (1,)
        if property_name == "Height":
            return (2,)
        if property_name == "Radius" and str(self.obj.FormType) == "Forms::Cylinder":
            return (0, 1)
        if property_name == "Radius":
            return (0, 1, 2)
        raise ValueError(f"Unsupported dimension property: {property_name}")

    def _scaled_dimension_points(self, points, property_name, requested, current=None):
        current = self._dimension_value(property_name) if current is None else current
        if current <= 1.0e-9 or requested <= 1.0e-9:
            return [App.Vector(point) for point in points]
        factor = requested / current
        center = App.Vector(self.obj.Shape.BoundBox.Center)
        form_type = str(self.obj.FormType)
        if form_type == "Forms::Torus" and property_name in (
            "MajorRadius",
            "MinorRadius",
        ):
            major = self._dimension_value("MajorRadius")
            minor = self._dimension_value("MinorRadius")
            result = [App.Vector(point) for point in points]
            for point in result:
                radial = (point.x * point.x + point.y * point.y) ** 0.5
                if radial <= 1.0e-12:
                    continue
                direction = App.Vector(point.x / radial, point.y / radial, 0.0)
                if property_name == "MajorRadius":
                    point += direction.multiply(requested - major)
                else:
                    section_radial = radial - major
                    section_z = point.z - center.z
                    section_factor = requested / max(minor, 1.0e-12)
                    target_radial = major + section_radial * section_factor
                    point.x = direction.x * target_radial
                    point.y = direction.y * target_radial
                    point.z = center.z + section_z * section_factor
            return result
        if form_type == "Forms::Tube" and property_name in (
            "OuterRadius",
            "InnerRadius",
        ):
            outer = self._dimension_value("OuterRadius")
            inner = self._dimension_value("InnerRadius")
            threshold = (outer + inner) * 0.5
            target_group = property_name == "OuterRadius"
            result = [App.Vector(point) for point in points]
            for point in result:
                radial = (point.x * point.x + point.y * point.y) ** 0.5
                if radial <= 1.0e-12 or ((radial >= threshold) != target_group):
                    continue
                group_current = outer if target_group else inner
                group_factor = requested / max(group_current, 1.0e-12)
                point.x *= group_factor
                point.y *= group_factor
            return result
        axes = self._dimension_axes(property_name)
        result = [App.Vector(point) for point in points]
        for point in result:
            for axis in axes:
                point[axis] = center[axis] + (point[axis] - center[axis]) * factor
        return result

    def _scale_dimension(self, property_name, requested):
        points = self._scaled_dimension_points(self.obj.ControlPoints, property_name, requested)
        self._clear_editor_selection()
        self._set_control_points(points, recompute=True)
        self._configure_symmetry(apply=bool(self.obj.Symmetric))
        self._sync_dimension_properties()
        self._update_dimension_gizmos()

    def _sync_dimension_properties(self):
        if str(self.obj.FormType) == "Forms::Pipe":
            return
        if self.obj.CageMode != "Editable" or self.obj.Shape.isNull():
            return
        for name, widget in self.parameter_widgets.items():
            if self.parameter_kinds[name] != "length":
                continue
            value = self._dimension_value(name)
            setattr(self.obj, name, value)
            blocker = QtCore.QSignalBlocker(widget)
            widget.setValue(value)
            del blocker

    def _structured_segments(self):
        form_type = str(self.obj.FormType)
        if form_type == "Forms::Box":
            return (int(self.obj.XSegments), int(self.obj.YSegments), int(self.obj.ZSegments))
        if form_type == "Forms::Cylinder":
            side = 2 * int(self.obj.SideSegments)
            return (side, side, int(self.obj.HeightSegments))
        if form_type == "Forms::Quadball":
            segments = int(self.obj.Segments)
            return (segments, segments, segments)
        if form_type == "Forms::Face":
            return (int(self.obj.XSegments), int(self.obj.YSegments))
        if form_type == "Forms::Surface":
            return (int(self.obj.USegments), int(self.obj.VSegments))
        raise ValueError("Unsupported structured Forms primitive")

    def _increased_structured_segments(self, property_name, value):
        current = list(self._structured_segments())
        form_type = str(self.obj.FormType)
        if form_type == "Forms::Box":
            current[{"XSegments": 0, "YSegments": 1, "ZSegments": 2}[property_name]] = value
        elif form_type == "Forms::Cylinder":
            if property_name == "SideSegments":
                current[0] = current[1] = 2 * value
            else:
                current[2] = value
        elif form_type == "Forms::Quadball":
            current = [value, value, value]
        elif form_type in ("Forms::Face", "Forms::Surface"):
            names = (
                {"XSegments": 0, "YSegments": 1}
                if form_type == "Forms::Face"
                else {"USegments": 0, "VSegments": 1}
            )
            current[names[property_name]] = value
        return tuple(current)

    def _increase_segments(self, property_name, value):
        old_property_value = int(getattr(self.obj, property_name))
        if value <= old_property_value:
            widget = self.parameter_widgets[property_name]
            blocker = QtCore.QSignalBlocker(widget)
            widget.setValue(old_property_value)
            del blocker
            return
        old_segments = self._structured_segments()
        new_segments = self._increased_structured_segments(property_name, value)
        points = [(point.x, point.y, point.z) for point in self.obj.ControlPoints]
        vertex_sharpness, edge_sharpness = self._sharpness_data()
        (
            resized_points,
            resized_faces,
            resized_vertex_sharpness,
            resized_edge_sharpness,
        ) = resize_structured_cage(
            points,
            old_segments,
            new_segments,
            surface=str(self.obj.FormType) in ("Forms::Face", "Forms::Surface"),
            vertex_sharpness=vertex_sharpness,
            edge_sharpness=edge_sharpness,
            return_sharpness=True,
        )
        self._clear_editor_selection()
        setattr(self.obj, property_name, value)
        self.parameter_widgets[property_name].setMinimum(value)
        self.obj.ControlPoints = [App.Vector(*point) for point in resized_points]
        self.obj.ControlFaces = [" ".join(str(index) for index in face) for face in resized_faces]
        self.obj.VertexSharpness = resized_vertex_sharpness
        self.obj.EdgeSharpness = [
            f"{edge[0]} {edge[1]} {sharpness:.12g}"
            for edge, sharpness in sorted(resized_edge_sharpness.items())
        ]
        self.obj.Document.recompute()
        self._configure_symmetry(apply=bool(self.obj.Symmetric))
        self._sync_dimension_properties()
        self._update_dimension_gizmos()
        self.view.redraw()

    def start(self):
        global _active_session

        if _active_session is not None and not _active_session.cleaned:
            finish_active_form_session()
        _active_session = self
        try:
            self._start()
        except Exception:
            self.cleanup()
            raise

    def _start(self):
        if self.document_edit:
            # Match Sketcher's cancel semantics without wrapping the whole edit
            # session in one transaction: each modeling action remains
            # independently undoable, while Cancel can restore this baseline.
            self.edit_backup = self.obj.dumpContent(0)
        self._enable_profile_edit_shape()
        self.view_object.DisplayMode = "Flat Lines"
        self._increase_pick_radius()
        self._increase_native_vertex_size()
        self._create_dragger()
        self._create_dimension_gizmos()
        self._configure_symmetry(apply=bool(getattr(self.obj, "Symmetric", False)))
        self.view_object.RootNode.addChild(self.root)
        Gui.Selection.clearSelection()
        Gui.Selection.addObserver(self, Gui.Selection.ResolveMode.NoResolve)
        self.selection_observer_added = True
        App.addDocumentObserver(self)
        self.document_observer_added = True
        self._install_selection_gate()
        self.key_filter = FormKeyFilter(self)
        # The 3D view is a child viewport, so a filter on the main window does
        # not receive its key events. QApplication observes both the view and
        # task widgets and makes M/Escape reliable regardless of focus.
        QtWidgets.QApplication.instance().installEventFilter(self.key_filter)
        self._show_input_hints()
        self.set_selection([])
        self._sync_dimension_properties()
        self._update_dimension_gizmos()
        Gui.Control.showDialog(self)
        set_forms_toolbar_mode(True)
        QtCore.QTimer.singleShot(0, self._hide_tool_handler)
        self.view.redraw()

    def _enable_profile_edit_shape(self):
        """Expose profile-backed Face patches when this session owns the switch.

        Normal document edit enters through ``ViewProviderFormFace.setEdit``
        and has already enabled the patch shape.  Creation tasks construct the
        session directly, so they must enable it here and restore it during
        cleanup.
        """
        if str(self.obj.FormType) != "Forms::Face":
            return
        profile = getattr(self.obj, "ProfileShape", None)
        proxy = getattr(self.obj, "Proxy", None)
        if (
            profile is None
            or profile.isNull()
            or proxy is None
            or not hasattr(proxy, "show_edit_shape")
            or bool(getattr(proxy, "_show_edit_shape", False))
        ):
            return
        proxy.show_edit_shape(self.obj, True)
        self.profile_edit_shape_owned = True

    def slotUndoDocument(self, document):
        if not self.cleaned and document == self.obj.Document:
            QtCore.QTimer.singleShot(0, self._refresh_after_history_change)

    def slotRedoDocument(self, document):
        if not self.cleaned and document == self.obj.Document:
            QtCore.QTimer.singleShot(0, self._refresh_after_history_change)

    def _refresh_after_history_change(self):
        """Resynchronize the editor after an undo or redo made in edit mode."""
        if self.cleaned:
            return
        self._clear_editor_selection()
        for name, widget in self.parameter_widgets.items():
            value = getattr(self.obj, name)
            value = value.Value if self.parameter_kinds[name] == "length" else int(value)
            blocker = QtCore.QSignalBlocker(widget)
            if self.parameter_kinds[name] != "length":
                # Segment increases raise the lower bound while editing. Undo
                # must be able to restore the previous, smaller value first.
                widget.setMinimum(1)
            widget.setValue(value)
            del blocker
        symmetric = bool(getattr(self.obj, "Symmetric", False))
        blocker = QtCore.QSignalBlocker(self.symmetric)
        self.symmetric.setChecked(symmetric)
        del blocker
        plane = str(getattr(self.obj, "SymmetryPlane", "YZ"))
        blocker = QtCore.QSignalBlocker(self.symmetry_plane)
        self.symmetry_plane.setCurrentIndex(self.symmetry_plane.findData(plane))
        del blocker
        self.symmetry_plane.setEnabled(symmetric)
        if (
            self.surface_tangent is not None
            and str(self.obj.FormType) == "Forms::Surface"
            and hasattr(self.obj, "Continuity")
        ):
            blocker = QtCore.QSignalBlocker(self.surface_tangent)
            self.surface_tangent.setChecked(str(self.obj.Continuity) == "Tangent")
            del blocker
        self._reset_surface_tool_cache()
        if self.insert_point_tool_active:
            self.insert_point_chain = []
            self.insert_point_hover = None
            self._clear_surface_preview()
        self._set_parametric_state(self.obj.CageMode == "Parametric")
        self._configure_symmetry(apply=False)
        self._sync_dimension_properties()
        self._update_dimension_gizmos()
        self.view.redraw()
        Gui.Command.update()

    def _increase_pick_radius(self):
        self.viewer = self.view.getViewer()
        self.previous_pick_radius = self.viewer.getPickRadius()
        requested = float(getattr(self.view_object, "SelectionPickRadius", 12))
        self.viewer.setPickRadius(max(self.previous_pick_radius, requested))

    def _increase_native_vertex_size(self):
        if "PointSize" not in self.view_object.PropertiesList:
            return
        self.previous_point_size = float(self.view_object.PointSize)
        self.view_object.PointSize = max(self.previous_point_size, 8.0)

    def _create_dragger(self):
        node_type = coin.SoType.fromName("SoTransformDragger")
        if node_type.isBad():
            raise RuntimeError("FreeCAD's transform dragger is unavailable")
        self.dragger = node_type.createInstance()
        if self.dragger is None:
            raise RuntimeError("Could not create FreeCAD's transform dragger")
        self.dragger.draggerSize.setValue(0.03)
        self.dragger.planarScaleVisible.setValue(True)
        self.dragger_switch = coin.SoSwitch()
        self.dragger_switch.addChild(self.dragger)
        self.root.addChild(self.dragger_switch)
        self._setup_dragger_autoscale()
        self.dragger_callbacks = [
            (
                "addStartCallback",
                self.view.addDraggerCallback(
                    self.dragger, "addStartCallback", self.dragger_started
                ),
            ),
            (
                "addMotionCallback",
                self.view.addDraggerCallback(self.dragger, "addMotionCallback", self.dragger_moved),
            ),
            (
                "addFinishCallback",
                self.view.addDraggerCallback(
                    self.dragger, "addFinishCallback", self.dragger_finished
                ),
            ),
        ]

    def _dimension_gizmo_names(self):
        if str(self.obj.FormType) == "Forms::Pipe":
            return []
        return [name for name, kind in self.parameter_kinds.items() if kind == "length"]

    def _dimension_gizmo_placement(self, name):
        form_shape = getattr(self.obj, "FormShape", None)
        shape = form_shape if form_shape is not None and not form_shape.isNull() else self.obj.Shape
        bounds = shape.BoundBox
        center = App.Vector(bounds.Center)
        rotation = (
            self.obj.FormPlacement.Rotation
            if self.obj.CageMode == "Parametric" and "FormPlacement" in self.obj.PropertiesList
            else App.Rotation()
        )
        if name == "Length":
            direction = App.Vector(1, 0, 0)
        elif name == "Width":
            direction = App.Vector(0, 1, 0)
        elif name == "Height":
            direction = App.Vector(0, 0, 1)
        else:
            direction = App.Vector(1, 1, 0)
        return center, rotation.multVec(direction)

    def _create_dimension_gizmos(self):
        node_type = coin.SoType.fromName("SoLinearDraggerContainer")
        if node_type.isBad():
            return
        for name in self._dimension_gizmo_names():
            container = node_type.createInstance()
            dragger = container.getPart("dragger", True)
            dragger.labelVisible = False
            dragger.color = (1.0, 0.0, 0.0)
            dragger.activeColor = (1.0, 0.7, 0.0)
            arrow = dragger.getPart("arrow", True)
            arrow.cylinderHeight = 3.5
            arrow.cylinderRadius = 0.2
            container.color = (1.0, 0.0, 0.0)
            switch = coin.SoSwitch()
            switch.addChild(container)
            self.root.addChild(switch)
            self.dimension_gizmo_switches[name] = switch
            callbacks = []
            for callback_type, callback in (
                (
                    "addStartCallback",
                    lambda node, dimension=name: self._dimension_drag_started(dimension, node),
                ),
                (
                    "addMotionCallback",
                    lambda node, dimension=name: self._dimension_drag_moved(dimension, node),
                ),
                (
                    "addFinishCallback",
                    lambda node, dimension=name: self._dimension_drag_finished(dimension, node),
                ),
            ):
                token = self.view.addDraggerCallback(dragger, callback_type, callback)
                callbacks.append((callback_type, token))
                self.dimension_gizmo_callbacks.append((dragger, callback_type, token))
            self.dimension_gizmos[name] = (container, dragger)

    def _set_dimension_gizmos_visible(self, visible):
        child = coin.SO_SWITCH_ALL if visible else coin.SO_SWITCH_NONE
        for switch in self.dimension_gizmo_switches.values():
            switch.whichChild = child

    def _gizmo_dimension_value(self, name):
        if self.obj.CageMode == "Parametric":
            return float(getattr(self.obj, name).Value)
        return self._dimension_value(name)

    def _update_dimension_gizmos(self):
        for name, (container, dragger) in self.dimension_gizmos.items():
            position, direction = self._dimension_gizmo_placement(name)
            self._set_dimension_gizmo_frame(container, position, direction)
            divisor = 1.0 if name == "Radius" else 2.0
            dragger.translation = (
                0.0,
                self._gizmo_dimension_value(name) / divisor,
                0.0,
            )
        self._set_dimension_gizmos_visible(not self.selected)
        self._update_dragger_scale()

    @staticmethod
    def _set_dimension_gizmo_frame(container, position, direction):
        direction = App.Vector(direction)
        direction.normalize()
        container.translation = (position.x, position.y, position.z)
        container.rotation = coin.SbRotation(
            coin.SbVec3f(0.0, 1.0, 0.0),
            coin.SbVec3f(direction.x, direction.y, direction.z),
        )

    def _transform_dimension_gizmos(self, translation, rotation):
        for name, (container, _dragger) in self.dimension_gizmos.items():
            frame = self.base_dimension_gizmo_frames.get(name)
            if frame is None:
                continue
            position, direction = frame
            transformed_position = translation.add(rotation.multVec(position.sub(self.base_center)))
            self._set_dimension_gizmo_frame(
                container,
                transformed_position,
                rotation.multVec(direction),
            )
        self._update_dragger_scale()

    def _dimension_drag_started(self, name, _dragger):
        self.dimension_transaction_open = self._begin_action(
            App.Qt.translate("Forms_Edit", "Resize form")
        )
        self.dimension_base_points = [App.Vector(point) for point in self.obj.ControlPoints]
        self.dimension_base_value = self._gizmo_dimension_value(name)

    def _dimension_drag_value(self, name, dragger):
        arrow_length = float(dragger.translation.getValue()[1])
        multiplier = 1.0 if name == "Radius" else 2.0
        return max(
            self.dimension_base_value
            + multiplier * (arrow_length - self.dimension_base_value / multiplier),
            0.01,
        )

    def _dimension_drag_moved(self, name, dragger):
        requested = self._dimension_drag_value(name, dragger)
        widget = self.parameter_widgets[name]
        blocker = QtCore.QSignalBlocker(widget)
        widget.setValue(requested)
        del blocker
        if self.obj.CageMode == "Editable":
            points = self._scaled_dimension_points(
                self.dimension_base_points,
                name,
                requested,
                self.dimension_base_value,
            )
            self._set_control_points(points)

    def _dimension_drag_finished(self, name, dragger):
        try:
            requested = self._dimension_drag_value(name, dragger)
            if self.obj.CageMode == "Parametric":
                setattr(self.obj, name, requested)
            else:
                self._dimension_drag_moved(name, dragger)
            self.obj.Document.recompute()
            self._configure_symmetry(apply=bool(self.obj.Symmetric))
            self._sync_dimension_properties()
            self._update_dimension_gizmos()
        except MODELING_ERRORS as error:
            self._finish_action(self.dimension_transaction_open, commit=False)
            self.dimension_transaction_open = False
            self._sync_dimension_properties()
            self._update_dimension_gizmos()
            report_modeling_error(App.Qt.translate("Forms_Edit", "Change dimension"), error)
            return
        except Exception:
            self._finish_action(self.dimension_transaction_open, commit=False)
            self.dimension_transaction_open = False
            raise
        self._finish_action(self.dimension_transaction_open)
        self.dimension_transaction_open = False

    def _setup_dragger_autoscale(self):
        """Keep the transform dragger at a fixed size on screen."""
        self.camera = self.view.getCameraNode()
        self.dragger_scale_node = self.dragger.getPart("scaleNode", True)
        self.dragger_scale_node.scaleFactor.disconnect()
        self.dragger.autoScaleResult.disconnect()
        self.camera_sensor = coin.SoFieldSensor(self._camera_changed, None)
        if self.camera.getTypeId().isDerivedFrom(coin.SoOrthographicCamera.getClassTypeId()):
            self.camera_sensor.attach(self.camera.height)
        else:
            self.camera_sensor.attach(self.camera.position)
        self.camera_orientation_sensor = coin.SoFieldSensor(self._camera_orientation_changed, None)
        self.camera_orientation_sensor.attach(self.camera.orientation)
        self._update_dragger_scale()

    def _camera_changed(self, _data, _sensor):
        self._update_dragger_scale()
        if (
            self.coordinate_space.currentData() == "View"
            and self.selected
            and not QtWidgets.QApplication.mouseButtons()
        ):
            self._reset_dragger_frame()

    def _camera_orientation_changed(self, _data, _sensor):
        if (
            self.coordinate_space.currentData() == "View"
            and self.selected
            and not QtWidgets.QApplication.mouseButtons()
        ):
            self._reset_dragger_frame()

    def _update_dragger_scale(self):
        if self.dragger is None or self.camera is None or self.dragger_scale_node is None:
            return
        origin = self.dragger.translation.getValue()
        radius = self.dragger.draggerSize.getValue() / 2.0
        scale = self.camera.getViewVolume().getWorldToScreenScale(origin, radius)
        self.dragger_scale_node.scaleFactor.setValue(scale, scale, scale)
        self.dragger.autoScaleResult.setValue(scale)
        for container, linear_dragger in self.dimension_gizmos.values():
            position = container.translation.getValue()
            linear_scale = self.camera.getViewVolume().getWorldToScreenScale(position, 0.02)
            linear_dragger.geometryScale = (linear_scale, linear_scale, linear_scale)
            linear_dragger.autoScaleResult = linear_scale

    def _cage_diagonal(self):
        points = self.obj.ControlPoints
        if not points:
            return 1.0
        xs = [point.x for point in points]
        ys = [point.y for point in points]
        zs = [point.z for point in points]
        return App.Vector(max(xs) - min(xs), max(ys) - min(ys), max(zs) - min(zs)).Length

    def _install_selection_gate(self):
        if self.selection_gate_added:
            Gui.Selection.removeSelectionGate()
            self.selection_gate_added = False
            self.selection_gate = None
        # "All" is the normal FreeCAD element-selection behavior. Installing
        # a Python gate here adds a Python round-trip to every hover/preselect
        # pick without filtering anything.
        if self.selection_filter.currentData() == "All":
            return
        self.selection_gate = FormSelectionGate(self)
        Gui.Selection.addSelectionGate(self.selection_gate, Gui.Selection.ResolveMode.NoResolve)
        self.selection_gate_added = True

    def _show_input_hints(self):
        if self.has_active_tool():
            return
        target, payload = self._alt_drag_target()
        if target is None:
            Gui.HintManager.hide()
            return
        if target == "faces":
            if len(payload) == 1:
                text = App.Qt.translate(
                    "Forms_Edit", "%1+%2 drag the selected face to add geometry"
                )
            else:
                text = App.Qt.translate(
                    "Forms_Edit", "%1+%2 drag the selected faces to add geometry"
                )
        elif len(payload) == 1:
            text = App.Qt.translate(
                "Forms_Edit", "%1+%2 drag the selected boundary edge to add a face"
            )
        else:
            text = App.Qt.translate(
                "Forms_Edit",
                "%1+%2 drag the selected boundary edges to add faces",
            )
        Gui.HintManager.show(
            Gui.InputHint(
                text,
                Gui.UserInput.KeyAlt,
                Gui.UserInput.MouseLeft,
            ),
        )

    def _alt_drag_target(self):
        """Return the supported Alt-drag operation represented by the selection."""
        if (
            self.cleaned
            or self.symmetric.isChecked()
            or getattr(self.obj, "LocalEdgeInserts", ())
            or getattr(self.obj, "DissolvedEdges", ())
            or str(getattr(self.obj, "TMeshData", "") or "")
        ):
            return None, None
        targets = self._selected_control_targets(respect_symmetry=False)
        try:
            cage = ControlCage.from_object(self.obj)
        except ValueError:
            return None, None
        if targets and all(kind == "Face" for kind, _indices, _anchor in targets):
            face_indices = {cage.face_index(indices) for _kind, indices, _anchor in targets}
            if None not in face_indices and cage.can_extrude_faces(face_indices):
                return "faces", face_indices
        if not targets or not all(
            kind == "Edge" and len(indices) == 2 for kind, indices, _anchor in targets
        ):
            return None, None
        edges = {tuple(sorted(indices)) for _kind, indices, _anchor in targets}
        if edges.issubset(set(cage.boundary_edges)):
            return "boundary_edges", edges
        return None, None

    def _selection_filter_changed(self, _index):
        if self.cleaned:
            return
        self.suppress_selection_observer = True
        try:
            Gui.Selection.clearSelection()
        finally:
            self.suppress_selection_observer = False
        self.set_selection([])
        self._install_selection_gate()
        self._show_input_hints()

    def _coordinate_space_changed(self, _index):
        if self.dragger is not None and self.selected:
            self._reset_dragger_frame()

    def _symmetry_axis(self):
        return {"XY": 2, "XZ": 1, "YZ": 0}[self.symmetry_plane.currentData()]

    @staticmethod
    def _reflected(point, axis, center):
        return App.Vector(*reflected((point.x, point.y, point.z), axis, center))

    def _configure_symmetry(self, apply=False):
        points = self._all_control_points()
        self.symmetry_pairs = []
        self.symmetry_plane_points = []
        # An asymmetric form is perfectly valid while symmetry is disabled.
        # Trying to build strict reflected pairs unconditionally prevented any
        # edited/asymmetric form from entering edit mode.
        if not points or not self.symmetric.isChecked():
            return
        axis = self._symmetry_axis()
        self.symmetry_center = 0.0
        vertices = {index: (point.x, point.y, point.z) for index, point in enumerate(points)}
        self.symmetry_pairs, self.symmetry_plane_points = control_pairs(
            vertices, axis, self.symmetry_center, strict=False
        )
        if apply:
            self._enforce_symmetry(points, set(range(len(points))))
            self._set_control_points(points, recompute=True)

    def _enforce_symmetry(self, points, moved):
        if not self.symmetric.isChecked():
            return
        axis = self._symmetry_axis()
        for positive, negative in self.symmetry_pairs:
            if positive in moved:
                points[negative] = self._reflected(points[positive], axis, self.symmetry_center)
            elif negative in moved:
                points[positive] = self._reflected(points[negative], axis, self.symmetry_center)
        for index in self.symmetry_plane_points:
            points[index][axis] = self.symmetry_center

    def _set_control_points(self, points, recompute=False):
        base_count = len(self.obj.ControlPoints)
        self.obj.ControlPoints = points[:base_count]
        if "LocalControlPoints" in self.obj.PropertiesList:
            self.obj.LocalControlPoints = points[base_count:]
        if recompute:
            # Recompute only the Form feature. In a Part Design Body this avoids
            # evaluating downstream features while the user is dragging.
            self.obj.recompute()
        self.view.redraw()

    def _all_control_points(self):
        """Return base cage and hierarchical local controls in index order."""
        return [App.Vector(point) for point in self.obj.ControlPoints] + [
            App.Vector(point) for point in getattr(self.obj, "LocalControlPoints", ())
        ]

    def _symmetry_changed(self, enabled):
        if not hasattr(self.obj, "Symmetric"):
            return
        transaction = self._begin_action(App.Qt.translate("Forms_Edit", "Change form symmetry"))
        try:
            self.obj.Symmetric = bool(enabled)
            self.symmetry_plane.setEnabled(bool(enabled))
            self._configure_symmetry(apply=bool(enabled))
            self._clear_editor_selection()
            self._sync_dimension_properties()
            self._update_dimension_gizmos()
        except MODELING_ERRORS as error:
            self._finish_action(transaction, commit=False)
            self._sync_dimension_properties()
            self._update_dimension_gizmos()
            report_modeling_error(App.Qt.translate("Forms_Edit", "Change symmetry"), error)
            return
        except Exception:
            self._finish_action(transaction, commit=False)
            raise
        self._finish_action(transaction)

    def _surface_continuity_changed(self, tangent):
        if self.cleaned or str(self.obj.FormType) != "Forms::Surface":
            return
        transaction = self._begin_action(
            App.Qt.translate("Forms_Edit", "Change form surface continuity")
        )
        try:
            self.obj.Continuity = "Tangent" if tangent else "Connected"
            self.obj.Document.recompute()
            self._clear_editor_selection()
            self.view.redraw()
        except MODELING_ERRORS as error:
            self._finish_action(transaction, commit=False)
            report_modeling_error(App.Qt.translate("Forms_Edit", "Change continuity"), error)
            return
        except Exception:
            self._finish_action(transaction, commit=False)
            raise
        self._finish_action(transaction)

    def _symmetry_plane_changed(self, _index):
        if not hasattr(self.obj, "SymmetryPlane"):
            return
        transaction = self._begin_action(App.Qt.translate("Forms_Edit", "Change symmetry plane"))
        try:
            self.obj.SymmetryPlane = self.symmetry_plane.currentData()
            self._configure_symmetry(apply=self.symmetric.isChecked())
            self._clear_editor_selection()
            self._sync_dimension_properties()
            self._update_dimension_gizmos()
        except MODELING_ERRORS as error:
            self._finish_action(transaction, commit=False)
            self._sync_dimension_properties()
            self._update_dimension_gizmos()
            report_modeling_error(App.Qt.translate("Forms_Edit", "Change symmetry plane"), error)
            return
        except Exception:
            self._finish_action(transaction, commit=False)
            raise
        self._finish_action(transaction)

    def _clear_editor_selection(self, clear_preselection=False):
        self.suppress_selection_observer = True
        try:
            Gui.Selection.clearSelection()
            if clear_preselection:
                Gui.Selection.clearPreselection()
        finally:
            self.suppress_selection_observer = False
        self.set_selection([])

    def _form_selection_subelement(self, document, object_name, subelement):
        """Resolve direct and Body-routed selection events to this Form."""
        raw = str(subelement or "")
        if not raw:
            return None

        marker = f"{self.obj.Name}."
        marker_index = raw.rfind(marker)
        if marker_index >= 0:
            return raw[marker_index + len(marker) :]

        if object_name == self.obj.Name:
            return raw

        try:
            doc = document if hasattr(document, "getObject") else App.getDocument(str(document))
            event_object = doc.getObject(str(object_name))
            body = self.obj.getParentGeoFeatureGroup()
        except (AttributeError, NameError, RuntimeError):
            return None
        if event_object is not body:
            return None

        # With the preceding feature intentionally visible during Forms edit,
        # Body-routed selections can identify that sibling explicitly. Do not
        # reinterpret e.g. Pad.<mapped Face3> as a face of the edited Form.
        for feature in getattr(body, "Group", ()):
            if feature is self.obj:
                continue
            if raw.startswith(f"{feature.Name}."):
                return None

        # A Body may report either FormName.EdgeN or just EdgeN while its tip
        # is being edited. Both identify the visible tip shape in that case.
        return raw if canonical_subelement_name(raw) else None

    def _range_selection_target(self, subelement):
        """Return ``(kind, stable ID, mapper)`` for one clicked Form element."""
        try:
            element = self.obj.Shape.getElement(subelement)
            mapper = self._control_element_mapper()
            indices = mapper.indices(element)
        except (Part.OCCError, RuntimeError, ValueError, IndexError):
            return None
        kind = element.ShapeType
        if kind == "Vertex" and len(indices) == 1:
            return kind, indices[0], mapper
        if kind == "Edge" and len(indices) == 2:
            return kind, tuple(sorted(indices)), mapper
        if kind == "Face":
            face_id = mapper.face_id(indices)
            if face_id is not None:
                return kind, face_id, mapper
        return None

    @staticmethod
    def _range_topology(mapper):
        """Return ordered logical faces and stable face IDs for range selection."""
        if mapper.mesh is None:
            return list(mapper.logical_faces), list(range(len(mapper.logical_faces)))
        face_ids = sorted(mapper.mesh.faces)
        return [mapper.mesh.faces[face_id].boundary for face_id in face_ids], face_ids

    def _extend_shift_range(self, subelement):
        """Replace the clicked pair with its Excel-style topological range."""
        target = self._range_selection_target(subelement)
        if target is None:
            return False
        kind, clicked, mapper = target
        anchor = self.range_selection_anchors.get(kind)
        if anchor is None:
            self.range_selection_anchors[kind] = clicked
            return False

        faces, face_ids = self._range_topology(mapper)
        try:
            if kind == "Vertex":
                selected_range = cage_vertex_selection_range(faces, anchor, clicked)
            elif kind == "Edge":
                selected_range = cage_edge_selection_range(faces, anchor, clicked)
            else:
                dense = {face_id: index for index, face_id in enumerate(face_ids)}
                selected_range = {
                    face_ids[index]
                    for index in cage_face_selection_range(
                        faces, dense[anchor], dense[clicked]
                    )
                }
        except (KeyError, ValueError):
            return False

        vertices = set()
        edges = set()
        selected_faces = set()
        for selected_kind, indices, _anchor in self._selected_control_targets(
            respect_symmetry=False
        ):
            if selected_kind == "Vertex" and len(indices) == 1:
                vertices.add(indices[0])
            elif selected_kind == "Edge" and len(indices) == 2:
                edges.add(tuple(sorted(indices)))
            elif selected_kind == "Face":
                selected_faces.add(frozenset(indices))

        if kind == "Vertex":
            vertices.update(selected_range)
        elif kind == "Edge":
            edges.update(selected_range)
        else:
            by_id = dict(zip(face_ids, faces))
            selected_faces.update(frozenset(by_id[face_id]) for face_id in selected_range)
        self.last_added_edge = None
        # Selection observers run while FreeCAD is still publishing the click
        # that triggered them. Clearing and rebuilding the selection from this
        # callback can re-enter the selection machinery and deadlock the GUI.
        # Apply the completed range after the current selection event returns.
        self.range_selection_generation += 1
        generation = self.range_selection_generation
        self._schedule_shift_range(vertices, edges, selected_faces, generation)
        return True

    def _schedule_shift_range(self, vertices, edges, faces, generation):
        """Apply a completed range on the next Qt event-loop turn."""
        self._defer_shift_range(
            lambda: self._apply_shift_range(
                vertices,
                edges,
                faces,
                generation,
            ),
        )

    @staticmethod
    def _defer_shift_range(callback):
        QtCore.QTimer.singleShot(0, callback)

    def _apply_shift_range(self, vertices, edges, faces, generation):
        """Replace a Shift-click selection outside the observer callback."""
        if self.cleaned or generation != self.range_selection_generation:
            return
        self._restore_control_selection(vertices, edges, faces, defer_dragger=True)

    def addSelection(self, document, object_name, subelement, _position):
        if self.suppress_selection_observer or self.cleaned:
            return
        if self.pivot_tool_active:
            self._pivot_selection_added(_position)
            return
        self._arm_dragger_reveal_deadline()
        if self.straighten_tool_active and self.straighten_reference_selecting:
            self._add_straighten_reference(document, object_name, subelement)
            return
        if self.flatten_tool_active and self.flatten_reference_selecting:
            self._set_flatten_reference_from_selection(document, object_name, subelement)
            return
        if self.weld_tool_active:
            self._set_weld_input_from_selection(document, object_name, subelement)
            return
        form_subelement = self._form_selection_subelement(document, object_name, subelement)
        canonical = canonical_subelement_name(form_subelement)
        modifiers = QtWidgets.QApplication.keyboardModifiers()
        if (
            form_subelement is not None
            and modifiers & QtCore.Qt.ShiftModifier
            and self._extend_shift_range(form_subelement)
        ):
            return
        if form_subelement is not None and not modifiers & QtCore.Qt.ShiftModifier:
            self.range_selection_generation += 1
            target = self._range_selection_target(form_subelement)
            if target is not None:
                kind, stable_id, _mapper = target
                self.range_selection_anchors[kind] = stable_id
        if form_subelement is not None and canonical.startswith("Edge"):
            self.last_added_edge = (
                document,
                self.obj.Name,
                form_subelement,
                canonical,
                time.monotonic(),
            )
        else:
            # The double-click candidate is valid only while that edge is the
            # element being clicked. Retaining it across a later face/vertex
            # selection can make a normal clear reconstruct an unrelated wire,
            # especially around an open boundary.
            self.last_added_edge = None
        self._queue_selection_sync()

    def removeSelection(self, document, object_name, subelement):
        if self.suppress_selection_observer or self.cleaned:
            return
        if self.pivot_tool_active:
            return
        form_subelement = self._form_selection_subelement(document, object_name, subelement)
        canonical = canonical_subelement_name(form_subelement)
        previous = self.last_added_edge
        self.last_added_edge = None
        if previous is not None:
            (
                previous_document,
                previous_object,
                previous_subelement,
                previous_edge,
                added_at,
            ) = previous
            interval = QtWidgets.QApplication.instance().doubleClickInterval() / 1000.0
            elapsed = time.monotonic() - added_at
            matches = (
                document == previous_document
                and form_subelement is not None
                and previous_object == self.obj.Name
                and canonical == previous_edge
                and elapsed <= interval
            )
            if matches:
                self.selection_sync_generation += 1
                if self.dragger_switch is not None:
                    self.dragger_switch.whichChild = coin.SO_SWITCH_NONE
                QtCore.QTimer.singleShot(
                    0,
                    lambda edge=previous_subelement: self._select_edge_loop(edge),
                )
                return
        self._queue_selection_sync()

    def setSelection(self, document):
        if self.pivot_tool_active:
            return
        self.last_added_edge = None
        if not QtWidgets.QApplication.keyboardModifiers() & QtCore.Qt.ShiftModifier:
            self.range_selection_generation += 1
            self.range_selection_anchors = {}
        self._queue_selection_sync()

    def setPreselection(self, document, object_name, subelement):
        """Refresh edge-driven tools directly from native preselection."""
        if self.cleaned or not (self.insert_point_tool_active or self.unweld_tool_active):
            return
        if self.insert_point_tool_active:
            self._restore_surface_tool_cursor()
        form_subelement = self._form_selection_subelement(
            document, object_name, subelement
        )
        if self.unweld_tool_active:
            edge = (
                self._control_edge_for_subelement(form_subelement)
                if form_subelement is not None
                and canonical_subelement_name(form_subelement).startswith("Edge")
                else None
            )
            self.unweld_hover_edge = edge
            if edge is None:
                self.unweld_segment_edges = None
                self._clear_surface_preview()
                self.view.redraw()
                return
            QtCore.QTimer.singleShot(
                0,
                lambda expected=edge: (
                    self._update_unweld_preview(expected)
                    if not self.cleaned
                    and self.unweld_tool_active
                    and self.unweld_hover_edge == expected
                    else None
                ),
            )
            return
        if form_subelement is not None and not canonical_subelement_name(
            form_subelement
        ).startswith("Edge"):
            return
        try:
            position = tuple(self.view.getCursorPos())
        except (AttributeError, RuntimeError, TypeError):
            position = self.surface_cursor_position
        if position is None:
            return
        self.surface_cursor_position = tuple(position)
        # Selection observers can run before the Selection singleton has
        # published its new SelectionObject. Defer one event-loop turn so
        # _hovered_insert_point() reads the exact preselected edge and pick.
        QtCore.QTimer.singleShot(
            0,
            lambda expected=tuple(position): (
                (
                    self._update_insert_point_preview(expected)
                    if self.insert_point_tool_active
                    else self._update_unweld_preview(expected)
                )
                if not self.cleaned
                and (self.insert_point_tool_active or self.unweld_tool_active)
                and self.surface_cursor_position == expected
                else None
            ),
        )

    def clearSelection(self, document):
        if self.pivot_tool_active:
            return
        if (
            not self.suppress_selection_observer
            and not QtWidgets.QApplication.keyboardModifiers() & QtCore.Qt.ShiftModifier
        ):
            self.range_selection_generation += 1
            self.range_selection_anchors = {}
        if not self.suppress_selection_observer and self.last_added_edge is not None:
            (
                _previous_document,
                _previous_object,
                previous_subelement,
                previous_edge,
                added_at,
            ) = self.last_added_edge
            elapsed = time.monotonic() - added_at
            interval = QtWidgets.QApplication.instance().doubleClickInterval() / 1000.0
            self.last_added_edge = None
            matches = elapsed <= interval
            if matches:
                self.selection_sync_generation += 1
                if self.dragger_switch is not None:
                    self.dragger_switch.whichChild = coin.SO_SWITCH_NONE
                QtCore.QTimer.singleShot(
                    0,
                    lambda edge=previous_subelement: self._select_edge_loop(edge),
                )
                return
        elif not self.suppress_selection_observer:
            self.last_added_edge = None
        self._queue_selection_sync()

    def _queue_selection_sync(self):
        if self.suppress_selection_observer or self.cleaned:
            return
        self.selection_sync_generation += 1
        generation = self.selection_sync_generation
        # Hide immediately. Showing or moving a dragger during the mouse-down
        # event can make that same click begin a drag unexpectedly.
        if self.dragger_switch is not None:
            self.dragger_switch.whichChild = coin.SO_SWITCH_NONE
        QtCore.QTimer.singleShot(0, lambda: self._deferred_selection_sync(generation))

    def _arm_dragger_reveal_deadline(self):
        """Start one dragger delay at the first click of a click sequence."""
        now = time.monotonic()
        if now >= self.dragger_reveal_deadline:
            interval = QtWidgets.QApplication.instance().doubleClickInterval() / 1000.0
            self.dragger_reveal_deadline = now + interval

    def _control_edge_for_subelement(self, subelement):
        try:
            element = self.obj.Shape.getElement(subelement)
        except (Part.OCCError, RuntimeError, ValueError, IndexError):
            return None
        try:
            mapped = self._control_element_mapper().indices(element)
        except (ValueError, RuntimeError):
            return None
        if len(mapped) != 2:
            return None
        edge = tuple(sorted(mapped))
        encoded = str(getattr(self.obj, "TMeshData", "") or "")
        if encoded:
            try:
                mapper = self._control_element_mapper()
                return edge if edge in mapper.mesh.atomic_edges() else None
            except (ValueError, RuntimeError):
                return None
        faces = [tuple(int(index) for index in face.split()) for face in self.obj.ControlFaces]
        return edge if edge in set(cage_edges(faces)) else None

    def _select_edge_loop(self, subelement):
        if self.cleaned:
            return
        selected_edge = self._control_edge_for_subelement(subelement)
        if selected_edge is None:
            return
        try:
            mapper = self._control_element_mapper()
        except (ValueError, RuntimeError):
            return
        cage = mapper.cage
        edge_counts = mapper.mesh.edge_counts() if mapper.mesh is not None else cage.edge_counts()
        boundary_edges = {edge for edge, count in edge_counts.items() if count == 1}
        if mapper.mesh is None:
            # The regular quad-cage walker also handles open edge chains. On
            # an open Face it follows one segmented side and stops at its
            # corners, instead of treating the entire perimeter as one loop.
            loop_edges = set(cage_edge_loop(cage.faces, selected_edge))
        elif selected_edge in boundary_edges:
            loop_edges = set(connected_edge_component(boundary_edges, selected_edge))
        else:
            loop_edges = {selected_edge}
        if loop_edges:
            self._restore_control_selection(set(), loop_edges, defer_dragger=True)

    def _deferred_selection_sync(self, generation):
        if self.cleaned or generation != self.selection_sync_generation:
            return
        if QtWidgets.QApplication.mouseButtons():
            QtCore.QTimer.singleShot(20, lambda: self._deferred_selection_sync(generation))
            return
        if self._whole_additive_feature_is_selected():
            self.select_whole_form()
            return
        if self._whole_additive_form_is_selected():
            self.select_whole_form(update_gui_selection=False)
            return
        targets = self._selected_control_targets()
        selected = set()
        anchors = []
        for _kind, indices, anchor in targets:
            selected.update(indices)
            if anchor is not None:
                anchors.append(anchor)
        self.set_selection(sorted(selected), anchors, defer_dragger=True)

    def _reveal_selection_dragger(self, generation):
        if (
            self.cleaned
            or generation != self.selection_sync_generation
            or self.has_active_tool()
            or not self.selected
            or QtWidgets.QApplication.mouseButtons()
        ):
            return
        self.dragger_switch.whichChild = coin.SO_SWITCH_ALL

    def _whole_additive_form_is_selected(self):
        if "FormPlacement" not in self.obj.PropertiesList:
            return False
        expected = set(self._form_face_subelements())
        if not expected:
            return False
        selected = set()
        for selection in Gui.Selection.getSelectionEx("", Gui.Selection.ResolveMode.NoResolve):
            if not selection.SubElementNames:
                return False
            for raw_name in selection.SubElementNames:
                form_name = self._form_selection_subelement(
                    self.obj.Document.Name,
                    selection.Object.Name,
                    raw_name,
                )
                canonical = canonical_subelement_name(form_name)
                if canonical not in expected:
                    return False
                selected.add(canonical)
        return selected == expected

    def _whole_additive_feature_is_selected(self):
        """Return whether the additive feature itself was selected in the tree."""
        if "FormPlacement" not in self.obj.PropertiesList:
            return False
        return any(
            selection.Object == self.obj and not selection.SubElementNames
            for selection in Gui.Selection.getSelectionEx()
        )

    def _form_face_subelements(self):
        """Return displayed face names belonging to the additive Form only."""
        form_shape = getattr(self.obj, "FormShape", None)
        if form_shape is None or form_shape.isNull():
            return []
        form_faces = form_shape.Faces
        return [
            f"Face{index}"
            for index, face in enumerate(self.obj.Shape.Faces, 1)
            if any(face.isPartner(form_face) for form_face in form_faces)
        ]

    def select_whole_form(self, update_gui_selection=True):
        """Select the complete additive Form and expose its positioning dragger."""
        if "FormPlacement" not in self.obj.PropertiesList:
            return False
        form_faces = self._form_face_subelements()
        if update_gui_selection:
            self.suppress_selection_observer = True
            try:
                Gui.Selection.clearSelection()
                if form_faces:
                    Gui.Selection.addSelection(self.obj, form_faces)
            finally:
                self.suppress_selection_observer = False
        points = self._all_control_points()
        if not points:
            return False
        shape = getattr(self.obj, "FormShape", None)
        anchors = None
        if shape is not None and not shape.isNull():
            bounds = shape.BoundBox
            anchors = [
                App.Vector(
                    (bounds.XMin + bounds.XMax) * 0.5,
                    (bounds.YMin + bounds.YMax) * 0.5,
                    (bounds.ZMin + bounds.ZMax) * 0.5,
                )
            ]
        self.set_selection(range(len(points)), anchors, whole_form=True)
        return True


    def _sharpness_data(self):
        cage = ControlCage.from_object(self.obj)
        return list(cage.vertex_sharpness), dict(cage.edge_sharpness)

    def _control_surface_points(self):
        """Return the BRep corner corresponding to every control-cage vertex."""
        return control_surface_points(self.obj)

    def _control_mapper_signature(self):
        """Return the inexpensive state that determines control/BRep mapping."""
        points = list(self.obj.ControlPoints) + list(getattr(self.obj, "LocalControlPoints", ()))
        return (
            tuple((point.x, point.y, point.z) for point in points),
            tuple(self.obj.ControlFaces),
            str(getattr(self.obj, "TMeshData", "") or ""),
            tuple(float(value) for value in getattr(self.obj, "VertexSharpness", ())),
            tuple(str(value) for value in getattr(self.obj, "EdgeSharpness", ())),
            tuple(str(value) for value in getattr(self.obj, "DissolvedEdges", ())),
            tuple(str(value) for value in getattr(self.obj, "FormSurfaceFaceMap", ())),
            float(self.obj.Shape.BoundBox.DiagonalLength) if not self.obj.Shape.isNull() else 0.0,
        )

    def _control_element_mapper(self):
        """Reuse expensive subdivision limit-point mapping until geometry changes."""
        signature = self._control_mapper_signature()
        if self.cached_control_mapper is None or signature != self.cached_control_mapper_signature:
            self.cached_control_mapper = ControlElementMapper(self.obj)
            self.cached_control_mapper_signature = signature
            self.cached_element_targets = {}
        return self.cached_control_mapper

    def _selected_control_targets(self, respect_symmetry=True):
        try:
            mapper = self._control_element_mapper()
        except ValueError:
            return []
        selections = Gui.Selection.getSelectionEx("", Gui.Selection.ResolveMode.NoResolve)
        targets = []
        locked = set()
        if str(self.obj.FormType) == "Forms::Surface":
            cage = ControlCage.from_object(self.obj)
            locked = {index for edge in cage.boundary_edges for index in edge}
        for selection in selections:
            for raw_subelement_name in selection.SubElementNames:
                form_subelement_name = self._form_selection_subelement(
                    self.obj.Document.Name,
                    selection.Object.Name,
                    raw_subelement_name,
                )
                if form_subelement_name is None:
                    continue
                subelement_name = canonical_subelement_name(form_subelement_name)
                if not subelement_name.startswith(("Vertex", "Edge", "Face")):
                    continue
                cache_key = str(form_subelement_name)
                target = self.cached_element_targets.get(cache_key)
                if target is None:
                    try:
                        # Preserve FreeCAD's complete mapped-element name here.
                        # Reducing it to the first visible VertexN/EdgeN/FaceN
                        # token can resolve a different generated BRep piece.
                        element = self.obj.Shape.getElement(cache_key)
                    except (Part.OCCError, RuntimeError, ValueError, IndexError):
                        try:
                            element = self.obj.Shape.getElement(subelement_name)
                        except (Part.OCCError, RuntimeError, ValueError, IndexError):
                            continue
                    target = mapper.target(element)
                    self.cached_element_targets[cache_key] = target
                mapped, anchor = target
                if mapped:
                    if locked:
                        mapped = tuple(index for index in mapped if index not in locked)
                    mapped = tuple(
                        index
                        for index in mapped
                        if not (
                            respect_symmetry
                            and self.symmetric.isChecked()
                            and index in self.symmetry_plane_points
                        )
                    )
                    if mapped:
                        targets.append(
                            (
                                subelement_name.rstrip("0123456789"),
                                mapped,
                                anchor,
                            )
                        )
        return targets

    def _selected_sharpness_targets(self):
        vertices, edges, _direct_edges, _faces = self._selected_sharpness_context()
        return vertices, edges

    def _selected_sharpness_context(self):
        try:
            mapper = self._control_element_mapper()
        except (Part.OCCError, RuntimeError, ValueError):
            return set(), set(), set(), set()
        cage_faces = (
            [face.boundary for face in mapper.mesh.faces.values()]
            if mapper.mesh is not None
            else mapper.logical_faces
        )
        cage_edge_set = set(tuple(sorted(edge)) for edge in cage_edges(cage_faces))
        cage_face_by_vertices = {frozenset(face): face for face in cage_faces}
        vertices = set()
        edges = set()
        direct_edges = set()
        selected_faces = set()
        base_count = len(self.obj.ControlPoints)
        for kind, indices, _anchor in self._selected_control_targets(respect_symmetry=False):
            if kind == "Vertex" and len(indices) == 1:
                if indices[0] < base_count:
                    vertices.add(indices[0])
            elif kind == "Edge" and len(indices) == 2:
                edge = tuple(sorted(indices))
                if edge in cage_edge_set:
                    edges.add(edge)
                    direct_edges.add(edge)
            elif kind == "Face":
                face = cage_face_by_vertices.get(frozenset(indices))
                if face is not None:
                    selected_faces.add(frozenset(face))
                    for position, start in enumerate(face):
                        edges.add(tuple(sorted((start, face[(position + 1) % len(face)]))))
        return vertices, edges, direct_edges, selected_faces

    def _restore_control_selection(self, vertices, edges, faces=None, defer_dragger=False):
        """Reselect cage targets on the newly generated BRep after recompute."""
        faces = set(faces or ())
        try:
            mapper = self._control_element_mapper()
        except ValueError:
            return
        mapping_shape = mapper.shape
        try:
            filter_membership = not mapping_shape.isSame(self.obj.Shape)
        except (Part.OCCError, RuntimeError):
            filter_membership = True

        def belongs_to_form(element, candidates):
            # Part.makeCompound() creates located partner subshapes rather than
            # retaining the exact FormShape subshape handles. isSame() is false
            # for those Body compound elements; isPartner() preserves the
            # underlying topology identity needed to exclude the base feature.
            return not filter_membership or any(
                element.isPartner(candidate) for candidate in candidates
            )

        subelements = []
        remaining_vertices = set(vertices)
        for shape_index, vertex in enumerate(self.obj.Shape.Vertexes, 1):
            if not remaining_vertices:
                break
            if not belongs_to_form(vertex, mapping_shape.Vertexes):
                continue
            mapped = mapper.indices(vertex)
            control_index = mapped[0] if len(mapped) == 1 else None
            if control_index in remaining_vertices:
                subelements.append(f"Vertex{shape_index}")
                remaining_vertices.remove(control_index)
        remaining_edges = set(edges)
        for shape_index, edge in enumerate(self.obj.Shape.Edges, 1):
            if not remaining_edges:
                break
            if not belongs_to_form(edge, mapping_shape.Edges):
                continue
            mapped = mapper.indices(edge)
            if len(mapped) != 2:
                continue
            control_edge = tuple(sorted(mapped))
            if control_edge in remaining_edges:
                subelements.append(f"Edge{shape_index}")
                remaining_edges.remove(control_edge)
        remaining_faces = set(faces)
        for shape_index, face in enumerate(self.obj.Shape.Faces, 1):
            if not remaining_faces:
                break
            if not belongs_to_form(face, mapping_shape.Faces):
                continue
            mapped = mapper.indices(face)
            if not mapped:
                continue
            control_face = frozenset(mapped)
            if control_face in remaining_faces:
                subelements.append(f"Face{shape_index}")
                remaining_faces.remove(control_face)

        self.suppress_selection_observer = True
        try:
            Gui.Selection.clearSelection()
            if subelements:
                Gui.Selection.addSelection(self.obj, subelements)
        finally:
            self.suppress_selection_observer = False
        selected = set(vertices)
        for edge in edges:
            selected.update(edge)
        for face in faces:
            selected.update(face)
        self.set_selection(
            sorted(selected) if subelements else [],
            defer_dragger=defer_dragger,
        )

    def _sync_sharpness_ui(self):
        vertices, edges, direct_edges, faces = self._selected_sharpness_context()
        self.current_sharpness_targets = (set(vertices), set(edges))
        self.current_sharpness_restore_targets = (
            set(vertices),
            set(direct_edges),
            set(faces),
        )
        vertex_values, edge_values = self._sharpness_data()
        values = [vertex_values[index] for index in vertices]
        values.extend(edge_values.get(edge, 0.0) for edge in edges)
        enabled = bool(values)
        for widget in (self.sharpness_slider, self.sharpness_spin):
            widget.setEnabled(enabled)
        if not values:
            value = 0.0
        else:
            value = values[0] if all(abs(item - values[0]) < 1.0e-9 for item in values) else 0.0
        for widget in (self.sharpness_slider, self.sharpness_spin):
            widget.blockSignals(True)
        self.sharpness_slider.setValue(round(value * 10.0))
        self.sharpness_spin.setValue(value * 10.0)
        for widget in (self.sharpness_slider, self.sharpness_spin):
            widget.blockSignals(False)

    def _queue_selected_sharpness(self, percentage):
        if self.cleaned:
            return
        percentage = max(0.0, min(float(percentage), 100.0))
        for widget in (self.sharpness_slider, self.sharpness_spin):
            widget.blockSignals(True)
        self.sharpness_slider.setValue(round(percentage))
        self.sharpness_spin.setValue(percentage)
        for widget in (self.sharpness_slider, self.sharpness_spin):
            widget.blockSignals(False)
        vertices, edges = self.current_sharpness_targets
        if not vertices and not edges:
            return
        self.pending_sharpness = (
            percentage / 10.0,
            (set(vertices), set(edges)),
            tuple(set(items) for items in self.current_sharpness_restore_targets),
        )
        if not self.sharpness_slider.isSliderDown():
            self.sharpness_update_timer.start()

    def _sharpness_slider_pressed(self):
        self.sharpness_update_timer.stop()

    def _sharpness_slider_released(self):
        if not self.cleaned and self.pending_sharpness is not None:
            self.sharpness_update_timer.start()

    def _apply_pending_sharpness(self):
        if self.cleaned or self.pending_sharpness is None:
            return
        value, targets, restore_targets = self.pending_sharpness
        self.pending_sharpness = None
        self._set_selected_sharpness(value, targets, restore_targets)

    def _set_selected_sharpness(self, value, targets=None, restore_targets=None):
        if self.cleaned:
            return
        vertices, edges = targets if targets is not None else self._selected_sharpness_targets()
        if not vertices and not edges:
            return
        transaction = self._begin_action(App.Qt.translate("Forms_Edit", "Change form sharpness"))
        try:
            if self.obj.CageMode == "Parametric":
                self.obj.CageMode = "Editable"
                self._set_parametric_state(False)
            value = max(0.0, min(float(value), 10.0))
            vertex_values, edge_values = self._sharpness_data()
            for index in vertices:
                vertex_values[index] = value
            for edge in edges:
                if value:
                    edge_values[edge] = value
                else:
                    edge_values.pop(edge, None)
            self.obj.VertexSharpness = vertex_values
            self.obj.EdgeSharpness = [
                f"{edge[0]} {edge[1]} {sharpness:.12g}"
                for edge, sharpness in sorted(edge_values.items())
                if sharpness > 0.0
            ]
            self.obj.Document.recompute()
            if restore_targets is None:
                restore_targets = (vertices, edges, set())
            self._restore_control_selection(*restore_targets)
            self._sync_sharpness_ui()
            self._sync_dimension_properties()
            self._update_dimension_gizmos()
        except MODELING_ERRORS as error:
            self._finish_action(transaction, commit=False)
            self._sync_sharpness_ui()
            report_modeling_error(App.Qt.translate("Forms_Edit", "Change sharpness"), error)
            return
        except Exception:
            self._finish_action(transaction, commit=False)
            raise
        self._finish_action(transaction)

    def set_selection(
        self,
        indices,
        anchors=None,
        whole_form=False,
        defer_dragger=False,
    ):
        all_points = self._all_control_points()
        point_count = len(all_points)
        self.selected = sorted(
            {index for index in indices if isinstance(index, int) and 0 <= index < point_count}
        )
        self.set_pivot_button.setEnabled(bool(self.selected) and not self.has_active_tool())
        self.whole_form_selected = bool(whole_form and self.selected)
        self._set_dimension_gizmos_visible(not self.selected)
        count = len(self.selected)
        if self.whole_form_selected:
            self.selection_label.setText(App.Qt.translate("Forms_Edit", "Whole form selected"))
        else:
            text = (
                App.Qt.translate("Forms_Edit", "1 control point selected")
                if count == 1
                else App.Qt.translate("Forms_Edit", "%1 control points selected").replace(
                    "%1", str(count)
                )
            )
            self.selection_label.setText(text)
        self._show_input_hints()
        if not self.selected:
            self._sync_sharpness_ui()
            self.dragger_switch.whichChild = coin.SO_SWITCH_NONE
            self._refresh_selection_command_actions()
            return
        self.dragger_switch.whichChild = (
            coin.SO_SWITCH_NONE if defer_dragger else coin.SO_SWITCH_ALL
        )
        self.base_points = all_points
        anchors = list(anchors or ())
        center = App.Vector()
        if anchors:
            for anchor in anchors:
                center = center.add(App.Vector(anchor))
            self.base_center = center.multiply(1.0 / len(anchors))
        else:
            for index in self.selected:
                center = center.add(self.base_points[index])
            self.base_center = center.multiply(1.0 / count)
        self._sync_sharpness_ui()
        self._reset_dragger_frame()
        self._update_dragger_scale()
        self._refresh_selection_command_actions()
        if defer_dragger:
            generation = self.selection_sync_generation
            delay = max(
                0,
                int((self.dragger_reveal_deadline - time.monotonic()) * 1000.0),
            )
            QtCore.QTimer.singleShot(
                delay,
                lambda current=generation: self._reveal_selection_dragger(current),
            )

    @staticmethod
    def _refresh_selection_command_actions():
        """Synchronize edit commands after deferred Body-owned selection changes."""
        for name in ("Forms_Match", "Forms_SetPivot"):
            command = Gui.Command.get(name)
            if command is None:
                continue
            enabled = command.isActive()
            for action in command.getAction():
                action.setEnabled(enabled)

    def _reset_dragger_frame(self):
        self.base_dragger_rotation = self._dragger_frame_rotation()
        quaternion = self.base_dragger_rotation.Q
        self.syncing = True
        self.dragger.translation.setValue(
            self.base_center.x, self.base_center.y, self.base_center.z
        )
        self.dragger.rotation.setValue(coin.SbRotation(*quaternion))
        self.dragger.planarScaleFactor.setValue(1.0, 1.0, 1.0)
        self.syncing = False
        self._update_dragger_scale()

    def _dragger_frame_rotation(self):
        mode = self.coordinate_space.currentData()
        if mode == "View":
            return self.view.getCameraOrientation()
        if mode != "Selection":
            return App.Rotation()
        selected = Gui.Selection.getSelectionEx()
        names = [
            name
            for selection in selected
            if selection.Object == self.obj
            for name in selection.SubElementNames
        ]
        names = [canonical_subelement_name(name) for name in names]
        for prefix in ("Face", "Edge"):
            name = next((candidate for candidate in names if candidate.startswith(prefix)), None)
            if name is not None:
                rotation = self._element_frame_rotation(name)
                if rotation is not None:
                    return rotation
        return App.Rotation()

    def _element_frame_rotation(self, name):
        try:
            element = self.obj.Shape.getElement(name)
            if name.startswith("Face"):
                u, v = element.Surface.parameter(element.CenterOfMass)
                z_axis = element.normalAt(u, v)
                edge = element.Edges[0]
                parameter = (edge.FirstParameter + edge.LastParameter) / 2.0
                x_axis = edge.tangentAt(parameter)
                normal_component = App.Vector(z_axis).multiply(x_axis.dot(z_axis))
                x_axis = x_axis.sub(normal_component)
            else:
                parameter = (element.FirstParameter + element.LastParameter) / 2.0
                x_axis = element.tangentAt(parameter)
                reference = App.Vector(0, 0, 1)
                if abs(x_axis.dot(reference)) > 0.9 * x_axis.Length:
                    reference = App.Vector(0, 1, 0)
                z_axis = x_axis.cross(reference)
            if x_axis.Length < 1.0e-9 or z_axis.Length < 1.0e-9:
                return None
            x_axis.normalize()
            z_axis.normalize()
            y_axis = z_axis.cross(x_axis)
            y_axis.normalize()
            return App.Rotation(x_axis, y_axis, z_axis, "XYZ")
        except (AttributeError, Part.OCCError, RuntimeError, ValueError, IndexError):
            return None

    def dragger_started(self, _dragger):
        """Capture whether Alt-drag should add a face or a boundary strip."""
        self.dragger_transaction_open = self._begin_action(
            App.Qt.translate("Forms_Edit", "Transform form")
        )
        # Each gesture is relative to the dragger's starting matrix. Refresh
        # the matching geometry baseline as well, otherwise a second gesture
        # reapplies its unchanged axes from the selection's original points.
        self.base_points = self._all_control_points()
        self.base_center = App.Vector(*self.dragger.translation.getValue().getValue())
        quaternion = self.dragger.rotation.getValue().getValue()
        self.base_dragger_rotation = App.Rotation(*quaternion)
        if "FormPlacement" in self.obj.PropertiesList:
            self.base_form_placement = self.obj.FormPlacement.copy()
        self.base_object_placement = self.obj.Placement.copy()
        self.pending_form_placement = None
        self.pending_control_points = None
        self.whole_form_motion_preview = False
        self.base_dimension_gizmo_frames = {
            name: self._dimension_gizmo_placement(name) for name in self.dimension_gizmos
        }
        self.alt_extrude_face_indices = set()
        self.alt_extrude_boundary_edges = set()
        self.extruded_top_faces = set()
        self.extruded_outer_edges = set()
        if self.whole_form_selected:
            return
        modifiers = QtWidgets.QApplication.keyboardModifiers()
        if not modifiers & QtCore.Qt.AltModifier:
            return
        target, payload = self._alt_drag_target()
        if target == "faces":
            self.alt_extrude_face_indices = set(payload)
        elif target == "boundary_edges":
            self.alt_extrude_boundary_edges = set(payload)

    def _begin_alt_extrusion(self):
        if not self.alt_extrude_face_indices and not self.alt_extrude_boundary_edges:
            return False
        modifiers = QtWidgets.QApplication.keyboardModifiers()
        keep_creases = bool(modifiers & QtCore.Qt.ControlModifier)
        cage = ControlCage.from_object(self.obj)
        if self.alt_extrude_face_indices:
            cage, tops, _side_faces = cage.extrude_faces(
                self.alt_extrude_face_indices,
                keep_creases,
            )
            self.selected = sorted({index for top in tops for index in top})
            self.extruded_top_faces = {frozenset(top) for top in tops}
        else:
            cage, outer_edges, _side_faces = cage.extrude_boundary_edges(
                self.alt_extrude_boundary_edges,
                keep_creases,
            )
            self.selected = sorted({vertex for edge in outer_edges for vertex in edge})
            self.extruded_outer_edges = set(outer_edges)
        self.obj.CageMode = "Editable"
        cage.write(self.obj)
        self._set_parametric_state(False)
        self.base_points = [App.Vector(point) for point in cage.vertices]
        self.alt_extrude_face_indices = set()
        self.alt_extrude_boundary_edges = set()
        self.selection_label.setText(
            App.Qt.translate("Forms_Edit", "%n control point(s) selected", "", len(self.selected))
        )
        return True

    def dragger_moved(self, dragger):
        if self.syncing or not self.selected:
            return
        if self.alt_extrude_face_indices or self.alt_extrude_boundary_edges:
            translation = App.Vector(*dragger.translation.getValue().getValue())
            if translation.sub(self.base_center).Length > 1.0e-7:
                self._begin_alt_extrusion()
        translation = App.Vector(*dragger.translation.getValue().getValue())
        quaternion = dragger.rotation.getValue().getValue()
        rotation = App.Rotation(*quaternion).multiply(self.base_dragger_rotation.inverted())
        scale = App.Vector(*dragger.planarScaleFactor.getValue().getValue())
        scaled = any(abs(component - 1.0) > 1.0e-7 for component in (scale.x, scale.y, scale.z))
        if self.whole_form_selected and not scaled:
            preview_base = translation.sub(rotation.multVec(self.base_center))
            preview_delta = App.Placement(preview_base, rotation)
            self.obj.Placement = preview_delta.multiply(self.base_object_placement)
            self.whole_form_motion_preview = True
            if self.obj.CageMode == "Parametric" and "FormPlacement" in self.obj.PropertiesList:
                placement = self.base_form_placement
                base = translation.add(rotation.multVec(placement.Base.sub(self.base_center)))
                self.pending_form_placement = App.Placement(
                    base, rotation.multiply(placement.Rotation)
                )
            else:
                points = [App.Vector(point) for point in self.base_points]
                for index in self.selected:
                    relative = self.base_points[index].sub(self.base_center)
                    points[index] = translation.add(rotation.multVec(relative))
                self._enforce_symmetry(points, set(self.selected))
                self.pending_control_points = points
            self._transform_dimension_gizmos(translation, rotation)
            self.view.redraw()
            return
        if self.obj.CageMode == "Parametric":
            self.obj.CageMode = "Editable"
            self._set_parametric_state(False)
        points = [App.Vector(point) for point in self.base_points]
        for index in self.selected:
            relative = self.base_points[index].sub(self.base_center)
            local = self.base_dragger_rotation.inverted().multVec(relative)
            local = App.Vector(local.x * scale.x, local.y * scale.y, local.z * scale.z)
            points[index] = translation.add(
                App.Rotation(*quaternion).multVec(local)
            )
        self._enforce_symmetry(points, set(self.selected))
        self._set_control_points(points, recompute=True)

    def dragger_finished(self, dragger):
        try:
            self.dragger_moved(dragger)
            if self.whole_form_motion_preview:
                self.obj.Placement = self.base_object_placement
                if self.pending_form_placement is not None:
                    self.obj.FormPlacement = self.pending_form_placement
                elif self.pending_control_points is not None:
                    self._set_control_points(self.pending_control_points)
                self.whole_form_motion_preview = False
            self.obj.Document.recompute()
            self._sync_dimension_properties()
            self._update_dimension_gizmos()
            if self.whole_form_selected:
                self.select_whole_form(update_gui_selection=False)
            elif self.extruded_top_faces:
                tops = set(self.extruded_top_faces)
                self.extruded_top_faces = set()
                self.alt_extrude_face_indices = set()
                self._restore_control_selection(set(), set(), tops)
            elif self.extruded_outer_edges:
                edges = set(self.extruded_outer_edges)
                self.extruded_outer_edges = set()
                self.alt_extrude_boundary_edges = set()
                self._restore_control_selection(set(), edges)
        except MODELING_ERRORS as error:
            if self.whole_form_motion_preview:
                self.obj.Placement = self.base_object_placement
                self.whole_form_motion_preview = False
            self._finish_action(self.dragger_transaction_open, commit=False)
            self.dragger_transaction_open = False
            self._sync_dimension_properties()
            self._update_dimension_gizmos()
            report_modeling_error(App.Qt.translate("Forms_Edit", "Transform Form"), error)
            return
        except Exception:
            if self.whole_form_motion_preview:
                self.obj.Placement = self.base_object_placement
                self.whole_form_motion_preview = False
            self._finish_action(self.dragger_transaction_open, commit=False)
            self.dragger_transaction_open = False
            raise
        self._finish_action(self.dragger_transaction_open)
        self.dragger_transaction_open = False

    def _flush_pending_updates(self):
        self.parameter_update_timer.stop()
        self.sharpness_update_timer.stop()
        self._apply_pending_sharpness()
        self._apply_pending_parameter_changes()

    def _cancel_pending_updates(self):
        self.parameter_update_timer.stop()
        self.sharpness_update_timer.stop()
        self.thicken_update_timer.stop()
        self.pending_parameter_changes = {}
        self.pending_sharpness = None

    def isAllowedAlterDocument(self):
        """Keep Undo/Redo and Forms modify commands available while editing."""
        return True

    def getStandardButtons(self):
        """Finish the live edit or restore its serialized starting state."""
        return QtWidgets.QDialogButtonBox.Ok | QtWidgets.QDialogButtonBox.Cancel

    def accept(self):
        if self.straighten_tool_active:
            self.apply_straighten_tool()
        if self.flatten_tool_active:
            self.apply_flatten_tool()
        if self.match_tool_active:
            self.apply_match_tool()
        if self.weld_tool_active:
            self.stop_weld_tool()
        if self.thicken_tool_active:
            self.stop_thicken_tool(apply=True)
        self._flush_pending_updates()
        self.obj.Document.recompute()
        if self.document_edit:
            document = self.obj.Document
            Gui.getDocument(document.Name).resetEdit()
            if self.creation_transaction and document.getBookedTransactionID() != 0:
                document.commitTransaction()
        else:
            self.obj.Document.commitTransaction()
            self.cleanup()
        return True

    def reject(self):
        if self.straighten_tool_active:
            self.stop_straighten_tool()
        if self.flatten_tool_active:
            self.stop_flatten_tool()
        if self.match_tool_active:
            self.stop_match_tool()
        if self.weld_tool_active:
            self.stop_weld_tool()
        if self.thicken_tool_active:
            self.stop_thicken_tool(apply=False)
        self._cancel_pending_updates()
        document = self.obj.Document
        if self.document_edit:
            gui_document = Gui.getDocument(document.Name)
            if self.creation_transaction:
                # resetEdit() may close the active transaction as an accepted
                # edit. Abort first so a newly-created feature is removed,
                # matching FreeCAD's other creation task panels.
                document.abortTransaction()
                gui_document.resetEdit()
                document.recompute()
            else:
                self.editing_cancelled = True
                gui_document.resetEdit()
        else:
            self.cleanup()
            document.abortTransaction()
            document.recompute()
        return True

    def cleanup(self):
        global _active_session

        if self.cleaned:
            return
        # Mark the session inert before touching document or viewer state. A
        # failed recompute, closing document, or already-destroyed 3D view must
        # never leave callbacks registered against a half-closed editor.
        self.cleaned = True
        if _active_session is self:
            _active_session = None

        def safely(action, description):
            try:
                action()
            except Exception as error:
                App.Console.PrintWarning(f"Forms cleanup ({description}): {error}\n")

        if self.whole_form_motion_preview:
            safely(
                lambda: setattr(self.obj, "Placement", self.base_object_placement),
                "whole-form motion preview",
            )
            self.whole_form_motion_preview = False

        if self.thicken_tool_active:
            # Accept and Reject explicitly finish Thicken before resetEdit().
            # Any residual tool here is an abnormal exit and must be restored.
            safely(lambda: self.stop_thicken_tool(apply=False), "Thicken")
        safely(self.stop_straighten_tool, "Straighten")
        safely(self.stop_flatten_tool, "Flatten")
        safely(self.stop_match_tool, "Match")
        safely(self.stop_weld_tool, "Weld")
        safely(self.stop_set_pivot_tool, "Set Pivot")
        safely(self.stop_surface_tool, "topology tool")
        for attribute in (
            "dragger_transaction_open",
            "dimension_transaction_open",
            "thicken_transaction_open",
        ):
            opened = bool(getattr(self, attribute, False))
            safely(
                lambda active=opened: self._finish_action(active, commit=False),
                attribute,
            )
            setattr(self, attribute, False)
        self._cancel_pending_updates()
        self.selection_sync_generation += 1
        self.range_selection_generation += 1
        if self.selection_observer_added:
            safely(lambda: Gui.Selection.removeObserver(self), "selection observer")
            self.selection_observer_added = False
        if self.document_observer_added:
            safely(lambda: App.removeDocumentObserver(self), "document observer")
            self.document_observer_added = False
        if self.selection_gate_added:
            safely(Gui.Selection.removeSelectionGate, "selection gate")
            self.selection_gate_added = False
        if self.key_filter is not None:
            application = QtWidgets.QApplication.instance()
            if application is not None:
                safely(
                    lambda: application.removeEventFilter(self.key_filter),
                    "event filter",
                )
            self.key_filter = None
        safely(lambda: QtCore.QTimer.singleShot(0, Gui.HintManager.hide), "input hints")
        if self.camera_sensor is not None:
            safely(self.camera_sensor.detach, "camera sensor")
            self.camera_sensor = None
        if self.camera_orientation_sensor is not None:
            safely(self.camera_orientation_sensor.detach, "orientation sensor")
            self.camera_orientation_sensor = None
        if self.viewer is not None and self.previous_pick_radius is not None:
            safely(
                lambda: self.viewer.setPickRadius(self.previous_pick_radius),
                "pick radius",
            )
            self.previous_pick_radius = None
        if self.previous_point_size is not None:
            safely(
                lambda: setattr(self.view_object, "PointSize", self.previous_point_size),
                "point size",
            )
            self.previous_point_size = None
        if self.dragger is not None:
            for callback_type, callback in self.dragger_callbacks:
                safely(
                    lambda kind=callback_type, slot=callback: self.view.removeDraggerCallback(
                        self.dragger, kind, slot
                    ),
                    "transform dragger",
                )
        self.dragger_callbacks = []
        for dragger, callback_type, callback in self.dimension_gizmo_callbacks:
            safely(
                lambda node=dragger, kind=callback_type, slot=callback: self.view.removeDraggerCallback(
                    node, kind, slot
                ),
                "dimension dragger",
            )
        self.dimension_gizmo_callbacks = []
        safely(lambda: self.view_object.RootNode.removeChild(self.root), "scene root")

        for timer in (
            self.parameter_update_timer,
            self.sharpness_update_timer,
            self.thicken_update_timer,
        ):
            safely(timer.stop, "timer")
            safely(timer.timeout.disconnect, "timer signal")
            safely(timer.deleteLater, "timer deletion")

        if self.editing_cancelled and self.edit_backup is not None:

            def restore_cancelled_edit():
                document = self.obj.Document
                transaction = document.getBookedTransactionID() == 0
                if transaction:
                    document.openTransaction(App.Qt.translate("Forms_Edit", "Cancel form editing"))
                try:
                    self.obj.restoreContent(self.edit_backup)
                    self.obj.purgeTouched()
                    document.recompute()
                except Exception:
                    if transaction and document.getBookedTransactionID() != 0:
                        document.abortTransaction()
                    raise
                if transaction and document.getBookedTransactionID() != 0:
                    document.commitTransaction()

            safely(restore_cancelled_edit, "cancelled edit restoration")
        self.edit_backup = None
        if self.profile_edit_shape_owned:
            proxy = getattr(self.obj, "Proxy", None)
            if proxy is not None and hasattr(proxy, "show_edit_shape"):
                safely(
                    lambda: proxy.show_edit_shape(self.obj, False),
                    "profile face display",
                )
            self.profile_edit_shape_owned = False
        safely(lambda: set_forms_toolbar_mode(False), "toolbar mode")
        safely(self.view.redraw, "view redraw")
