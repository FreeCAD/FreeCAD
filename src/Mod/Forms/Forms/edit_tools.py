# SPDX-License-Identifier: LGPL-2.1-or-later
"""Compatibility facade for independently owned editor tools.

Commands and existing callers keep their session API. Each tool owns its data
and callbacks and reaches shared selection/transaction services through a weak
session reference; a retained tool callback cannot retain the editor itself.
"""

import FreeCAD as App
from pivy import coin
from .placement import global_placement
from .tool_controller import ToolField, ToolMethod
from .tool_straighten import StraightenTool
from .tool_flatten import FlattenTool
from .tool_weld import WeldTool
from .tool_match import MatchTool
from .tool_thicken import ThickenTool
from .tool_pivot import PivotTool
from .tool_topology import TopologyTool


class FormEditToolsMixin:
    """Forward the historical session API to per-tool controllers."""

    _tool_types = {
        "straighten": StraightenTool,
        "flatten": FlattenTool,
        "weld": WeldTool,
        "match": MatchTool,
        "thicken": ThickenTool,
        "pivot": PivotTool,
        "topology": TopologyTool,
    }

    def _tool(self, name):
        if "_tools" not in self.__dict__:
            self._tools = {}
        if name not in self._tools:
            self._tools[name] = self._tool_types[name](self)
        return self._tools[name]

    # Straighten
    straighten_apply_button = ToolField("straighten", "straighten_apply_button")
    straighten_cancel_button = ToolField("straighten", "straighten_cancel_button")
    straighten_indices = ToolField("straighten", "straighten_indices")
    straighten_mode = ToolField("straighten", "straighten_mode")
    straighten_preview_root = ToolField("straighten", "straighten_preview_root")
    straighten_preview_shape = ToolField("straighten", "straighten_preview_shape")
    straighten_preview_status = ToolField("straighten", "straighten_preview_status")
    straighten_preview_switch = ToolField("straighten", "straighten_preview_switch")
    straighten_range = ToolField("straighten", "straighten_range")
    straighten_reference_button = ToolField("straighten", "straighten_reference_button")
    straighten_reference_name = ToolField("straighten", "straighten_reference_name")
    straighten_reference_selecting = ToolField("straighten", "straighten_reference_selecting")
    straighten_reference_widget = ToolField("straighten", "straighten_reference_widget")
    straighten_references = ToolField("straighten", "straighten_references")
    straighten_type = ToolField("straighten", "straighten_type")
    straighten_visibility_before = ToolField("straighten", "straighten_visibility_before")
    start_straighten_tool = ToolMethod("straighten", "start_straighten_tool")
    _straighten_mode_changed = ToolMethod("straighten", "_straighten_mode_changed")
    _toggle_straighten_reference_selection = ToolMethod("straighten", "_toggle_straighten_reference_selection")
    _straighten_reference_element = ToolMethod("straighten", "_straighten_reference_element")
    _add_straighten_reference = ToolMethod("straighten", "_add_straighten_reference")
    _straighten_reference_line = ToolMethod("straighten", "_straighten_reference_line")
    _straighten_target_indices = ToolMethod("straighten", "_straighten_target_indices")
    _start_straighten_preview_visibility = ToolMethod("straighten", "_start_straighten_preview_visibility")
    _restore_straighten_preview_visibility = ToolMethod("straighten", "_restore_straighten_preview_visibility")
    _clear_straighten_preview = ToolMethod("straighten", "_clear_straighten_preview")
    _update_straighten_preview = ToolMethod("straighten", "_update_straighten_preview")
    apply_straighten_tool = ToolMethod("straighten", "apply_straighten_tool")
    stop_straighten_tool = ToolMethod("straighten", "stop_straighten_tool")

    # Flatten
    flatten_apply_button = ToolField("flatten", "flatten_apply_button")
    flatten_cancel_button = ToolField("flatten", "flatten_cancel_button")
    flatten_indices = ToolField("flatten", "flatten_indices")
    flatten_mode = ToolField("flatten", "flatten_mode")
    flatten_preview_root = ToolField("flatten", "flatten_preview_root")
    flatten_preview_shape = ToolField("flatten", "flatten_preview_shape")
    flatten_preview_status = ToolField("flatten", "flatten_preview_status")
    flatten_preview_switch = ToolField("flatten", "flatten_preview_switch")
    flatten_reference = ToolField("flatten", "flatten_reference")
    flatten_reference_button = ToolField("flatten", "flatten_reference_button")
    flatten_reference_name = ToolField("flatten", "flatten_reference_name")
    flatten_reference_selecting = ToolField("flatten", "flatten_reference_selecting")
    flatten_reference_widget = ToolField("flatten", "flatten_reference_widget")
    flatten_visibility_before = ToolField("flatten", "flatten_visibility_before")
    start_flatten_tool = ToolMethod("flatten", "start_flatten_tool")
    _flatten_mode_changed = ToolMethod("flatten", "_flatten_mode_changed")
    _toggle_flatten_reference_selection = ToolMethod("flatten", "_toggle_flatten_reference_selection")
    _flatten_reference_from_event = ToolMethod("flatten", "_flatten_reference_from_event")
    _set_flatten_reference_from_selection = ToolMethod("flatten", "_set_flatten_reference_from_selection")
    _flatten_plane = ToolMethod("flatten", "_flatten_plane")
    _start_flatten_preview_visibility = ToolMethod("flatten", "_start_flatten_preview_visibility")
    _restore_flatten_preview_visibility = ToolMethod("flatten", "_restore_flatten_preview_visibility")
    _set_flatten_preview_shape = ToolMethod("flatten", "_set_flatten_preview_shape")
    _clear_flatten_preview = ToolMethod("flatten", "_clear_flatten_preview")
    _update_flatten_preview = ToolMethod("flatten", "_update_flatten_preview")
    apply_flatten_tool = ToolMethod("flatten", "apply_flatten_tool")
    stop_flatten_tool = ToolMethod("flatten", "stop_flatten_tool")

    # Weld
    weld_apply_button = ToolField("weld", "weld_apply_button")
    weld_cancel_button = ToolField("weld", "weld_cancel_button")
    weld_first_edge = ToolField("weld", "weld_first_edge")
    weld_first_name = ToolField("weld", "weld_first_name")
    weld_other = ToolField("weld", "weld_other")
    weld_other_button = ToolField("weld", "weld_other_button")
    weld_other_name = ToolField("weld", "weld_other_name")
    weld_second_edge = ToolField("weld", "weld_second_edge")
    weld_second_name = ToolField("weld", "weld_second_name")
    weld_selecting_other = ToolField("weld", "weld_selecting_other")
    weld_status = ToolField("weld", "weld_status")
    start_weld_tool = ToolMethod("weld", "start_weld_tool")
    _toggle_weld_other_selection = ToolMethod("weld", "_toggle_weld_other_selection")
    _weld_edge_from_event = ToolMethod("weld", "_weld_edge_from_event")
    _set_weld_input_from_selection = ToolMethod("weld", "_set_weld_input_from_selection")
    apply_weld_tool = ToolMethod("weld", "apply_weld_tool")
    stop_weld_tool = ToolMethod("weld", "stop_weld_tool")

    # Match
    match_apply_button = ToolField("match", "match_apply_button")
    match_cancel_button = ToolField("match", "match_cancel_button")
    match_inputs = ToolField("match", "match_inputs")
    match_mode = ToolField("match", "match_mode")
    match_preview_root = ToolField("match", "match_preview_root")
    match_preview_shape = ToolField("match", "match_preview_shape")
    match_preview_status = ToolField("match", "match_preview_status")
    match_preview_switch = ToolField("match", "match_preview_switch")
    match_visibility_before = ToolField("match", "match_visibility_before")
    start_match_tool = ToolMethod("match", "start_match_tool")
    _start_match_preview_visibility = ToolMethod("match", "_start_match_preview_visibility")
    _restore_match_preview_visibility = ToolMethod("match", "_restore_match_preview_visibility")
    _set_match_preview_shape = ToolMethod("match", "_set_match_preview_shape")
    _clear_match_preview = ToolMethod("match", "_clear_match_preview")
    _update_match_preview = ToolMethod("match", "_update_match_preview")
    apply_match_tool = ToolMethod("match", "apply_match_tool")
    stop_match_tool = ToolMethod("match", "stop_match_tool")

    # Thicken
    thicken_apply_button = ToolField("thicken", "thicken_apply_button")
    thicken_cancel_button = ToolField("thicken", "thicken_cancel_button")
    thicken_distance = ToolField("thicken", "thicken_distance")
    thicken_original_cage = ToolField("thicken", "thicken_original_cage")
    thicken_original_mode = ToolField("thicken", "thicken_original_mode")
    thicken_transaction_open = ToolField("thicken", "thicken_transaction_open")
    thicken_update_timer = ToolField("thicken", "thicken_update_timer")
    start_thicken_tool = ToolMethod("thicken", "start_thicken_tool")
    _create_thicken_tool_widget = ToolMethod("thicken", "_create_thicken_tool_widget")
    _reverse_thicken_direction = ToolMethod("thicken", "_reverse_thicken_direction")
    _queue_thicken_preview = ToolMethod("thicken", "_queue_thicken_preview")
    _apply_thicken_preview = ToolMethod("thicken", "_apply_thicken_preview")
    stop_thicken_tool = ToolMethod("thicken", "stop_thicken_tool")

    # Pivot
    pivot_pick_pending = ToolField("pivot", "pivot_pick_pending")
    pivot_previous_selection_filter = ToolField("pivot", "pivot_previous_selection_filter")
    pivot_selection_snapshot = ToolField("pivot", "pivot_selection_snapshot")
    pivot_snap_point = ToolField("pivot", "pivot_snap_point")
    pivot_tool_callback = ToolField("pivot", "pivot_tool_callback")
    pivot_tool_mouse_callback = ToolField("pivot", "pivot_tool_mouse_callback")
    start_set_pivot_tool = ToolMethod("pivot", "start_set_pivot_tool")
    stop_set_pivot_tool = ToolMethod("pivot", "stop_set_pivot_tool")
    _snap_pivot_point = ToolMethod("pivot", "_snap_pivot_point")
    _pivot_selection_added = ToolMethod("pivot", "_pivot_selection_added")
    _restore_pivot_selection = ToolMethod("pivot", "_restore_pivot_selection")
    _complete_pivot_pick = ToolMethod("pivot", "_complete_pivot_pick")
    _pivot_tool_event = ToolMethod("pivot", "_pivot_tool_event")
    _pivot_tool_mouse_event = ToolMethod("pivot", "_pivot_tool_mouse_event")

    # Topology
    insert_orientation = ToolField("topology", "insert_orientation")
    insert_point_chain = ToolField("topology", "insert_point_chain")
    insert_point_hover = ToolField("topology", "insert_point_hover")
    insert_whole_loop = ToolField("topology", "insert_whole_loop")
    subdivide_last_counts = ToolField("topology", "subdivide_last_counts")
    subdivide_u = ToolField("topology", "subdivide_u")
    subdivide_v = ToolField("topology", "subdivide_v")
    surface_cursor_position = ToolField("topology", "surface_cursor_position")
    surface_hover_face = ToolField("topology", "surface_hover_face")
    surface_preview_coordinates = ToolField("topology", "surface_preview_coordinates")
    surface_preview_key = ToolField("topology", "surface_preview_key")
    surface_preview_lines = ToolField("topology", "surface_preview_lines")
    surface_preview_switch = ToolField("topology", "surface_preview_switch")
    surface_tool_cache_mapper = ToolField("topology", "surface_tool_cache_mapper")
    surface_tool_callback = ToolField("topology", "surface_tool_callback")
    surface_tool_control_points = ToolField("topology", "surface_tool_control_points")
    surface_tool_cursor_icon = ToolField("topology", "surface_tool_cursor_icon")
    surface_tool_hover_faces = ToolField("topology", "surface_tool_hover_faces")
    surface_tool_mouse_callback = ToolField("topology", "surface_tool_mouse_callback")
    surface_tool_shape_faces = ToolField("topology", "surface_tool_shape_faces")
    unweld_hover_edge = ToolField("topology", "unweld_hover_edge")
    unweld_segment_edges = ToolField("topology", "unweld_segment_edges")
    unweld_separate_forms = ToolField("topology", "unweld_separate_forms")
    start_insert_edge_tool = ToolMethod("topology", "start_insert_edge_tool")
    start_subdivide_tool = ToolMethod("topology", "start_subdivide_tool")
    start_insert_point_tool = ToolMethod("topology", "start_insert_point_tool")
    start_unweld_tool = ToolMethod("topology", "start_unweld_tool")
    _start_surface_tool = ToolMethod("topology", "_start_surface_tool")
    stop_insert_edge_tool = ToolMethod("topology", "stop_insert_edge_tool")
    stop_subdivide_tool = ToolMethod("topology", "stop_subdivide_tool")
    stop_surface_tool = ToolMethod("topology", "stop_surface_tool")
    _insert_loop_changed = ToolMethod("topology", "_insert_loop_changed")
    _create_surface_preview = ToolMethod("topology", "_create_surface_preview")
    _clear_surface_preview = ToolMethod("topology", "_clear_surface_preview")
    _reset_surface_tool_cache = ToolMethod("topology", "_reset_surface_tool_cache")
    _ensure_surface_tool_cache = ToolMethod("topology", "_ensure_surface_tool_cache")
    _set_surface_preview_curves = ToolMethod("topology", "_set_surface_preview_curves")
    _show_surface_tool_hints = ToolMethod("topology", "_show_surface_tool_hints")
    _subdivision_count_changed = ToolMethod("topology", "_subdivision_count_changed")
    toggle_surface_tool_orientation = ToolMethod("topology", "toggle_surface_tool_orientation")
    _surface_tool_event = ToolMethod("topology", "_surface_tool_event")
    _surface_tool_mouse_event = ToolMethod("topology", "_surface_tool_mouse_event")
    _hovered_insert_point = ToolMethod("topology", "_hovered_insert_point")
    _update_insert_point_preview = ToolMethod("topology", "_update_insert_point_preview")
    _update_unweld_preview = ToolMethod("topology", "_update_unweld_preview")
    _commit_unweld_preview = ToolMethod("topology", "_commit_unweld_preview")
    _append_insert_point = ToolMethod("topology", "_append_insert_point")
    _commit_insert_point_chain = ToolMethod("topology", "_commit_insert_point_chain")
    _hovered_control_face = ToolMethod("topology", "_hovered_control_face")
    _shape_face_for_control_face = ToolMethod("topology", "_shape_face_for_control_face")
    _insert_curve_points = ToolMethod("topology", "_insert_curve_points")
    _update_insert_preview = ToolMethod("topology", "_update_insert_preview")
    _update_subdivide_preview = ToolMethod("topology", "_update_subdivide_preview")
    _update_surface_tool_preview = ToolMethod("topology", "_update_surface_tool_preview")
    _commit_surface_tool_preview = ToolMethod("topology", "_commit_surface_tool_preview")
    _commit_insert_preview = ToolMethod("topology", "_commit_insert_preview")
    _commit_subdivide_preview = ToolMethod("topology", "_commit_subdivide_preview")
    _set_form_surface_segments = ToolMethod("topology", "_set_form_surface_segments")
    _deferred_clear_tool_selection = ToolMethod("topology", "_deferred_clear_tool_selection")
    _restore_surface_tool_cursor = ToolMethod("topology", "_restore_surface_tool_cursor")
    _finish_surface_tool_click = ToolMethod("topology", "_finish_surface_tool_click")
    _queue_tool_selection_clear = ToolMethod("topology", "_queue_tool_selection_clear")

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


    @staticmethod
    def _selection_document(document):
        return document if hasattr(document, "getObject") else App.getDocument(str(document))


    @staticmethod
    def _global_placement(obj):
        return global_placement(obj)


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
