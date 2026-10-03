# SPDX-License-Identifier: LGPL-2.1-or-later

import math
import unittest

from Forms.brep import ConversionError
from Forms.cage import ControlCage, canonical_subelement_name
from Forms.feedback import MODELING_ERRORS, report_modeling_error
from Forms.symmetry import control_pairs
from Forms.tmesh import DyadicTMesh
from Forms.topology import (
    box_control_cage,
    cage_edge_loop,
    cage_edge_selection_range,
    cage_edges,
    cage_face_selection_range,
    cage_vertex_range,
    cage_vertex_selection_range,
    catmull_clark,
    catmull_clark_limit_points,
    catmull_clark_patch_grids,
    catmull_clark_step,
    catmull_clark_step_details,
    connected_edge_component,
    cylinder_control_cage,
    face_control_cage,
    flatten_points,
    quadball_control_cage,
    resize_structured_cage,
    sphere_control_cage,
    straighten_points,
    torus_control_cage,
    tube_control_cage,
)


class FeedbackTest(unittest.TestCase):
    def test_modeling_errors_exclude_unexpected_runtime_failures(self):
        self.assertIsInstance(ConversionError("invalid cage"), MODELING_ERRORS)
        self.assertIsInstance(ValueError("invalid input"), MODELING_ERRORS)
        self.assertNotIsInstance(RuntimeError("programming failure"), MODELING_ERRORS)

    def test_modeling_error_reports_to_tool_status_without_raising(self):
        class Status:
            text = ""

            def setText(self, value):
                self.text = value

        status = Status()
        result = report_modeling_error("Forms test", ValueError("invalid selection"), status)
        self.assertFalse(result)
        self.assertEqual(status.text, "invalid selection")


