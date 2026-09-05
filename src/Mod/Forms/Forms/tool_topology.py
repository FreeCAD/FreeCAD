# SPDX-License-Identifier: LGPL-2.1-or-later
"""Topology tool controls, preview, and commit/cancel lifecycle."""

import math
import FreeCAD as App
import FreeCADGui as Gui
import Part
from PySide import QtCore, QtWidgets
from pivy import coin
from .cage import ControlCage, canonical_subelement_name
from .feedback import MODELING_ERRORS, report_modeling_error
from .operations import (
    insert_edge_loop,
    insert_edge_on_face,
    insert_point_face_target,
    insert_point_edges,
    local_insert_target,
    subdivide_faces,
    unweld_segment,
)
from .taskpanels import load_panel
from .topology import cage_edge_loop
from .tool_controller import ToolController


class TopologyTool(ToolController):
    def __init__(self, session):
        super().__init__(session)
        self.surface_tool_callback = None
        self.surface_tool_mouse_callback = None
        self.surface_tool_cursor_icon = None
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
        self.subdivide_u = None
        self.subdivide_v = None
        self.subdivide_last_counts = {"u": 2, "v": 2}

    def start_insert_edge_tool(self):
        """Start the repeatable hover-preview Insert Edge handler."""
        widget = load_panel("TaskFormInsertEdge.ui")
        is_surface = str(self.session.obj.FormType) == "Forms::Surface"
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
        if str(self.session.obj.FormType) == "Forms::Surface":
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
        if str(self.session.obj.FormType) == "Forms::Surface":
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
        if self.session.cleaned or self.session.has_active_tool():
            widget.deleteLater()
            return False
        self.session._flush_pending_updates()
        self.session._clear_editor_selection()
        self.session.active_tool = tool
        self.surface_tool_cursor_icon = cursor_icon
        self.insert_orientation = 0
        self.insert_point_chain = []
        self.insert_point_hover = None
        self.unweld_segment_edges = None
        self.unweld_hover_edge = None
        self.unweld_separate_forms = None
        self.surface_cursor_position = None
        self.session._show_tool_handler(title, widget, command_name)
        if tool in ("insert_point", "unweld"):
            self.session._install_selection_gate()
        # Insert Point intentionally keeps viewer picking enabled: its hover
        # state is driven by FreeCAD's native edge preselection. The Pivy
        # mouse callback still owns clicks, so this does not create selections.
        self.session._suspend_selection_for_tool(
            disable_selection=tool not in ("insert_point", "unweld")
        )
        self._create_surface_preview()
        self._reset_surface_tool_cache()
        try:
            if hasattr(self.session.view, "activateToolHandler"):
                self.session.view.activateToolHandler(self.surface_tool_cursor_icon)
            self.surface_tool_callback = self.session.view.addEventCallback(
                "SoEvent", self._surface_tool_event
            )
            self.surface_tool_mouse_callback = self.session.view.addEventCallbackPivy(
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
        if self.session.insert_tool_active:
            self.stop_surface_tool()

    def stop_subdivide_tool(self):
        if self.session.subdivide_tool_active:
            self.stop_surface_tool()

    def stop_surface_tool(self):
        """Dismiss a hover topology handler without ending Form edit."""
        if (
            not self.session.surface_tool_active
            and self.surface_tool_callback is None
            and self.surface_tool_mouse_callback is None
        ):
            return
        self.session.active_tool = None
        self.surface_tool_cursor_icon = None
        if self.surface_tool_callback is not None:
            try:
                self.session.view.removeEventCallback("SoEvent", self.surface_tool_callback)
            except (AttributeError, RuntimeError):
                pass
            self.surface_tool_callback = None
        if self.surface_tool_mouse_callback is not None:
            try:
                self.session.view.removeEventCallbackPivy(
                    coin.SoMouseButtonEvent.getClassTypeId(),
                    self.surface_tool_mouse_callback,
                )
            except (AttributeError, RuntimeError):
                pass
            self.surface_tool_mouse_callback = None
        if hasattr(self.session.view, "deactivateToolHandler"):
            try:
                self.session.view.deactivateToolHandler()
            except (AttributeError, RuntimeError):
                pass
        self.session._resume_selection_after_tool()
        self.session._install_selection_gate()
        self.session._hide_tool_handler()
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
        if not self.session.cleaned:
            self.session._show_input_hints()
        self.session.view.redraw()
        # Command IsActive() depends on active_tool. Refresh it immediately;
        # otherwise the command manager waits for the next GUI/selection event.
        Gui.Command.update()

    def _insert_loop_changed(self, _checked):
        if self.session.insert_tool_active and self.surface_cursor_position is not None:
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
        self.session.root.addChild(self.surface_preview_switch)

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
        self.surface_tool_control_points = self.session._control_surface_points()
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
        if self.session.unweld_tool_active:
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
        if self.session.insert_point_tool_active:
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
            if self.session.insert_tool_active
            else App.Qt.translate("Forms_Edit", "%1 subdivide the hovered face")
        )
        switch = (
            App.Qt.translate("Forms_Edit", "%1 switch edge direction")
            if self.session.insert_tool_active
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
        if self.session.subdivide_tool_active and self.surface_cursor_position is not None:
            self._update_subdivide_preview(self.surface_cursor_position)

    def toggle_surface_tool_orientation(self):
        if not self.session.surface_tool_active:
            return False
        if self.session.insert_point_tool_active or self.session.unweld_tool_active:
            return False
        if self.session.insert_tool_active:
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
        if self.session.cleaned or not self.session.surface_tool_active:
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
            if self.session.insert_point_tool_active:
                self._restore_surface_tool_cursor()
            self.surface_cursor_position = tuple(info.get("Position", ()))
            # Unweld is driven exclusively by FreeCAD's native preselection.
            # Re-picking by screen position here made its wire flicker between
            # location events even while the native edge stayed preselected.
            if not self.session.unweld_tool_active:
                self._update_surface_tool_preview(self.surface_cursor_position)
            return

    def _surface_tool_mouse_event(self, event_callback):
        """Own mouse clicks while a topology handler is running."""
        if self.session.cleaned or not self.session.surface_tool_active:
            return
        event = event_callback.getEvent()
        if event.getState() != coin.SoButtonEvent.DOWN:
            return
        button = event.getButton()
        if button == coin.SoMouseButtonEvent.BUTTON2:
            # The dictionary callback cannot mark an event handled, which let
            # the navigation style open its context menu after dismissal.
            event_callback.setHandled()
            if self.session.insert_point_tool_active and self.insert_point_chain:
                self._commit_insert_point_chain()
            else:
                self.session._later(0, self.stop_surface_tool)
        elif button == coin.SoMouseButtonEvent.BUTTON1:
            event_callback.setHandled()
            if self.session.insert_point_tool_active:
                self._append_insert_point()
            else:
                self._commit_surface_tool_preview()

    def _hovered_insert_point(self, position):
        """Return ``(edge, fraction, surface point)`` under the cursor."""
        info = self.session.view.getObjectInfo(tuple(position)) if len(position) == 2 else None
        try:
            mapper = self.session._control_element_mapper()
            self._ensure_surface_tool_cache(mapper)
        except (ValueError, RuntimeError):
            return None
        names = []
        picked = None
        try:
            preselection = Gui.Selection.getPreselection()
            object_name = str(getattr(preselection, "ObjectName", "") or "")
            document_name = str(
                getattr(preselection, "DocumentName", "") or self.session.obj.Document.Name
            )
            for raw_name in getattr(preselection, "SubElementNames", ()):
                form_name = self.session._form_selection_subelement(
                    document_name, object_name, raw_name
                )
                if form_name is not None:
                    names.append(form_name)
            picked_points = tuple(getattr(preselection, "PickedPoints", ()) or ())
            if picked_points:
                picked = self.session._global_placement(self.session.obj).inverse().multVec(
                    App.Vector(picked_points[0])
                )
        except (AttributeError, RuntimeError, TypeError, ValueError):
            pass
        # getObjectInfo remains a fallback for navigation styles which update
        # the mouse callback just before publishing their preselection object.
        if info:
            info_names = []
            for raw_name in (info.get("SubName"), info.get("Component")):
                form_name = self.session._form_selection_subelement(
                    info.get("Document", self.session.obj.Document.Name),
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
                    picked = self.session._global_placement(self.session.obj).inverse().multVec(
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
                    element = self.session.obj.Shape.getElement(candidate)
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
                mapper = self.session._control_element_mapper()
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
                scale = max(self.session.obj.Shape.BoundBox.DiagonalLength * 0.004, 1.0e-4)
                point = points[0]
                curves = [
                    [point.add(App.Vector(-scale, 0, 0)), point.add(App.Vector(scale, 0, 0))],
                    [point.add(App.Vector(0, -scale, 0)), point.add(App.Vector(0, scale, 0))],
                ]
            self._set_surface_preview_curves(curves)
        self.session.view.redraw()

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
            cage = ControlCage.from_object(self.session.obj)
            if (
                getattr(self.session.obj, "LocalEdgeInserts", ())
                or str(getattr(self.session.obj, "TMeshData", "") or "")
                or getattr(self.session.obj, "DissolvedEdges", ())
            ):
                raise ValueError("Unweld requires an all-quad base control cage")
            if not cage.is_closed:
                raise ValueError("Unweld currently requires a closed Form")
            segment = tuple(cage_edge_loop(cage.faces, edge))
            # Do the same validation as commit so a visually accepted preview
            # can never turn into a non-separating or partial cut on click.
            cage.split_along_edges(segment)
            mapper = self.session._control_element_mapper()
            segment_set = set(segment)
            curves = []
            for shape_edge in self.session.obj.Shape.Edges:
                mapped = tuple(sorted(mapper.indices(shape_edge)))
                if len(mapped) == 2 and mapped in segment_set:
                    curves.append(
                        [App.Vector(point) for point in shape_edge.discretize(Number=17)]
                    )
        except (Part.OCCError, RuntimeError, TypeError, ValueError, IndexError):
            self._clear_surface_preview()
            self.session.view.redraw()
            return
        if not curves:
            self._clear_surface_preview()
            self.session.view.redraw()
            return
        self.unweld_segment_edges = segment
        self._set_surface_preview_curves(curves)
        self.surface_preview_key = preview_key
        self.session.view.redraw()

    def _commit_unweld_preview(self):
        if not self.session.unweld_tool_active or not self.unweld_segment_edges:
            return False
        self.session._flush_pending_updates()
        transaction = self.session._begin_action(App.Qt.translate("Forms_Unweld", "Unweld Form"))
        try:
            objects = unweld_segment(
                self.session.obj,
                self.unweld_segment_edges,
                separate_forms=(
                    self.unweld_separate_forms is not None
                    and self.unweld_separate_forms.isChecked()
                ),
            )
            if self.session.edit_backup is not None:
                self.session.edit_backup.record_created(objects)
            self.session.obj.Document.recompute()
            self.session.cached_control_mapper = None
            self.session.cached_control_mapper_signature = None
            self._reset_surface_tool_cache()
            self.session._clear_editor_selection(clear_preselection=True)
            self.session.topology_changed()
            self._clear_surface_preview()
            self.session.view.redraw()
        except MODELING_ERRORS as error:
            self.session._finish_action(transaction, commit=False)
            return report_modeling_error(
                App.Qt.translate("Forms_Unweld", "Unweld Form"), error
            )
        except Exception:
            self.session._finish_action(transaction, commit=False)
            raise
        self.session._finish_action(transaction)
        self.session._later(0, self.stop_surface_tool)
        return True

    def _append_insert_point(self):
        if not self.session.insert_point_tool_active or self.insert_point_hover is None:
            return False
        edge, fraction, point = self.insert_point_hover
        if self.insert_point_chain:
            previous_edge, previous_fraction, _previous_point = self.insert_point_chain[-1]
            try:
                mapper = self.session._control_element_mapper()
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
        if not self.session.insert_point_tool_active or not self.insert_point_chain:
            return False
        if len(self.insert_point_chain) < 2:
            self.insert_point_chain = []
            self.insert_point_hover = None
            self._clear_surface_preview()
            self.session.view.redraw()
            return True
        self.session._flush_pending_updates()
        transaction = self.session._begin_action(App.Qt.translate("Forms_InsertPoint", "Insert points"))
        try:
            insert_point_edges(
                self.session.obj,
                [(edge, fraction) for edge, fraction, _point in self.insert_point_chain],
            )
            self.session.obj.Document.recompute()
            self.session._set_parametric_state(False)
            self.session._clear_editor_selection(clear_preselection=True)
            self.session._sync_dimension_properties()
            self.session._update_dimension_gizmos()
            self._reset_surface_tool_cache()
            self.insert_point_chain = []
            self.insert_point_hover = None
            self._clear_surface_preview()
            self._queue_tool_selection_clear()
            self.session.view.redraw()
        except MODELING_ERRORS as error:
            self.session._finish_action(transaction, commit=False)
            return report_modeling_error(
                App.Qt.translate("Forms_InsertPoint", "Insert points"), error
            )
        except Exception:
            self.session._finish_action(transaction, commit=False)
            raise
        self.session._finish_action(transaction)
        return True

    def _hovered_control_face(self, position):
        info = self.session.view.getObjectInfo(tuple(position)) if len(position) == 2 else None
        if not info or str(info.get("Object", "")) != self.session.obj.Name:
            return None
        names = [info.get("SubName"), info.get("Component")]
        cache_key = tuple(str(name or "") for name in names)
        if cache_key in self.surface_tool_hover_faces:
            return self.surface_tool_hover_faces[cache_key]
        try:
            mapper = self.session._control_element_mapper()
            self._ensure_surface_tool_cache(mapper)
        except (ValueError, RuntimeError):
            return None
        for name in names:
            canonical = canonical_subelement_name(name)
            if not canonical.startswith("Face"):
                continue
            for candidate in (str(name), canonical):
                try:
                    element = self.session.obj.Shape.getElement(candidate)
                    if str(self.session.obj.FormType) == "Forms::Surface" and any(
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
        if str(self.session.obj.FormType) == "Forms::Surface":
            result = mapper.form_surface_faces[0] if mapper.form_surface_faces else None
            self.surface_tool_shape_faces[face_index] = result
            return result
        target_face = mapper.mesh.faces[face_index] if mapper.mesh is not None else None
        target = frozenset(
            target_face.boundary
            if target_face is not None
            else ControlCage.from_object(self.session.obj).faces[face_index]
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
        surface_points = surface_points or self.session._control_surface_points()
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

    def _update_insert_preview(self, position):
        face_index = self._hovered_control_face(position)
        if face_index is None:
            self._clear_surface_preview()
            self.session.view.redraw()
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
        if str(self.session.obj.FormType) == "Forms::Surface":
            try:
                mapper = self.session._control_element_mapper()
                domain = self.session._form_surface_domain(mapper)
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
                self.session.view.redraw()
                return
            self._set_surface_preview_curves([curve])
            self.surface_preview_key = preview_key
            self.surface_hover_face = face_index
            self.session.view.redraw()
            return
        try:
            mapper = self.session._control_element_mapper()
            topology = mapper.mesh or ControlCage.from_object(self.session.obj)
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
                _ring_start, ring_edges, target_faces = self.session._whole_loop_data(
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
            self.session.view.redraw()
            return
        if not curves:
            self.session.view.redraw()
            return
        self._set_surface_preview_curves(curves)
        self.surface_preview_key = preview_key
        self.surface_hover_face = face_index
        self.session.view.redraw()

    def _update_subdivide_preview(self, position):
        self._clear_surface_preview()
        face_index = self._hovered_control_face(position)
        if face_index is None:
            self.session.view.redraw()
            return
        try:
            mapper = self.session._control_element_mapper()
            u_count = self.subdivide_u.value()
            v_count = self.subdivide_v.value()
            if str(self.session.obj.FormType) == "Forms::Surface":
                domain = self.session._form_surface_domain(mapper)
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
            self.session.view.redraw()
            return
        if not curves:
            self.session.view.redraw()
            return
        points = [(point.x, point.y, point.z) for curve in curves for point in curve]
        counts = [len(curve) for curve in curves]
        self.surface_preview_coordinates.point.setValues(0, len(points), points)
        self.surface_preview_lines.numVertices.setValues(0, len(counts), counts)
        self.surface_preview_switch.whichChild = coin.SO_SWITCH_ALL
        self.surface_hover_face = face_index
        self.session.view.redraw()

    def _update_surface_tool_preview(self, position):
        if self.session.insert_tool_active:
            self._update_insert_preview(position)
        elif self.session.insert_point_tool_active:
            self._update_insert_point_preview(position)
        elif self.session.subdivide_tool_active:
            self._update_subdivide_preview(position)

    def _commit_surface_tool_preview(self):
        if self.session.insert_tool_active:
            return self._commit_insert_preview()
        if self.session.subdivide_tool_active:
            return self._commit_subdivide_preview()
        if self.session.unweld_tool_active:
            return self._commit_unweld_preview()
        return False

    def _commit_insert_preview(self):
        if self.session.cleaned or not self.session.insert_tool_active or self.surface_hover_face is None:
            return False
        self.session._flush_pending_updates()
        transaction = self.session._begin_action(App.Qt.translate("Forms_Edit", "Insert form edge"))
        try:
            if str(self.session.obj.FormType) == "Forms::Surface":
                property_name = "USegments" if self.insert_orientation == 0 else "VSegments"
                self._set_form_surface_segments(
                    int(self.session.obj.USegments) + (property_name == "USegments"),
                    int(self.session.obj.VSegments) + (property_name == "VSegments"),
                )
            else:
                whole_loop = bool(
                    self.insert_whole_loop is not None and self.insert_whole_loop.isChecked()
                )
                if whole_loop:
                    mapper = self.session._control_element_mapper()
                    if mapper.mesh is not None:
                        raise ValueError("Whole-loop insertion currently requires an all-quad cage")
                    topology = ControlCage.from_object(self.session.obj)
                    insert_edge, _targets, _side = local_insert_target(
                        topology,
                        self.surface_hover_face,
                        self.insert_orientation,
                        "left",
                    )
                    ring_start, _ring_edges, _faces = self.session._whole_loop_data(
                        topology, self.surface_hover_face, insert_edge
                    )
                    insert_edge_loop(self.session.obj, ring_start)
                else:
                    insert_edge_on_face(
                        self.session.obj,
                        self.surface_hover_face,
                        self.insert_orientation,
                        "left",
                    )
            self.session.obj.Document.recompute()
            self._reset_surface_tool_cache()
            self.session._set_parametric_state(False)
            self.session._clear_editor_selection(clear_preselection=True)
            self.session._sync_dimension_properties()
            self.session._update_dimension_gizmos()
            self._clear_surface_preview()
            self._queue_tool_selection_clear()
            self.session.view.redraw()
        except MODELING_ERRORS as error:
            self.session._finish_action(transaction, commit=False)
            report_modeling_error(App.Qt.translate("Forms_InsertEdge", "Insert Edge"), error)
            self._clear_surface_preview()
            self.session.view.redraw()
            return False
        except Exception:
            self.session._finish_action(transaction, commit=False)
            raise
        self.session._finish_action(transaction)
        return True

    def _commit_subdivide_preview(self):
        if self.session.cleaned or not self.session.subdivide_tool_active or self.surface_hover_face is None:
            return False
        self.session._flush_pending_updates()
        transaction = self.session._begin_action(App.Qt.translate("Forms_Edit", "Subdivide form face"))
        try:
            if str(self.session.obj.FormType) == "Forms::Surface":
                self._set_form_surface_segments(
                    int(self.session.obj.USegments) * self.subdivide_u.value(),
                    int(self.session.obj.VSegments) * self.subdivide_v.value(),
                )
            else:
                subdivide_faces(
                    self.session.obj,
                    (self.surface_hover_face,),
                    self.subdivide_u.value(),
                    self.subdivide_v.value(),
                )
            self.session.obj.Document.recompute()
            self._reset_surface_tool_cache()
            self.session._set_parametric_state(False)
            self.session._clear_editor_selection(clear_preselection=True)
            self.session._sync_dimension_properties()
            self.session._update_dimension_gizmos()
            self._clear_surface_preview()
            self._queue_tool_selection_clear()
            self.session.view.redraw()
        except MODELING_ERRORS as error:
            self.session._finish_action(transaction, commit=False)
            report_modeling_error(App.Qt.translate("Forms_Subdivide", "Subdivide"), error)
            self._clear_surface_preview()
            self.session.view.redraw()
            return False
        except Exception:
            self.session._finish_action(transaction, commit=False)
            raise
        self.session._finish_action(transaction)
        return True

    def _set_form_surface_segments(self, u_segments, v_segments):
        """Resize the regular control grid used by a filled Part Design face."""
        u_segments = int(u_segments)
        v_segments = int(v_segments)
        if self.session.obj.CageMode == "Parametric":
            self.session.obj.USegments = u_segments
            self.session.obj.VSegments = v_segments
            self.session.obj.Document.recompute()
            for name, value in (
                ("USegments", u_segments),
                ("VSegments", v_segments),
            ):
                blocker = QtCore.QSignalBlocker(self.session.parameter_widgets[name])
                self.session.parameter_widgets[name].setValue(value)
                del blocker
            return
        if u_segments > int(self.session.obj.USegments):
            self.session._increase_segments("USegments", u_segments)
        if v_segments > int(self.session.obj.VSegments):
            self.session._increase_segments("VSegments", v_segments)

    def _deferred_clear_tool_selection(self):
        if not self.session.cleaned and self.session.surface_tool_active:
            self.session._clear_editor_selection(clear_preselection=True)

    def _restore_surface_tool_cursor(self):
        """Reapply the active topology tool cursor after viewer event routing."""
        if (
            self.session.cleaned
            or not self.session.surface_tool_active
            or not self.surface_tool_cursor_icon
            or not hasattr(self.session.view, "activateToolHandler")
        ):
            return
        try:
            self.session.view.activateToolHandler(self.surface_tool_cursor_icon)
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
            self.session._later(delay, self._finish_surface_tool_click)