class GeometryEditingTest(unittest.TestCase):
    def test_mapped_subelement_uses_the_current_terminal_token(self):
        self.assertEqual(
            canonical_subelement_name("Form.;Face1;:C17;:H:5,F.Face24"),
            "Face24",
        )

    def test_connected_edge_component_keeps_hole_wires_separate(self):
        first = {(0, 1), (1, 2), (2, 3), (0, 3)}
        second = {(4, 5), (5, 6), (6, 7), (4, 7)}

        self.assertEqual(
            set(connected_edge_component(first | second, (1, 2))),
            first,
        )

    def test_unweld_and_weld_are_inverse_topology_operations(self):
        vertices, faces = cylinder_control_cage(10, 20, 2, 3)
        cage = ControlCage(vertices, faces)
        seam = None
        for edge in cage_edges(faces):
            candidate = cage_edge_loop(faces, edge)
            try:
                cage.split_along_edges(candidate)
            except ValueError:
                continue
            seam = candidate
            break
        self.assertIsNotNone(seam)

        first, second = cage.split_along_edges(seam)

        self.assertEqual(len(first.faces) + len(second.faces), len(cage.faces))
        self.assertTrue(first.boundary_edges)
        self.assertTrue(second.boundary_edges)
        first_edge = first.boundary_edges[0]
        second_edge = second.boundary_edges[0]
        welded = first.weld_boundary(second, first_edge, second_edge)
        self.assertEqual(len(welded.faces), len(cage.faces))
        self.assertEqual(len(welded.vertices), len(cage.vertices))
        self.assertTrue(welded.is_closed)

    def test_unweld_rejects_a_nonseparating_segment(self):
        vertices, faces = face_control_cage(20, 10, 2, 2)
        cage = ControlCage(vertices, faces)
        with self.assertRaisesRegex(ValueError, "internal control segment"):
            cage.split_along_edges([(0, 1)])

    def test_straighten_fits_arbitrary_selected_points(self):
        vertices = [(0, 0, 0), (1, 0.2, 0), (2, -0.1, 0), (3, 0, 0)]
        result = straighten_points(vertices, range(4))
        first = result[0]
        direction = tuple(result[-1][axis] - first[axis] for axis in range(3))
        for point in result[1:-1]:
            cross = (
                direction[1] * (point[2] - first[2])
                - direction[2] * (point[1] - first[1]),
                direction[2] * (point[0] - first[0])
                - direction[0] * (point[2] - first[2]),
                direction[0] * (point[1] - first[1])
                - direction[1] * (point[0] - first[0]),
            )
            self.assertAlmostEqual(sum(value * value for value in cross), 0.0)

    def test_straighten_uses_explicit_and_parallel_lines(self):
        vertices = [(0, 2, 0), (2, 4, 0), (4, 6, 0)]
        explicit = straighten_points(vertices, range(3), ((0, 0, 0), (1, 0, 0)))
        parallel = straighten_points(vertices, range(3), (None, (1, 0, 0)))
        self.assertEqual([point[1] for point in explicit], [0.0] * 3)
        self.assertEqual([point[1] for point in parallel], [4.0] * 3)

    def test_range_selection_uses_unique_shortest_cage_path(self):
        _vertices, faces = face_control_cage(10, 10, 3, 1)
        self.assertEqual(cage_vertex_range(faces, 0, 3), [0, 1, 2, 3])

    def test_shift_vertex_range_includes_all_shortest_grid_routes(self):
        _vertices, faces = face_control_cage(10, 10, 2, 2)
        self.assertEqual(
            cage_vertex_selection_range(faces, 0, 8),
            set(range(9)),
        )

    def test_shift_face_range_selects_a_rectangular_grid_block(self):
        _vertices, faces = face_control_cage(10, 10, 2, 2)
        self.assertEqual(
            cage_face_selection_range(faces, 0, 3),
            set(range(4)),
        )

    def test_shift_edge_range_follows_connected_edge_segments(self):
        _vertices, faces = face_control_cage(10, 10, 3, 1)
        self.assertEqual(
            cage_edge_selection_range(faces, (0, 1), (2, 3)),
            {(0, 1), (1, 2), (2, 3)},
        )

    def test_shift_ranges_follow_rows_on_a_five_by_five_surface(self):
        _vertices, faces = face_control_cage(10, 10, 5, 5)

        self.assertEqual(cage_vertex_selection_range(faces, 0, 5), set(range(6)))
        self.assertEqual(
            cage_edge_selection_range(faces, (0, 1), (4, 5)),
            {(index, index + 1) for index in range(5)},
        )
        self.assertEqual(cage_face_selection_range(faces, 0, 4), set(range(5)))

    def test_shift_range_reselection_is_deferred_outside_the_observer(self):
        from types import SimpleNamespace

        from Forms.edit import FormEditSession

        _vertices, faces = face_control_cage(10, 10, 5, 5)
        mapper = SimpleNamespace(mesh=None, logical_faces=faces)
        restored = []
        pending = []
        session = object.__new__(FormEditSession)
        session.cleaned = False
        session.last_added_edge = None
        session.selection_sync_generation = 0
        session.range_selection_generation = 0
        session.range_selection_anchors = {"Face": 0}
        session._range_selection_target = lambda _subelement: ("Face", 4, mapper)
        session._selected_control_targets = lambda respect_symmetry=False: [
            ("Face", faces[0], None)
        ]
        session._restore_control_selection = (
            lambda vertices, edges, selected_faces, defer_dragger=False: restored.append(
                (vertices, edges, selected_faces, defer_dragger)
            )
        )
        session._schedule_shift_range = (
            lambda vertices, edges, selected_faces, generation: pending.append(
                (vertices, edges, selected_faces, generation)
            )
        )

        self.assertTrue(session._extend_shift_range("Face5"))
        self.assertEqual(restored, [])
        self.assertEqual(len(pending), 1)
        session._apply_shift_range(*pending[0])
        self.assertEqual(len(restored), 1)
        self.assertEqual(restored[0][2], {frozenset(faces[index]) for index in range(5)})
        self.assertTrue(restored[0][3])

    def test_shift_range_scheduler_captures_all_arguments(self):
        from Forms.edit import FormEditSession

        applied = []
        pending = []
        session = object.__new__(FormEditSession)
        session._defer_shift_range = pending.append
        session._apply_shift_range = lambda *arguments: applied.append(arguments)
        expected = ({1, 2}, {(1, 2)}, {frozenset((0, 1, 2, 3))}, 7)

        session._schedule_shift_range(*expected)
        self.assertEqual(applied, [])
        self.assertEqual(len(pending), 1)
        pending[0]()
        self.assertEqual(applied, [expected])

    def test_face_deletion_rejects_boundaries_touching_at_one_vertex(self):
        vertices, faces = face_control_cage(10, 10, 5, 5)
        cage = ControlCage(vertices, faces).delete_faces([0])

        with self.assertRaisesRegex(ValueError, "boundaries meeting at a vertex"):
            cage.delete_faces([5])

    def test_flatten_projects_to_best_fit_plane(self):
        vertices = [(0, 0, 0.0), (2, 0, 0.3), (2, 2, -0.2), (0, 2, 0.1)]
        result = flatten_points(vertices, range(4))
        first = result[0]
        ab = tuple(result[1][axis] - first[axis] for axis in range(3))
        ac = tuple(result[2][axis] - first[axis] for axis in range(3))
        ad = tuple(result[3][axis] - first[axis] for axis in range(3))
        normal = (
            ab[1] * ac[2] - ab[2] * ac[1],
            ab[2] * ac[0] - ab[0] * ac[2],
            ab[0] * ac[1] - ab[1] * ac[0],
        )
        self.assertAlmostEqual(sum(normal[axis] * ad[axis] for axis in range(3)), 0.0)

    def test_flatten_projects_to_explicit_plane(self):
        vertices = [(0, 0, 3), (1, 0, -2), (1, 1, 5), (0, 1, 8)]
        result = flatten_points(vertices, range(4), ((0, 0, 1), (0, 0, 2)))
        self.assertEqual([point[2] for point in result], [1.0] * 4)

    def test_flatten_parallel_plane_uses_selected_centroid(self):
        vertices = [(0, 0, 3), (1, 0, -1), (1, 1, 5), (0, 1, 1)]
        result = flatten_points(vertices, range(4), (None, (0, 0, 1)))
        self.assertEqual([point[2] for point in result], [2.0] * 4)


class SymmetryTest(unittest.TestCase):
    def test_non_strict_pairing_recovers_an_asymmetric_edited_shape(self):
        vertices = {0: (3.0, 0.0, 0.0), 1: (-2.0, 0.0, 0.0)}

        with self.assertRaises(ValueError):
            control_pairs(vertices, 0)

        pairs, plane_points = control_pairs(vertices, 0, strict=False)
        self.assertEqual(pairs, [(0, 1)])
        self.assertEqual(plane_points, [])


class BoxTopologyTest(unittest.TestCase):
    def test_unit_box_has_eight_vertices_and_six_quads(self):
        vertices, faces = box_control_cage(10, 20, 30)
        self.assertEqual(len(vertices), 8)
        self.assertEqual(len(faces), 6)
        self.assertEqual(len(cage_edges(faces)), 12)

    def test_segments_share_boundary_vertices(self):
        vertices, faces = box_control_cage(10, 20, 30, 2, 3, 4)
        expected_vertices = (3 * 4 * 5) - (1 * 2 * 3)
        expected_faces = 2 * (2 * 3 + 2 * 4 + 3 * 4)
        self.assertEqual(len(vertices), expected_vertices)
        self.assertEqual(len(faces), expected_faces)

    def test_face_winding_points_outward(self):
        vertices, faces = box_control_cage(10, 20, 30, 2, 2, 2)
        center = (0.0, 0.0, 0.0)

        for face in faces:
            points = [vertices[index] for index in face]
            edge_a = tuple(points[1][axis] - points[0][axis] for axis in range(3))
            edge_b = tuple(points[2][axis] - points[0][axis] for axis in range(3))
            normal = (
                edge_a[1] * edge_b[2] - edge_a[2] * edge_b[1],
                edge_a[2] * edge_b[0] - edge_a[0] * edge_b[2],
                edge_a[0] * edge_b[1] - edge_a[1] * edge_b[0],
            )
            face_center = tuple(sum(point[axis] for point in points) / 4 for axis in range(3))
            outward = tuple(face_center[axis] - center[axis] for axis in range(3))
            self.assertGreater(sum(normal[axis] * outward[axis] for axis in range(3)), 0)

    def test_invalid_inputs_are_rejected(self):
        with self.assertRaises(ValueError):
            box_control_cage(0, 10, 10)
        with self.assertRaises(ValueError):
            box_control_cage(10, 10, 10, 0, 1, 1)

    def test_segmented_cube_edge_loop_is_one_continuous_wire(self):
        _vertices, faces = box_control_cage(10, 10, 10, 2, 2, 2)
        loops = [cage_edge_loop(faces, edge) for edge in cage_edges(faces)]
        loop = max(loops, key=len)
        start = loop[0]
        self.assertGreater(len(loop), 1)
        loop = cage_edge_loop(faces, start)
        self.assertIn(start, loop)
        degrees = {}
        for edge in loop:
            for vertex in edge:
                degrees[vertex] = degrees.get(vertex, 0) + 1
        self.assertTrue(all(degree <= 2 for degree in degrees.values()))
        visited = {loop[0]}
        pending = [loop[0]]
        while pending:
            edge = pending.pop()
            connected = [
                candidate
                for candidate in loop
                if candidate not in visited and set(candidate).intersection(edge)
            ]
            visited.update(connected)
            pending.extend(connected)
        self.assertEqual(visited, set(loop))


class PrimitiveTopologyTest(unittest.TestCase):
    def test_face_is_an_open_quad_grid(self):
        vertices, faces = face_control_cage(20, 10, 2, 3)
        self.assertEqual(len(vertices), 12)
        self.assertEqual(len(faces), 6)
        self.assertEqual(len(cage_edges(faces)), 17)

    def test_face_boundary_loop_stops_at_open_grid_corners(self):
        _vertices, faces = face_control_cage(20, 10, 2, 3)
        self.assertEqual(set(cage_edge_loop(faces, (0, 1))), {(0, 1), (1, 2)})

    def test_cylinder_is_closed_and_all_quad(self):
        vertices, faces = cylinder_control_cage(10, 20, 2, 2)
        edge_counts = {}
        for face in faces:
            self.assertEqual(len(face), 4)
            for position, start in enumerate(face):
                edge = tuple(sorted((start, face[(position + 1) % 4])))
                edge_counts[edge] = edge_counts.get(edge, 0) + 1
        self.assertTrue(vertices)
        self.assertTrue(all(count == 2 for count in edge_counts.values()))

    def test_sphere_points_are_on_requested_radius(self):
        vertices, faces = sphere_control_cage(12, 2, 3)
        self.assertTrue(faces)
        for point in vertices:
            length = sum(component * component for component in point) ** 0.5
            self.assertAlmostEqual(length, 12.0, places=7)

    def test_sphere_has_single_north_and_south_pole(self):
        vertices, faces = sphere_control_cage(12, 2, 2)
        north = [index for index, point in enumerate(vertices) if point == (0.0, 0.0, 12.0)]
        south = [index for index, point in enumerate(vertices) if point == (0.0, 0.0, -12.0)]
        self.assertEqual(north, [0])
        self.assertEqual(south, [len(vertices) - 1])
        self.assertEqual(sum(north[0] in face for face in faces), 8)
        self.assertEqual(sum(south[0] in face for face in faces), 8)
        self.assertTrue(all(len(face) == 4 and len(set(face)) == 4 for face in faces))

        edge_counts = {}
        for face in faces:
            for position, start in enumerate(face):
                edge = tuple(sorted((start, face[(position + 1) % 4])))
                edge_counts[edge] = edge_counts.get(edge, 0) + 1
        self.assertTrue(all(count == 2 for count in edge_counts.values()))

    def test_quadball_retains_cube_derived_topology(self):
        quad_vertices, quad_faces = quadball_control_cage(12, 2)
        sphere_vertices, sphere_faces = sphere_control_cage(12, 2, 2)
        self.assertTrue(all(len(face) == 4 for face in quad_faces + sphere_faces))
        self.assertNotEqual((len(quad_vertices), len(quad_faces)),
                            (len(sphere_vertices), len(sphere_faces)))


class StructuredResizeTest(unittest.TestCase):
    def test_box_adds_rows_in_only_the_requested_axis(self):
        vertices, _faces = box_control_cage(2, 2, 2, 2, 2, 2)
        vertices[0] = (vertices[0][0], vertices[0][1], vertices[0][2] - 3.0)
        resized, faces = resize_structured_cage(vertices, (2, 2, 2), (3, 2, 2))
        self.assertEqual(len(resized), 34)
        self.assertEqual(len(faces), 32)
        self.assertIn(vertices[0], resized)

    def test_face_can_add_rows_independently(self):
        vertices, _faces = face_control_cage(2, 2, 2, 2)
        resized, faces = resize_structured_cage(vertices, (2, 2), (2, 4), surface=True)
        self.assertEqual(len(resized), 15)
        self.assertEqual(len(faces), 8)

    def test_segment_removal_is_rejected(self):
        vertices, _faces = box_control_cage(2, 2, 2, 2, 2, 2)
        with self.assertRaises(ValueError):
            resize_structured_cage(vertices, (2, 2, 2), (1, 2, 2))

    def test_added_rows_preserve_vertex_and_edge_sharpness(self):
        vertices, faces = box_control_cage(2, 2, 2, 2, 2, 2)
        edge = cage_edges(faces)[0]
        vertex_values = [0.0] * len(vertices)
        vertex_values[edge[0]] = 4.0
        resized, resized_faces, new_vertices, new_edges = resize_structured_cage(
            vertices,
            (2, 2, 2),
            (3, 2, 2),
            vertex_sharpness=vertex_values,
            edge_sharpness={edge: 6.0},
            return_sharpness=True,
        )
        self.assertEqual(len(resized), len(new_vertices))
        self.assertEqual(max(new_vertices), 4.0)
        self.assertTrue(set(new_edges).issubset(set(cage_edges(resized_faces))))
        self.assertEqual(max(new_edges.values()), 6.0)


class CageEditingTest(unittest.TestCase):
    def test_thicken_adds_two_editable_skins_and_minimal_boundary_walls(self):
        vertices, faces = face_control_cage(20, 10, 2, 2)
        source = ControlCage(vertices, faces)
        boundary_count = len(source.boundary_edges)

        thickened = source.thickened(2.0, sharp=True)

        self.assertEqual(len(thickened.vertices), len(vertices) * 2)
        self.assertEqual(len(thickened.faces), len(faces) * 2 + boundary_count)
        self.assertTrue(thickened.is_closed)
        self.assertTrue(all(len(face) == 4 for face in thickened.faces))
        for index in range(len(vertices)):
            self.assertAlmostEqual(
                thickened.vertices[index + len(vertices)][2] - thickened.vertices[index][2],
                2.0,
            )
        for edge in source.boundary_edges:
            duplicate = tuple(sorted((edge[0] + len(vertices), edge[1] + len(vertices))))
            self.assertEqual(thickened.edge_sharpness[edge], 10.0)
            self.assertEqual(thickened.edge_sharpness[duplicate], 10.0)

    def test_thicken_supports_negative_distance_and_rejects_closed_cages(self):
        vertices, faces = face_control_cage(20, 10, 1, 1)
        thickened = ControlCage(vertices, faces).thickened(-3.0)
        self.assertTrue(thickened.is_closed)
        self.assertAlmostEqual(min(point[2] for point in thickened.vertices), -3.0)

        box_vertices, box_faces = box_control_cage(10, 10, 10)
        with self.assertRaisesRegex(ValueError, "open Form surface"):
            ControlCage(box_vertices, box_faces).thickened(2.0)

    def test_bridge_connects_two_quad_boundaries_with_minimal_strip(self):
        vertices = [
            (-1, -1, -1),
            (1, -1, -1),
            (1, 1, -1),
            (-1, 1, -1),
            (-1, -1, 1),
            (1, -1, 1),
            (1, 1, 1),
            (-1, 1, 1),
        ]
        cage = ControlCage(vertices, [(0, 3, 2, 1), (4, 5, 6, 7)])
        loops = cage.boundary_loops()
        selected = [
            tuple(sorted((loops[0][0], loops[0][1]))),
            tuple(sorted((loops[1][0], loops[1][1]))),
        ]

        bridged = cage.bridge_boundaries(selected)

        self.assertEqual(len(bridged.faces), 6)
        self.assertTrue(bridged.is_closed)
        self.assertTrue(all(len(face) == 4 for face in bridged.faces))

    def test_erase_extrusion_restores_the_original_minimal_cage(self):
        vertices, faces = box_control_cage(10, 10, 10)
        original = ControlCage(vertices, faces)
        extruded, _top, sides = original.extrude_face(0)

        restored = extruded.erase_and_fill((0,) + sides)

        self.assertEqual(len(restored.vertices), len(original.vertices))
        self.assertEqual(len(restored.faces), len(original.faces))
        self.assertTrue(restored.is_closed)
        self.assertEqual(
            {frozenset(face) for face in restored.faces},
            {frozenset(face) for face in original.faces},
        )

    def test_minimal_fill_uses_two_quads_for_a_six_edge_boundary(self):
        vertices, faces = box_control_cage(20, 20, 20, 2, 2, 2)
        cage = ControlCage(vertices, faces).delete_faces([0, 2])

        filled = cage.fill_boundaries([cage.boundary_edges[0]], mode="minimal")

        self.assertEqual(len(filled.faces), len(cage.faces) + 2)
        self.assertEqual(len(filled.vertices), len(cage.vertices))
        self.assertTrue(filled.is_closed)

    def test_edge_collapse_merges_controls_and_preserves_a_closed_cage(self):
        vertices, faces = box_control_cage(10, 10, 10)
        edge = cage_edges(faces)[0]
        vertex_values = [0.0] * len(vertices)
        vertex_values[edge[0]] = 2.0
        vertex_values[edge[1]] = 5.0
        cage = ControlCage(
            vertices,
            faces,
            vertex_values,
            {edge: 7.0},
        )
        midpoint = tuple(
            (vertices[edge[0]][axis] + vertices[edge[1]][axis]) * 0.5 for axis in range(3)
        )

        collapsed = cage.merge_vertices(edge)

        self.assertEqual(len(collapsed.vertices), len(cage.vertices) - 1)
        self.assertEqual(collapsed.vertices[min(edge)], midpoint)
        self.assertEqual(collapsed.vertex_sharpness[min(edge)], 5.0)
        self.assertTrue(collapsed.is_closed)
        self.assertTrue(all(len(face) >= 3 for face in collapsed.faces))
        self.assertTrue(set(collapsed.edge_sharpness).issubset(set(cage_edges(collapsed.faces))))

    def test_face_extrusion_adds_only_one_quad_ring(self):
        vertices, faces = box_control_cage(10, 10, 10)
        cage = ControlCage(vertices, faces)

        extruded, top, side_faces = cage.extrude_face(0)

        self.assertEqual(len(extruded.vertices), len(vertices) + 4)
        self.assertEqual(len(extruded.faces), len(faces) + 4)
        self.assertEqual(extruded.faces[0], top)
        self.assertEqual(len(side_faces), 4)
        self.assertTrue(extruded.is_closed)
        self.assertTrue(all(len(face) == 4 for face in extruded.faces))

    def test_adjacent_face_extrusion_adds_only_the_region_perimeter(self):
        vertices, faces = face_control_cage(20, 10, 2, 1)
        cage = ControlCage(vertices, faces)

        extruded, tops, side_faces = cage.extrude_faces({0, 1})

        self.assertEqual(len(tops), 2)
        self.assertEqual(len(side_faces), 6)
        self.assertEqual(len(extruded.vertices), len(vertices) + 6)
        self.assertEqual(len(extruded.faces), len(faces) + 6)
        self.assertEqual(extruded.faces[0], tops[0])
        self.assertEqual(extruded.faces[1], tops[1])
        self.assertFalse(extruded.is_closed)
        self.assertTrue(all(len(face) == 4 for face in extruded.faces))

    def test_face_extrusion_rejects_disconnected_regions(self):
        vertices, faces = face_control_cage(30, 10, 3, 1)
        cage = ControlCage(vertices, faces)

        with self.assertRaisesRegex(ValueError, "edge-connected"):
            cage.extrude_faces({0, 2})

    def test_boundary_edge_extrusion_adds_one_minimal_quad_strip(self):
        vertices, faces = face_control_cage(20, 20, 1, 1)
        original = ControlCage(vertices, faces)
        selected = set(original.boundary_edges[:2])

        extruded, outer_edges, side_faces = original.extrude_boundary_edges(selected)

        self.assertEqual(len(extruded.vertices), len(original.vertices) + 3)
        self.assertEqual(len(extruded.faces), len(original.faces) + 2)
        self.assertEqual(len(outer_edges), 2)
        self.assertEqual(len(side_faces), 2)
        self.assertTrue(outer_edges.issubset(set(extruded.boundary_edges)))
        self.assertFalse(extruded.is_closed)
        self.assertTrue(all(len(face) == 4 for face in extruded.faces))

    def test_repeated_face_extrusion_adds_successive_rings(self):
        vertices, faces = box_control_cage(10, 10, 10)
        first, first_top, _side_faces = ControlCage(vertices, faces).extrude_face(0)
        first_top_index = first.face_index(first_top)

        second, second_top, _side_faces = first.extrude_face(first_top_index)

        self.assertEqual(len(second.vertices), len(vertices) + 8)
        self.assertEqual(len(second.faces), len(faces) + 8)
        self.assertEqual(second.faces[first_top_index], second_top)
        self.assertTrue(second.is_closed)

    def test_face_extrusion_preserves_creases_only_when_requested(self):
        vertices, faces = box_control_cage(10, 10, 10)
        face = faces[0]
        sharp_edges = {
            tuple(sorted((face[index], face[(index + 1) % 4]))): 7.0 for index in range(4)
        }
        cage = ControlCage(vertices, faces, [3.0] * len(vertices), sharp_edges)

        smooth, smooth_top, _side_faces = cage.extrude_face(0)
        creased, creased_top, _side_faces = cage.extrude_face(0, keep_creases=True)

        self.assertTrue(all(smooth.vertex_sharpness[index] == 0.0 for index in smooth_top))
        self.assertTrue(all(creased.vertex_sharpness[index] == 3.0 for index in creased_top))
        for index in range(4):
            edge = tuple(sorted((creased_top[index], creased_top[(index + 1) % 4])))
            self.assertEqual(creased.edge_sharpness[edge], 7.0)


class DyadicTMeshTest(unittest.TestCase):
    def test_quad_cage_migrates_without_changing_ids(self):
        vertices, faces = box_control_cage(10, 10, 10)
        mesh = DyadicTMesh.from_quad_cage(vertices, faces)

        self.assertEqual(set(mesh.vertices), set(range(len(vertices))))
        self.assertEqual(set(mesh.faces), set(range(len(faces))))
        self.assertEqual([mesh.faces[index].corners for index in range(len(faces))], faces)
        self.assertTrue(mesh.is_closed)

    def test_local_insert_adds_two_independent_vertices_and_one_face(self):
        vertices, faces = box_control_cage(10, 10, 10)
        mesh = DyadicTMesh.from_quad_cage(vertices, faces)
        target = mesh.faces[0]
        selected_edge = target.sides[0]

        inserted, new_vertices, child_faces = mesh.insert_edge(0, selected_edge)

        self.assertEqual(len(mesh.vertices), len(vertices))
        self.assertEqual(len(inserted.vertices), len(vertices) + 2)
        self.assertEqual(len(inserted.faces), len(faces) + 1)
        self.assertEqual(len(inserted.atomic_edges()), len(cage_edges(faces)) + 3)
        self.assertEqual(child_faces[0], 0)
        shared = set(inserted.faces[child_faces[0]].corners).intersection(
            inserted.faces[child_faces[1]].corners
        )
        self.assertEqual(shared, set(new_vertices))
        self.assertEqual(
            sorted(len(face.t_vertices) for face in inserted.faces.values()), [0] * 5 + [1, 1]
        )
        self.assertTrue(inserted.is_closed)

        untouched = inserted.vertices[new_vertices[1]]
        inserted.set_vertex(new_vertices[0], (1.25, 2.5, 3.75))
        self.assertEqual(inserted.vertices[new_vertices[0]], (1.25, 2.5, 3.75))
        self.assertEqual(inserted.vertices[new_vertices[1]], untouched)

    def test_tmesh_serialization_preserves_stable_ids_and_sides(self):
        vertices, faces = box_control_cage(10, 10, 10)
        mesh = DyadicTMesh.from_quad_cage(vertices, faces)
        mesh, _new_vertices, _new_faces = mesh.insert_edge(0, mesh.faces[0].sides[0])

        restored = DyadicTMesh.decode(mesh.encode())

        self.assertEqual(restored.vertices, mesh.vertices)
        self.assertEqual(restored.faces, mesh.faces)
        self.assertEqual(restored.edge_intervals, mesh.edge_intervals)
        self.assertEqual(restored.next_vertex_id, mesh.next_vertex_id)
        self.assertEqual(restored.next_face_id, mesh.next_face_id)

    def test_edge_loop_continues_along_split_sides_but_not_t_branches(self):
        from Forms.topology import face_control_cage, cage_edge_loop
        vertices, faces = face_control_cage(30, 30, 3, 3)
        mesh = DyadicTMesh.from_quad_cage(vertices, faces)
        for edge in cage_edges(faces):
            self.assertEqual(mesh.edge_loop(edge), cage_edge_loop(faces, edge))
        side = mesh.faces[4].sides[1]
        original = set(mesh.edge_loop(side))
        inserted, new_vertices, _faces = mesh.insert_edge(4, mesh.faces[4].sides[0])
        middle = next(vertex for vertex in new_vertices
                      if tuple(sorted((side[0], vertex))) in inserted.atomic_edges())
        pieces = {tuple(sorted((side[0], middle))), tuple(sorted((middle, side[-1])))}
        expected = original.difference({tuple(sorted(side))}).union(pieces)
        for piece in pieces:
            self.assertEqual(set(inserted.edge_loop(piece)), expected)
        branch = tuple(sorted(new_vertices))
        self.assertEqual(inserted.edge_loop(branch), [branch])

    def test_non_dyadic_insert_is_rejected(self):
        vertices, faces = box_control_cage(10, 10, 10)
        mesh = DyadicTMesh.from_quad_cage(vertices, faces)
        with self.assertRaises(ValueError):
            mesh.insert_edge(0, mesh.faces[0].sides[0], 0.25)

    def test_leaf_can_be_split_repeatedly_without_refining_its_neighbors(self):
        vertices, faces = box_control_cage(10, 10, 10)
        mesh = DyadicTMesh.from_quad_cage(vertices, faces)
        mesh, _vertices, _faces = mesh.insert_edge(0, mesh.faces[0].sides[0])
        before_faces = set(mesh.faces)
        mesh, new_vertices, children = mesh.insert_edge(0, mesh.faces[0].sides[0])

        self.assertEqual(len(new_vertices), 2)
        self.assertEqual(set(mesh.faces).difference(before_faces), {children[1]})
        self.assertTrue(mesh.is_closed)
        self.assertEqual(max(mesh.vertex_levels.values()), 2)

    def test_subdivide_creates_four_leaves_at_one_refinement_level(self):
        vertices, faces = box_control_cage(10, 10, 10)
        mesh = DyadicTMesh.from_quad_cage(vertices, faces)

        mesh, descendants = mesh.subdivide([0])

        self.assertEqual(len(descendants), 4)
        self.assertEqual(len(mesh.faces), len(faces) + 3)
        self.assertEqual(len(mesh.vertices), len(vertices) + 5)
        self.assertEqual(max(mesh.vertex_levels.values()), 1)
        self.assertTrue(mesh.is_closed)

    def test_subdivide_refines_parameter_axes_independently(self):
        vertices, faces = box_control_cage(10, 10, 10)
        mesh = DyadicTMesh.from_quad_cage(vertices, faces)

        mesh, descendants = mesh.subdivide_grid([0], 2, 1)

        self.assertEqual(len(descendants), 8)
        self.assertEqual(len(mesh.faces), len(faces) + 7)
        self.assertEqual(max(mesh.vertex_levels.values()), 2)
        self.assertTrue(mesh.is_closed)


class CatmullClarkTest(unittest.TestCase):
    def test_closed_box_refinement_counts(self):
        vertices, faces = box_control_cage(10, 10, 10)
        vertices, faces = catmull_clark_step(vertices, faces)
        self.assertEqual(len(vertices), 26)
        self.assertEqual(len(faces), 24)
        self.assertTrue(all(len(face) == 4 for face in faces))

        vertices, faces = catmull_clark_step(vertices, faces)
        self.assertEqual(len(vertices), 98)
        self.assertEqual(len(faces), 96)

    def test_torus_is_a_closed_periodic_quad_cage(self):
        vertices, faces = torus_control_cage(15, 5, 2, 2)
        cage = ControlCage(vertices, faces)
        self.assertEqual(len(vertices), 64)
        self.assertEqual(len(faces), 64)
        self.assertTrue(cage.is_closed)
        self.assertTrue(all(len(face) == 4 for face in faces))

    def test_tube_is_closed_hollow_and_all_quad(self):
        vertices, faces = tube_control_cage(10, 6, 20, 2, 2)
        cage = ControlCage(vertices, faces)
        radial = [math.hypot(point[0], point[1]) for point in vertices]
        self.assertAlmostEqual(min(radial), 6.0)
        self.assertAlmostEqual(max(radial), 10.0)
        self.assertTrue(cage.is_closed)
        self.assertTrue(all(len(face) == 4 for face in faces))

    def test_boundary_rule_on_single_quad(self):
        vertices = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0)]
        vertices, faces = catmull_clark(vertices, [(0, 1, 2, 3)], 1)
        self.assertEqual(vertices[0], (0.125, 0.125, 0.0))
        self.assertEqual(len(vertices), 9)
        self.assertEqual(len(faces), 4)

    def test_level_zero_preserves_topology(self):
        vertices, faces = box_control_cage(10, 10, 10)
        result_vertices, result_faces = catmull_clark(vertices, faces, 0)
        self.assertEqual(result_vertices, vertices)
        self.assertEqual(result_faces, faces)

    def test_invalid_topology_is_rejected(self):
        with self.assertRaises(ValueError):
            catmull_clark([(0, 0, 0)], [(0, 1, 2)], 1)
        with self.assertRaises(ValueError):
            catmull_clark([(0, 0, 0), (1, 0, 0), (1, 1, 0)], [(0, 1, 1)], 1)

    def test_cube_limit_points_are_inset_symmetrically(self):
        vertices, faces = box_control_cage(12, 12, 12)
        limit_points = catmull_clark_limit_points(vertices, faces)
        self.assertEqual(len(limit_points), 8)
        for point in limit_points:
            self.assertTrue(all(round(value, 7) in (-3.0, 3.0) for value in point))

    def test_sharp_cube_edges_reach_the_control_corners(self):
        vertices, faces = box_control_cage(12, 12, 12)
        sharp_edges = {edge: 10.0 for edge in cage_edges(faces)}
        limit_points = catmull_clark_limit_points(vertices, faces, sharp_edges)
        self.assertEqual(limit_points, vertices)

    def test_fractional_vertex_sharpness_decays_before_evaluating_limit(self):
        vertices, faces = box_control_cage(12, 12, 12)
        values = [0.5] + [0.0] * (len(vertices) - 1)
        before = catmull_clark_limit_points(vertices, faces, vertex_sharpness=values)
        vv, ff, old, _, _, ee, cc = catmull_clark_step_details(
            vertices, faces, vertex_sharpness=values)
        after = catmull_clark_limit_points(vv, ff, ee, cc)
        for index, point in enumerate(before):
            for actual, expected in zip(point, after[old[index]]):
                self.assertAlmostEqual(actual, expected)

    def test_patch_grids_share_limit_samples(self):
        vertices, faces = box_control_cage(10, 20, 30)
        grids = catmull_clark_patch_grids(vertices, faces, 2)
        self.assertEqual(len(grids), 6)
        self.assertTrue(all(len(grid) == 5 and len(grid[0]) == 5 for grid in grids))

        sample_counts = {}
        for grid in grids:
            boundary = (
                grid[0]
                + grid[-1]
                + [row[0] for row in grid[1:-1]]
                + [row[-1] for row in grid[1:-1]]
            )
            for point in boundary:
                rounded = tuple(round(value, 9) for value in point)
                sample_counts[rounded] = sample_counts.get(rounded, 0) + 1
        self.assertTrue(all(count >= 2 for count in sample_counts.values()))


def suite():
    suite = unittest.TestSuite()
    suite.addTests(unittest.defaultTestLoader.loadTestsFromTestCase(FeedbackTest))
    suite.addTests(unittest.defaultTestLoader.loadTestsFromTestCase(GeometryEditingTest))
    suite.addTests(unittest.defaultTestLoader.loadTestsFromTestCase(SymmetryTest))
    suite.addTests(unittest.defaultTestLoader.loadTestsFromTestCase(BoxTopologyTest))
    suite.addTests(unittest.defaultTestLoader.loadTestsFromTestCase(PrimitiveTopologyTest))
    suite.addTests(unittest.defaultTestLoader.loadTestsFromTestCase(StructuredResizeTest))
    suite.addTests(unittest.defaultTestLoader.loadTestsFromTestCase(CageEditingTest))
    suite.addTests(unittest.defaultTestLoader.loadTestsFromTestCase(DyadicTMeshTest))
    suite.addTests(unittest.defaultTestLoader.loadTestsFromTestCase(CatmullClarkTest))
    return suite
