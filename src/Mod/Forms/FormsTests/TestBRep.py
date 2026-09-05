# SPDX-License-Identifier: LGPL-2.1-or-later

import os
import math
import tempfile
import unittest

import FreeCAD as App
import Part

from Forms import (
    create_additive_form,
    create_box,
    create_cylinder,
    create_face,
    create_pipe,
    create_quadball,
    create_sphere,
    create_subtractive_form,
    create_torus,
    create_tube,
    delete_faces,
    dissolve_edges,
    erase_and_fill,
    fill_holes,
    flatten_control_points,
    insert_edge,
    insert_edge_loop,
    make_editable,
    match_boundary,
    set_edge_crease,
    straighten_control_points,
    subdivide_faces,
    thicken_surface,
    unweld_segment,
    weld_boundaries,
)
from Forms.brep import cage_to_solid, cage_to_surface
from Forms.box import FormFeatureProxy
from Forms.cage import (
    ControlCage,
    ControlElementMapper,
    control_indices_for_element,
    control_surface_points,
)
from Forms.topology import box_control_cage, cage_edge_loop, cage_edges, face_control_cage
from Forms.operations import (
    insert_edge_on_face,
    insert_point_face_target,
    insert_point_edges,
    local_insert_target,
    preview_flatten_control_points,
    preview_straighten_control_points,
    preview_straighten_surface_points,
)
from Forms.placement import global_placement
from Forms.tmesh import HierarchicalTMesh
from Forms.symmetry import mirror_faces, vertex_map


class BRepConversionTest(unittest.TestCase):
    def tearDown(self):
        for document in list(App.listDocuments().values()):
            if document.Name.startswith("FormsTest"):
                App.closeDocument(document.Name)

    def test_global_placement_uses_the_document_tree_context(self):
        document = App.newDocument("FormsTestGlobalPlacement")
        parent = document.addObject("App::Part", "Parent")
        parent.Placement.Base = App.Vector(10, 0, 0)
        obj = document.addObject("Part::Feature", "Nested")
        obj.Placement.Base = App.Vector(1, 2, 3)
        parent.addObject(obj)

        placement = global_placement(obj)

        self.assertTrue(placement.Base.isEqual(App.Vector(11, 2, 3), 1.0e-9))

    def test_partdesign_boolean_history_uses_the_new_feature_tag(self):
        cases = (
            (
                "Additive",
                create_additive_form,
                App.Placement(App.Vector(15, 10, 5), App.Rotation()),
                12.0,
                "FUS",
            ),
            (
                "Subtractive",
                create_subtractive_form,
                App.Placement(App.Vector(10, 10, 5), App.Rotation()),
                10.0,
                "CUT",
            ),
        )
        for operation, factory, placement, size, opcode in cases:
            with self.subTest(operation=operation):
                document = App.newDocument(f"FormsTest{operation}HistoryTag")
                body = document.addObject("PartDesign::Body", "Body")
                source = body.newObject("PartDesign::Feature", "Source")
                source.Shape = Part.makeBox(20, 20, 10)
                form = factory(body, source, "Box", placement=placement)
                form.Length = size
                form.Width = size
                form.Height = size
                document.recompute()

                mapped_names = [
                    str(name) for name in form.Shape.ElementMap if opcode in str(name)
                ]
                form_tag = f":H{form.ID:x}"
                source_tag = f":H{source.ID:x}"
                self.assertTrue(mapped_names)
                self.assertTrue(all(form_tag in name for name in mapped_names))
                self.assertTrue(all(source_tag not in name for name in mapped_names))
                self.assertEqual(form.Shape.Tag, form.ID)
                self.assertEqual(source.Shape.Tag, source.ID)

    def test_default_cage_produces_valid_solid(self):
        vertices, faces = box_control_cage(20, 20, 20)
        solid, deviation, level = cage_to_solid(vertices, faces, 0.05, 3)
        self.assertEqual(solid.ShapeType, "Solid")
        self.assertTrue(solid.isValid())
        self.assertEqual(len(solid.Solids), 1)
        self.assertEqual(len(solid.Faces), 6)
        self.assertGreater(solid.Volume, 0.0)
        self.assertLessEqual(deviation, 0.05)
        self.assertEqual(level, 2)

    def test_insert_point_handler_accepts_only_opposite_face_sides(self):
        vertices, faces = box_control_cage(20, 20, 20)
        cage = ControlCage(vertices, faces)
        face = faces[0]
        sides = [
            tuple(sorted((start, face[(index + 1) % 4])))
            for index, start in enumerate(face)
        ]

        face_id, first_side, second_side = insert_point_face_target(
            cage, sides[0], sides[2]
        )
        self.assertEqual(face_id, 0)
        self.assertEqual((first_side - second_side) % 4, 2)
        with self.assertRaisesRegex(ValueError, "opposite sides"):
            insert_point_face_target(cage, sides[0], sides[1])

    def test_unweld_then_weld_round_trips_two_document_forms(self):
        document = App.newDocument("FormsTestWeldRoundTrip")
        obj = create_cylinder(document)
        obj.HeightSegments = 3
        document.recompute()
        cage = ControlCage.from_object(obj)
        seam = None
        for edge in cage_edges(cage.faces):
            candidate = cage_edge_loop(cage.faces, edge)
            try:
                cage.split_along_edges(candidate)
            except ValueError:
                continue
            seam = candidate
            break
        self.assertIsNotNone(seam)

        document.openTransaction("Unweld Form")
        first, second = unweld_segment(obj, seam)
        document.recompute()
        document.commitTransaction()

        self.assertIs(first, obj)
        self.assertIsNotNone(document.getObject(second.Name))
        first_cage = ControlCage.from_object(first)
        second_cage = ControlCage.from_object(second)
        self.assertFalse(first_cage.is_closed)
        self.assertFalse(second_cage.is_closed)
        self.assertTrue(first.Shape.isValid())
        self.assertTrue(second.Shape.isValid())
        second_name = second.Name

        document.undo()
        document.recompute()
        self.assertIsNone(document.getObject(second_name))
        self.assertTrue(ControlCage.from_object(first).is_closed)
        document.redo()
        document.recompute()
        second = document.getObject(second_name)
        self.assertIsNotNone(second)
        first_cage = ControlCage.from_object(first)
        second_cage = ControlCage.from_object(second)

        document.openTransaction("Weld Forms")
        weld_boundaries(
            first,
            first_cage.boundary_edges[0],
            second,
            second_cage.boundary_edges[0],
        )
        document.recompute()
        document.commitTransaction()

        self.assertIsNone(document.getObject(second_name))
        self.assertTrue(ControlCage.from_object(first).is_closed)
        self.assertTrue(first.Shape.isValid())
        self.assertEqual(first.Shape.ShapeType, "Solid")
        document.undo()
        document.recompute()
        self.assertIsNotNone(document.getObject(second_name))
        self.assertFalse(ControlCage.from_object(first).is_closed)
        document.redo()
        document.recompute()
        self.assertIsNone(document.getObject(second_name))
        self.assertTrue(ControlCage.from_object(first).is_closed)

    def test_unweld_can_keep_both_surfaces_in_one_form(self):
        document = App.newDocument("FormsTestUnweldSingleObject")
        obj = create_cylinder(document)
        obj.HeightSegments = 3
        document.recompute()
        original = ControlCage.from_object(obj)
        seam = None
        for edge in cage_edges(original.faces):
            candidate = cage_edge_loop(original.faces, edge)
            try:
                original.split_along_edges(candidate)
            except ValueError:
                continue
            seam = candidate
            break
        self.assertIsNotNone(seam)

        result = unweld_segment(obj, seam, separate_forms=False)
        document.recompute()

        cage = ControlCage.from_object(obj)
        self.assertEqual(result, (obj,))
        self.assertEqual(len(document.Objects), 1)
        self.assertEqual(len(cage.faces), len(original.faces))
        self.assertEqual(len(cage.boundary_loops()), 2)
        self.assertFalse(cage.is_closed)
        self.assertTrue(obj.Shape.isValid())
        mapper = ControlElementMapper(obj)
        component_controls = [
            {
                vertex
                for face_id in face_ids
                for vertex in cage.faces[face_id]
            }
            for face_ids in cage.face_components()
        ]
        mapped_edges = [
            set(mapped)
            for edge in obj.Shape.Edges
            for mapped in (mapper.indices(edge),)
            if len(mapped) == 2
        ]
        self.assertTrue(mapped_edges)
        self.assertTrue(
            all(any(mapped.issubset(controls) for controls in component_controls)
                for mapped in mapped_edges)
        )
        self.assertTrue(
            all(any(mapped.issubset(controls) for mapped in mapped_edges)
                for controls in component_controls)
        )

    def test_insert_point_joins_independent_edge_positions_with_real_topology(self):
        document = App.newDocument("FormsTestInsertPoint")
        obj = create_box(document)
        obj.XSegments = 1
        obj.YSegments = 1
        obj.ZSegments = 1
        document.recompute()
        original = ControlCage.from_object(obj)
        face = original.faces[0]
        first_edge = tuple(sorted((face[0], face[1])))
        opposite_edge = tuple(sorted((face[2], face[3])))

        _obj, point_ids = insert_point_edges(
            obj,
            ((first_edge, 0.1), (opposite_edge, 0.7)),
        )
        document.recompute()

        edited = ControlCage.from_object(obj)
        mesh = HierarchicalTMesh.decode(obj.TMeshData)
        self.assertEqual(edited.vertices, original.vertices)
        self.assertEqual(edited.faces, original.faces)
        self.assertEqual(len(mesh.vertices), len(original.vertices) + 2)
        self.assertEqual(len(mesh.faces), len(original.faces) + 1)
        self.assertIn(tuple(sorted(point_ids)), mesh.atomic_edges())
        self.assertTrue(edited.is_closed)
        self.assertTrue(obj.Shape.isValid())
        self.assertEqual(obj.Shape.ShapeType, "Solid")
        self.assertEqual(len(obj.Shape.Faces), len(original.faces) + 1)
        self.assertEqual(len(control_surface_points(obj)), len(mesh.vertices))
        mapper = ControlElementMapper(obj)
        seam_children = [
            edge for edge in obj.Shape.Edges if set(mapper.indices(edge)) == set(point_ids)
        ]
        self.assertEqual(len(seam_children), 1)

    def test_insert_point_polyline_continues_across_an_adjacent_face(self):
        document = App.newDocument("FormsTestInsertPointPolyline")
        obj = create_box(document)
        obj.XSegments = 1
        obj.YSegments = 1
        obj.ZSegments = 1
        document.recompute()
        original = ControlCage.from_object(obj)
        first_face = original.faces[0]
        shared = tuple(sorted((first_face[1], first_face[2])))
        neighbor = next(
            face
            for face in original.faces[1:]
            if shared[0] in face and shared[1] in face
        )
        third_edge = next(
            tuple(sorted((start, neighbor[(index + 1) % len(neighbor)])))
            for index, start in enumerate(neighbor)
            if tuple(sorted((start, neighbor[(index + 1) % len(neighbor)]))) != shared
            and start not in shared
            and neighbor[(index + 1) % len(neighbor)] not in shared
        )
        first_edge = tuple(sorted((first_face[3], first_face[0])))

        _obj, point_ids = insert_point_edges(
            obj,
            ((first_edge, 0.2), (shared, 0.6), (third_edge, 0.8)),
        )
        document.recompute()

        edited = ControlCage.from_object(obj)
        mesh = HierarchicalTMesh.decode(obj.TMeshData)
        edges = mesh.atomic_edges()
        self.assertIn(tuple(sorted(point_ids[:2])), edges)
        self.assertIn(tuple(sorted(point_ids[1:])), edges)
        self.assertEqual(len(mesh.faces), len(original.faces) + 2)
        self.assertEqual(len(obj.Shape.Faces), len(original.faces) + 2)
        self.assertTrue(edited.is_closed)
        self.assertTrue(obj.Shape.isValid())

    def test_control_cage_rejects_non_finite_geometry_before_conversion(self):
        with self.assertRaisesRegex(ValueError, "finite 3D coordinates"):
            ControlCage([(0, 0, 0), (1, 0, 0), (0, float("nan"), 0)], [(0, 1, 2)])

        with self.assertRaisesRegex(ValueError, "vertex sharpness must be finite"):
            ControlCage(
                [(0, 0, 0), (1, 0, 0), (0, 1, 0)],
                [(0, 1, 2)],
                [0.0, float("inf"), 0.0],
            )

        with self.assertRaisesRegex(ValueError, "edge sharpness must be finite"):
            ControlCage(
                [(0, 0, 0), (1, 0, 0), (0, 1, 0)],
                [(0, 1, 2)],
                edge_sharpness={(0, 2): float("inf")},
            )

    def test_edge_sharpness_produces_a_valid_squarer_solid(self):
        vertices, faces = box_control_cage(20, 20, 20)
        smooth, _deviation, _level = cage_to_solid(vertices, faces, 0.05, 3)
        edge_values = {edge: 10.0 for edge in cage_edges(faces)}
        sharp, _deviation, _level = cage_to_solid(vertices, faces, 0.05, 3, edge_values)
        self.assertTrue(sharp.isValid())
        self.assertEqual(sharp.ShapeType, "Solid")
        self.assertGreater(sharp.Volume, smooth.Volume)

    def test_crease_uncrease_and_geometry_alignment_recompute_valid_solid(self):
        document = App.newDocument("FormsTestModifyTools")
        obj = create_box(document)
        document.recompute()
        cage = ControlCage.from_object(obj)
        chain = cage.edge_ring(cage_edges(cage.faces)[0])
        chain_edges = set(chain)

        set_edge_crease(obj, chain_edges, 10.0)
        document.recompute()
        self.assertTrue(chain_edges.issubset(ControlCage.from_object(obj).edge_sharpness))
        set_edge_crease(obj, chain_edges, 0.0)
        flatten_control_points(obj, cage.faces[0])
        document.recompute()

        self.assertFalse(chain_edges.intersection(ControlCage.from_object(obj).edge_sharpness))
        self.assertEqual(obj.ConversionStatus, "Valid solid")
        self.assertTrue(obj.Shape.isValid())

    def test_flatten_preview_does_not_modify_the_form(self):
        document = App.newDocument("FormsTestFlattenPreview")
        obj = create_box(document)
        document.recompute()
        original_points = list(obj.ControlPoints)
        original_volume = obj.Shape.Volume

        preview = preview_flatten_control_points(
            obj, ControlCage.from_object(obj).faces[0], ((0, 0, 0), (0, 0, 1))
        )

        self.assertFalse(preview.isNull())
        self.assertEqual(list(obj.ControlPoints), original_points)
        self.assertAlmostEqual(obj.Shape.Volume, original_volume)

    def test_straighten_previews_do_not_modify_the_form(self):
        document = App.newDocument("FormsTestStraightenPreview")
        obj = create_box(document)
        document.recompute()
        points = list(obj.ControlPoints)
        points[1] = points[1].add(App.Vector(0, 3, 1))
        obj.ControlPoints = points
        obj.CageMode = "Editable"
        document.recompute()
        original_points = list(obj.ControlPoints)

        control_preview = preview_straighten_control_points(obj, (0, 1, 2))
        surface_preview = preview_straighten_surface_points(obj, (0, 1, 2))

        self.assertFalse(control_preview.isNull())
        self.assertFalse(surface_preview.isNull())
        self.assertEqual(list(obj.ControlPoints), original_points)

        straighten_control_points(obj, (0, 1, 2), surface_points=True)
        document.recompute()
        mapped = control_surface_points(obj)
        direction = mapped[2].sub(mapped[0])
        self.assertLess(direction.cross(mapped[1].sub(mapped[0])).Length, 1.0e-5)

    def test_editable_form_recomputes_a_real_solid(self):
        document = App.newDocument("FormsTestEditable")
        obj = create_box(document)
        document.recompute()
        self.assertEqual(obj.XSegments, 2)
        self.assertEqual(obj.YSegments, 2)
        self.assertEqual(obj.ZSegments, 2)
        self.assertEqual(obj.Shape.ShapeType, "Solid")
        self.assertEqual(obj.ConversionStatus, "Valid solid")

        make_editable(obj)
        points = list(obj.ControlPoints)
        points[0] = points[0].add(App.Vector(-2, -1, 1))
        obj.ControlPoints = points
        document.recompute()
        self.assertEqual(obj.Shape.ShapeType, "Solid")
        self.assertTrue(obj.Shape.isValid())
        self.assertGreater(obj.Shape.Volume, 0.0)

    def test_generated_solid_survives_save_and_restore(self):
        handle, path = tempfile.mkstemp(suffix=".FCStd")
        os.close(handle)
        try:
            document = App.newDocument("FormsTestSave")
            obj = create_box(document)
            make_editable(obj)
            document.recompute()
            expected_volume = obj.Shape.Volume
            document.saveAs(path)
            App.closeDocument(document.Name)

            restored = App.openDocument(path)
            restored.recompute()
            restored_obj = restored.getObject("FormBox")
            self.assertEqual(restored_obj.Shape.ShapeType, "Solid")
            self.assertTrue(restored_obj.Shape.isValid())
            self.assertAlmostEqual(restored_obj.Shape.Volume, expected_volume, places=6)
        finally:
            if os.path.exists(path):
                os.remove(path)

    def test_additional_primitives_produce_brep_shapes(self):
        document = App.newDocument("FormsTestPrimitives")
        cylinder = create_cylinder(document)
        sphere = create_sphere(document)
        quadball = create_quadball(document)
        face = create_face(document)
        torus = create_torus(document)
        tube = create_tube(document)
        document.recompute()
        self.assertEqual(cylinder.Shape.ShapeType, "Solid")
        self.assertEqual(sphere.Shape.ShapeType, "Solid")
        self.assertEqual(quadball.Shape.ShapeType, "Solid")
        self.assertIn(face.Shape.ShapeType, ("Face", "Shell"))
        self.assertTrue(cylinder.Shape.isValid())
        self.assertTrue(sphere.Shape.isValid())
        self.assertTrue(quadball.Shape.isValid())
        self.assertTrue(face.Shape.isValid())
        self.assertEqual(torus.Shape.ShapeType, "Solid")
        self.assertEqual(tube.Shape.ShapeType, "Solid")
        self.assertTrue(torus.Shape.isValid())
        self.assertTrue(tube.Shape.isValid())
        self.assertEqual(len(torus.Shape.Solids), 1)
        self.assertEqual(len(tube.Shape.Solids), 1)

    def test_all_primitives_share_one_feature_proxy_lifecycle(self):
        document = App.newDocument("FormsTestSharedProxy")
        objects = (
            create_box(document),
            create_cylinder(document),
            create_sphere(document),
            create_quadball(document),
            create_face(document),
            create_torus(document),
            create_tube(document),
        )
        document.recompute()

        self.assertTrue(all(isinstance(obj.Proxy, FormFeatureProxy) for obj in objects))
        self.assertTrue(all(obj.ConversionStatus.startswith("Valid") for obj in objects))

    def test_form_face_can_start_from_a_face_or_closed_wire(self):
        rectangle = Part.makePolygon(
            [
                App.Vector(-15, -5, 3),
                App.Vector(15, -5, 3),
                App.Vector(15, 5, 3),
                App.Vector(-15, 5, 3),
                App.Vector(-15, -5, 3),
            ]
        )
        for index, profile in enumerate((Part.Face(rectangle), Part.Wire([Part.makeCircle(10)]))):
            with self.subTest(profile=profile.ShapeType):
                document = App.newDocument(f"FormsTestProfileFace{index}")
                obj = create_face(document, profile=profile)
                document.recompute()

                cage = ControlCage.from_object(obj)
                self.assertEqual(obj.CageMode, "Editable")
                self.assertEqual(len(cage.boundary_loops()), 1)
                self.assertTrue(obj.Shape.isValid())
                expected = profile if profile.ShapeType == "Face" else Part.Face(profile)
                self.assertAlmostEqual(obj.Shape.Area, expected.Area, places=7)

    def test_profile_form_face_exposes_and_subdivides_control_patches(self):
        profile = Part.Face(
            Part.makePolygon(
                [
                    App.Vector(-15, -5, 0),
                    App.Vector(15, -5, 0),
                    App.Vector(15, 5, 0),
                    App.Vector(-15, 5, 0),
                    App.Vector(-15, -5, 0),
                ]
            )
        )
        document = App.newDocument("FormsTestProfileFaceSubdivision")
        obj = create_face(document, profile=profile)
        document.recompute()

        cage = ControlCage.from_object(obj)
        self.assertEqual(len(obj.Shape.Faces), 1)
        self.assertGreater(len(cage.faces), 1)

        obj.Proxy.show_edit_shape(obj, True)
        self.assertGreater(len(obj.Shape.Faces), 1)
        mapper = ControlElementMapper(obj)
        mapped_faces = {
            mapper.face_id(mapper.indices(face))
            for face in obj.Shape.Faces
        }
        mapped_faces.discard(None)
        self.assertEqual(mapped_faces, set(range(len(cage.faces))))

        subdivide_faces(obj, [0])
        document.recompute()
        self.assertTrue(obj.TMeshData)
        obj.Proxy.show_edit_shape(obj, False)
        self.assertGreater(len(obj.Shape.Faces), 1)

    def test_profile_form_face_restores_exact_display_after_unmodified_edit(self):
        profile = Part.Face(
            Part.Wire(
                [
                    Part.makeCircle(10),
                ]
            )
        )
        document = App.newDocument("FormsTestProfileFaceEditDisplay")
        obj = create_face(document, profile=profile)
        document.recompute()

        self.assertEqual(len(obj.Shape.Faces), 1)
        self.assertGreater(len(ControlCage.from_object(obj).faces), 1)
        obj.Proxy.show_edit_shape(obj, True)
        self.assertGreater(len(obj.Shape.Faces), 1)
        self.assertAlmostEqual(obj.Shape.Area, profile.Area, places=7)
        free_edges = [
            edge
            for edge in obj.Shape.Edges
            if sum(
                candidate.isSame(edge)
                for face in obj.Shape.Faces
                for candidate in face.Edges
            )
            == 1
        ]
        edit_boundary = Part.makeCompound(free_edges)
        for source_edge in profile.Edges:
            for point in source_edge.discretize(Number=33):
                self.assertLess(
                    Part.Vertex(point).distToShape(edit_boundary)[0],
                    1.0e-7,
                )
        for edit_edge in free_edges:
            for point in edit_edge.discretize(Number=9):
                self.assertLess(
                    Part.Vertex(point).distToShape(profile.Wires[0])[0],
                    1.0e-7,
                )
        obj.Proxy.show_edit_shape(obj, False)
        self.assertEqual(len(obj.Shape.Faces), 1)
        self.assertAlmostEqual(obj.Shape.Area, profile.Area, places=7)

    def test_profile_form_face_creation_session_exposes_control_patches(self):
        from Forms.edit import FormEditSession

        profile = Part.Face(Part.Wire([Part.makeCircle(10)]))
        document = App.newDocument("FormsTestProfileFaceCreationDisplay")
        obj = create_face(document, profile=profile)
        document.recompute()

        # Creation starts FormEditSession directly instead of entering through
        # ViewProviderFormFace.setEdit. Exercise that exact lifecycle branch.
        session = FormEditSession.__new__(FormEditSession)
        session.obj = obj
        session.profile_edit_shape_owned = False
        session._enable_profile_edit_shape()
        self.assertTrue(session.profile_edit_shape_owned)
        self.assertGreater(len(obj.Shape.Faces), 1)
        obj.Proxy.show_edit_shape(obj, False)

    def test_profile_form_face_adapts_to_concave_curved_boundaries(self):
        points = [
            App.Vector(0, 0),
            App.Vector(20, 0),
            App.Vector(24, 4),
            App.Vector(20, 8),
            App.Vector(12, 8),
            App.Vector(9, 5),
            App.Vector(6, 8),
            App.Vector(0, 8),
            App.Vector(-3, 4),
        ]
        profile = Part.Face(
            Part.Wire(
                [
                    Part.makeLine(points[0], points[1]),
                    Part.Arc(points[1], points[2], points[3]).toShape(),
                    Part.makeLine(points[3], points[4]),
                    Part.Arc(points[4], points[5], points[6]).toShape(),
                    Part.makeLine(points[6], points[7]),
                    Part.Arc(points[7], points[8], points[0]).toShape(),
                ]
            )
        )
        profile.Placement = App.Placement(
            App.Vector(3, -2, 7),
            App.Rotation(App.Vector(1, 2, 0.5), 37),
        )
        document = App.newDocument("FormsTestConcaveProfileFace")
        obj = create_face(document, profile=profile)
        document.recompute()

        obj.Proxy.show_edit_shape(obj, True)
        self.assertGreater(len(obj.Shape.Faces), 1)
        self.assertTrue(obj.Shape.isValid())
        self.assertAlmostEqual(obj.Shape.Area, profile.Area, places=7)
        free_edges = [
            edge
            for edge in obj.Shape.Edges
            if sum(
                candidate.isSame(edge)
                for face in obj.Shape.Faces
                for candidate in face.Edges
            )
            == 1
        ]
        edit_boundary = Part.makeCompound(free_edges)
        for source_edge in profile.Edges:
            for point in source_edge.discretize(Number=17):
                self.assertLess(
                    Part.Vertex(point).distToShape(edit_boundary)[0],
                    1.0e-7,
                )

    def test_form_face_preserves_profile_holes_and_exact_boundaries(self):
        outer = Part.Wire([Part.makeCircle(20)])
        inner = Part.Wire([Part.makeCircle(8)])
        profile = Part.makeFace([outer, inner], "Part::FaceMakerCheese").Faces[0]
        document = App.newDocument("FormsTestProfileFaceHoles")
        obj = create_face(document, profile=profile)
        document.recompute()

        cage = ControlCage.from_object(obj)
        self.assertEqual(len(cage.boundary_loops()), 2)
        self.assertEqual(len(obj.Shape.Wires), 2)
        self.assertAlmostEqual(obj.Shape.Area, profile.Area, places=7)
        initial_boundary = Part.makeCompound(obj.Shape.Edges)
        for source_edge in profile.Edges:
            for point in source_edge.discretize(Number=17):
                self.assertLess(Part.Vertex(point).distToShape(initial_boundary)[0], 1.0e-7)

        boundary = {vertex for edge in cage.boundary_edges for vertex in edge}
        interior = next(index for index in range(len(cage.vertices)) if index not in boundary)
        controls = list(obj.ControlPoints)
        controls[interior] = controls[interior] + App.Vector(0, 0, 2)
        obj.ControlPoints = controls
        document.recompute()

        self.assertTrue(obj.Shape.isValid())
        self.assertEqual(len(ControlCage.from_object(obj).boundary_loops()), 2)
        free_edges = [
            edge
            for edge in obj.Shape.Edges
            if sum(
                candidate.isSame(edge)
                for face in obj.Shape.Faces
                for candidate in face.Edges
            )
            == 1
        ]
        self.assertEqual(len(Part.sortEdges(free_edges)), 2)

    def test_pipe_accepts_wire_network_and_detects_t_junction_segments(self):
        from Forms.pipe import _cage_components

        document = App.newDocument("FormsTestPipe")
        path = document.addObject("Part::Feature", "Path")
        center = App.Vector(0, 0, 0)
        path.Shape = Part.makeCompound([
            Part.makeLine(center, App.Vector(20, 0, 0)),
            Part.makeLine(center, App.Vector(0, 20, 0)),
            Part.makeLine(center, App.Vector(0, 0, 20)),
        ])
        pipe = create_pipe(document, path)
        pipe.Diameter = 4
        self.assertEqual(pipe.SectionSegments, 1)
        for section_segments in (1, 2):
            with self.subTest(section_segments=section_segments):
                pipe.SectionSegments = section_segments
                document.recompute()
                self.assertEqual(len(pipe.PipeSegmentKeys), 3)
                self.assertEqual(len(pipe.Shape.Solids), 1)
                cage = ControlCage.from_object(pipe)
                self.assertEqual(len(_cage_components(cage)), 1)
                self.assertTrue(cage.is_closed)
                self.assertTrue(pipe.Shape.isValid())

        from Forms.pipe import set_segment_diameter
        set_segment_diameter(pipe, pipe.PipeSegmentKeys[0], 8)
        document.recompute()
        self.assertIn('"diameter": 8.0', pipe.SegmentDiameters[0])
        self.assertFalse(pipe.Shape.isNull(), pipe.ConversionStatus)
        self.assertTrue(pipe.Shape.isValid(), pipe.ConversionStatus)

        from Forms.pipe import set_segment_samples
        control_count = len(pipe.ControlPoints)
        set_segment_samples(pipe, pipe.PipeSegmentKeys[0], 6)
        document.recompute()
        self.assertIn('"samples": 6', pipe.SegmentSamples[0])
        self.assertGreater(len(pipe.ControlPoints), control_count)
        self.assertTrue(pipe.Shape.isValid(), pipe.ConversionStatus)

    def test_pipe_closes_periodically_around_a_closed_wire(self):
        document = App.newDocument("FormsTestClosedPipe")
        path = document.addObject("Part::Feature", "ClosedPath")
        path.Shape = Part.Wire([Part.makeCircle(20)])
        pipe = create_pipe(document, path)
        pipe.Diameter = 4
        document.recompute()
        self.assertEqual(len(pipe.PipeSegmentKeys), 1)
        self.assertEqual(pipe.Shape.ShapeType, "Solid")
        self.assertTrue(pipe.Shape.isValid())

    def test_pipe_splits_an_edge_at_an_interior_t_junction(self):
        document = App.newDocument("FormsTestInteriorTJunction")
        path = document.addObject("Part::Feature", "TJunctionPath")
        path.Shape = Part.makeCompound([
            Part.makeLine(App.Vector(-20, 0, 0), App.Vector(20, 0, 0)),
            Part.makeLine(App.Vector(0, 0, 0), App.Vector(0, 20, 0)),
        ])
        pipe = create_pipe(document, path)
        pipe.Diameter = 4
        for section_segments in (1, 2):
            with self.subTest(section_segments=section_segments):
                pipe.SectionSegments = section_segments
                document.recompute()
                self.assertEqual(len(pipe.PipeSegmentKeys), 3)
                self.assertEqual(len(pipe.Shape.Solids), 1)
                self.assertTrue(pipe.Shape.isValid())

    def test_pipe_builds_an_oblique_y_junction_at_both_section_densities(self):
        from Forms.pipe import _cage_components

        document = App.newDocument("FormsTestObliqueYJunction")
        path = document.addObject("Part::Feature", "YJunctionPath")
        center = App.Vector(0, 0, 0)
        path.Shape = Part.makeCompound(
            [
                Part.makeLine(center, App.Vector(0, 30, 0)),
                Part.makeLine(center, App.Vector(-26, -15, 0)),
                Part.makeLine(center, App.Vector(26, -15, 0)),
            ]
        )
        pipe = create_pipe(document, path)
        pipe.Diameter = 4
        for section_segments in (1, 2):
            with self.subTest(section_segments=section_segments):
                pipe.SectionSegments = section_segments
                document.recompute()
                self.assertFalse(pipe.Shape.isNull(), pipe.ConversionStatus)
                self.assertEqual(len(pipe.Shape.Solids), 1, pipe.ConversionStatus)
                self.assertTrue(pipe.Shape.isValid(), pipe.ConversionStatus)
                cage = ControlCage.from_object(pipe)
                self.assertEqual(len(_cage_components(cage)), 1)
                self.assertTrue(cage.is_closed)
                # A planar equal-diameter junction must retain the pipe's
                # thickness normal to that plane instead of inflating into a
                # rounded central ball.
                expected_half_thickness = 2.0 * (1.0 + 0.08 / section_segments)
                self.assertLessEqual(
                    max(abs(point.z) for point in pipe.ControlPoints),
                    expected_half_thickness + 1.0e-7,
                )

    def test_pipe_junction_face_bands_do_not_switch_sides(self):
        from Forms.pipe import _three_way_junction_cage

        directions = [
            App.Vector(0, 1, 0),
            App.Vector(-0.866025403784, -0.5, 0),
            App.Vector(0.866025403784, -0.5, 0),
        ]
        ports = []
        global_index = 0
        side_count = 8
        for direction in directions:
            direction.normalize()
            ring_center = direction * 4.0
            normal = App.Vector(0, 0, 1)
            binormal = direction.cross(normal)
            loop = []
            guides = {}
            for side in range(side_count):
                angle = 2.0 * math.pi * side / side_count
                point = ring_center + normal * (2.0 * math.cos(angle)) + binormal * (
                    2.0 * math.sin(angle)
                )
                loop.append((global_index, (point.x, point.y, point.z)))
                guide = point + direction * 2.0
                guides[global_index] = (guide.x, guide.y, guide.z)
                global_index += 1
            ports.append((direction, loop, 4.0, guides))

        _vertices, faces, boundary_maps, _updates = _three_way_junction_cage(
            App.Vector(), ports, 1
        )
        edge_faces = {}
        for face_index, face in enumerate(faces):
            for position, first in enumerate(face):
                edge = tuple(sorted((first, face[(position + 1) % 4])))
                edge_faces.setdefault(edge, []).append(face_index)

        boundary_port = {}
        rings = [list(mapping) for mapping in boundary_maps]
        for port_index, ring in enumerate(rings):
            for position, first in enumerate(ring):
                boundary_port[
                    tuple(sorted((first, ring[(position + 1) % len(ring)])))
                ] = port_index

        def strip_exit(start_edge):
            edge = start_edge
            previous_face = None
            while True:
                candidates = [
                    face_index
                    for face_index in edge_faces[edge]
                    if face_index != previous_face
                ]
                if not candidates:
                    return boundary_port[edge]
                face_index = candidates[0]
                face = faces[face_index]
                position = next(
                    index
                    for index in range(4)
                    if tuple(sorted((face[index], face[(index + 1) % 4]))) == edge
                )
                edge = tuple(
                    sorted((face[(position + 2) % 4], face[(position + 3) % 4]))
                )
                previous_face = face_index

        for port_index, ring in enumerate(rings):
            exits = [
                strip_exit(tuple(sorted((first, ring[(position + 1) % len(ring)]))))
                for position, first in enumerate(ring)
            ]
            self.assertEqual(len(set(exits[: side_count // 2])), 1)
            self.assertEqual(len(set(exits[side_count // 2 :])), 1)
            self.assertNotEqual(exits[0], port_index)
            self.assertNotEqual(exits[side_count // 2], port_index)
            self.assertNotEqual(exits[0], exits[side_count // 2])

    def test_pipe_frames_follow_the_subdivision_centerline_tangent(self):
        from Forms.pipe import _frames

        points = [
            App.Vector(-2, 0, 0),
            App.Vector(0, 0, 0),
            App.Vector(0, 20, 0),
        ]
        normal, binormal = _frames(points)[1]
        tangent = normal.cross(binormal)

        expected = points[2] - points[0]
        expected.normalize()
        self.assertAlmostEqual(tangent.dot(expected), 1.0, places=7)

    def test_pipe_rounds_source_edge_junctions_with_balanced_controls(self):
        from Forms.pipe import _segment_points, path_segments

        first = Part.makeLine(App.Vector(0, 0, 0), App.Vector(60, 0, 0))
        second = Part.makeLine(App.Vector(60, 0, 0), App.Vector(60, 30, 0))
        shape = Part.makeCompound([first, second])
        segments, _adjacency, edge_records = path_segments(shape, include_edges=True)
        edges = [record[0] for record in edge_records]
        points = _segment_points(edges, segments[0], 3)

        corner = App.Vector(60, 0, 0)
        nearest = sorted(points, key=lambda point: point.sub(corner).Length)[:2]
        self.assertGreater(nearest[0].sub(corner).Length, 1.0e-7)
        self.assertAlmostEqual(
            nearest[0].sub(corner).Length,
            nearest[1].sub(corner).Length,
            places=7,
        )

    def test_pipe_keeps_one_continuous_cage_across_connected_wires(self):
        from Forms.pipe import _cage_components, pipe_control_cage

        document = App.newDocument("FormsTestConnectedPipeWires")
        path = document.addObject("Part::Feature", "ConnectedWires")
        points = [
            App.Vector(0, 0, 0),
            App.Vector(10, 0, 0),
            App.Vector(10, 50, 0),
            App.Vector(30, 70, 0),
        ]
        path.Shape = Part.makeCompound(
            [Part.makeLine(points[index], points[index + 1]) for index in range(3)]
        )

        vertices, faces, keys, _descriptions = pipe_control_cage(path, 4)
        cage = ControlCage(vertices, faces)
        self.assertEqual(len(keys), 1)
        self.assertEqual(len(_cage_components(cage)), 1)
        self.assertTrue(cage.is_closed)

    def test_open_face_cage_converts_to_surface(self):
        vertices, faces = face_control_cage(20, 10, 2, 2)
        shape, deviation, level = cage_to_surface(vertices, faces, 0.05, 3)
        self.assertIn(shape.ShapeType, ("Face", "Shell"))
        self.assertTrue(shape.isValid())
        self.assertLessEqual(deviation, 0.05)
        self.assertEqual(level, 2)

    def test_thicken_surface_produces_a_valid_editable_solid(self):
        document = App.newDocument("FormsTestThicken")
        obj = create_face(document)
        document.recompute()
        source = ControlCage.from_object(obj)

        thicken_surface(obj, 2.0)
        document.recompute()

        thickened = ControlCage.from_object(obj)
        self.assertEqual(obj.CageMode, "Editable")
        self.assertTrue(thickened.is_closed)
        self.assertEqual(len(thickened.vertices), len(source.vertices) * 2)
        self.assertEqual(obj.Shape.ShapeType, "Solid")
        self.assertTrue(obj.Shape.isValid())
        self.assertGreater(obj.Shape.Volume, 0.0)
        self.assertEqual(obj.ConversionStatus, "Valid solid")

    def test_delete_face_changes_a_closed_form_into_a_surface(self):
        document = App.newDocument("FormsTestDeleteFace")
        obj = create_box(document)
        document.recompute()
        original = ControlCage.from_object(obj)

        delete_faces(obj, [0])
        document.recompute()

        edited = ControlCage.from_object(obj)
        self.assertEqual(obj.CageMode, "Editable")
        self.assertEqual(len(edited.faces), len(original.faces) - 1)
        self.assertEqual(len(edited.boundary_edges), 4)
        self.assertFalse(edited.is_closed)
        self.assertIn(obj.Shape.ShapeType, ("Face", "Shell"))
        self.assertTrue(obj.Shape.isValid())
        self.assertEqual(obj.ConversionStatus, "Valid surface")

    def test_delete_edge_dissolves_seam_without_opening_surface(self):
        document = App.newDocument("FormsTestDissolveEdge")
        obj = create_face(document)
        document.recompute()
        cage = ControlCage.from_object(obj)
        internal = [edge for edge, count in cage.edge_counts().items() if count == 2]
        self.assertTrue(internal)
        initial_boundary = tuple(cage.boundary_loops())
        initial_faces = len(obj.Shape.Faces)

        dissolve_edges(obj, [internal[0]])
        document.recompute()

        self.assertTrue(obj.Shape.isValid(), obj.ConversionStatus)
        self.assertEqual(tuple(ControlCage.from_object(obj).boundary_loops()), initial_boundary)
        self.assertEqual(len(obj.ControlFaces), len(cage.faces))
        self.assertEqual(len(obj.Shape.Faces), initial_faces - 1)
        self.assertEqual(len(obj.DissolvedEdges), 1)
        mapper = ControlElementMapper(obj)
        self.assertEqual(len(mapper.logical_faces), len(cage.faces) - 1)
        self.assertNotIn(internal[0], set(cage_edges(mapper.logical_faces)))
        merged = next(
            face for face, group in zip(mapper.logical_faces, mapper.logical_face_groups)
            if len(group) == 2
        )
        self.assertEqual(len(merged), 6)
        self.assertIn(tuple(merged), [mapper.indices(face) for face in obj.Shape.Faces])

    def test_delete_whole_edge_segment_merges_a_face_strip(self):
        document = App.newDocument("FormsTestDissolveEdgeSegment")
        obj = create_face(document)
        obj.XSegments = 3
        obj.YSegments = 1
        document.recompute()
        cage = ControlCage.from_object(obj)
        internal = [edge for edge, count in cage.edge_counts().items() if count == 2]
        self.assertEqual(len(internal), 2)

        dissolve_edges(obj, internal)
        document.recompute()

        self.assertTrue(obj.Shape.isValid(), obj.ConversionStatus)
        self.assertEqual(len(obj.Shape.Faces), 1)
        self.assertEqual(len(obj.DissolvedEdges), 2)
        self.assertEqual(len(ControlCage.from_object(obj).boundary_loops()), 1)
        mapper = ControlElementMapper(obj)
        self.assertEqual(len(mapper.logical_faces), 1)
        self.assertEqual(mapper.logical_face_groups, [(0, 1, 2)])

    def test_delete_edge_rejects_surface_boundary(self):
        document = App.newDocument("FormsTestDissolveBoundaryEdge")
        obj = create_face(document)
        document.recompute()
        boundary = ControlCage.from_object(obj).boundary_edges[0]
        with self.assertRaisesRegex(ValueError, "internal"):
            dissolve_edges(obj, [boundary])

    def test_delete_edge_keeps_a_closed_form_solid(self):
        document = App.newDocument("FormsTestDissolveSolidEdge")
        obj = create_box(document)
        obj.XSegments = 1
        obj.YSegments = 1
        obj.ZSegments = 1
        document.recompute()
        cage = ControlCage.from_object(obj)
        edge = next(iter(cage.edge_counts()))
        initial_faces = len(obj.Shape.Faces)

        dissolve_edges(obj, [edge])
        document.recompute()

        self.assertFalse(obj.Shape.isNull(), obj.ConversionStatus)
        self.assertTrue(obj.Shape.isValid(), obj.ConversionStatus)
        self.assertEqual(len(obj.Shape.Solids), 1, obj.ConversionStatus)
        self.assertEqual(len(obj.Shape.Faces), initial_faces - 1)
        self.assertEqual(obj.ConversionStatus, "Valid solid")

    def test_delete_edge_recombines_locally_subdivided_faces(self):
        from Forms.tmesh import HierarchicalTMesh

        document = App.newDocument("FormsTestDissolveLocalEdge")
        obj = create_face(document)
        obj.XSegments = 1
        obj.YSegments = 1
        document.recompute()
        subdivide_faces(obj, [0], 2, 1)
        document.recompute()
        mesh = HierarchicalTMesh.decode(obj.TMeshData)
        internal = [edge for edge, count in mesh.edge_counts().items() if count == 2]
        self.assertEqual(len(mesh.faces), 2)
        self.assertTrue(internal)

        dissolve_edges(obj, [internal[0]])
        document.recompute()

        dissolved = HierarchicalTMesh.decode(obj.TMeshData)
        self.assertEqual(len(dissolved.faces), 1)
        self.assertEqual(len(obj.Shape.Faces), 1)
        self.assertTrue(obj.Shape.isValid(), obj.ConversionStatus)

    def test_delete_edge_keeps_pipe_valid(self):
        document = App.newDocument("FormsTestDissolvePipeEdge")
        path = document.addObject("Part::Feature", "Path")
        path.Shape = Part.makeLine(App.Vector(0, 0, 0), App.Vector(0, 0, 30))
        obj = create_pipe(document, path)
        obj.Diameter = 4
        document.recompute()
        cage = ControlCage.from_object(obj)
        edge = next(edge for edge, count in cage.edge_counts().items() if count == 2)
        initial_faces = len(obj.Shape.Faces)

        dissolve_edges(obj, [edge])
        document.recompute()

        self.assertFalse(obj.Shape.isNull(), obj.ConversionStatus)
        self.assertTrue(obj.Shape.isValid(), obj.ConversionStatus)
        self.assertEqual(len(obj.Shape.Solids), 1, obj.ConversionStatus)
        self.assertEqual(len(obj.Shape.Faces), initial_faces - 1)

    def test_delete_edge_is_undoable(self):
        document = App.newDocument("FormsTestDissolveEdgeUndo")
        obj = create_face(document)
        document.recompute()
        edge = next(
            edge
            for edge, count in ControlCage.from_object(obj).edge_counts().items()
            if count == 2
        )
        initial_faces = len(obj.Shape.Faces)
        document.openTransaction("Dissolve edge")
        dissolve_edges(obj, [edge])
        document.recompute()
        document.commitTransaction()
        self.assertEqual(len(obj.Shape.Faces), initial_faces - 1)

        document.undo()
        document.recompute()
        self.assertFalse(obj.DissolvedEdges)
        self.assertEqual(len(obj.Shape.Faces), initial_faces)

        document.redo()
        document.recompute()
        self.assertEqual(len(obj.Shape.Faces), initial_faces - 1)

    def test_standalone_form_matches_external_face_without_modifying_it(self):
        document = App.newDocument("FormsTestExternalFaceMatch")
        obj = create_box(document)
        document.recompute()
        delete_faces(obj, [0])
        cage = ControlCage.from_object(obj)
        loop = cage.boundary_loops()[0]
        offset = App.Vector(0, 0, 7)
        target_points = [App.Vector(*cage.vertices[index]).add(offset) for index in loop]
        wire = Part.makePolygon(target_points + [target_points[0]])
        support = document.addObject("Part::Feature", "SupportFace")
        support.Shape = Part.Face(wire)
        original_area = support.Shape.Area
        original_length = support.Shape.Length
        original_center = App.Vector(support.Shape.CenterOfMass)
        original_vertices = [App.Vector(vertex.Point) for vertex in support.Shape.Vertexes]

        match_boundary(
            obj,
            [cage.boundary_edges[0]],
            (support, ["Face1"]),
            "Connected",
        )
        document.recompute()

        self.assertIsNone(obj.getParentGeoFeatureGroup())
        self.assertEqual(obj.getTypeIdOfProperty("MatchSupport"), "App::PropertyLinkSub")
        self.assertEqual(obj.getTypeIdOfProperty("MatchParameters"), "App::PropertyFloatList")
        self.assertAlmostEqual(support.Shape.Area, original_area, places=9)
        self.assertAlmostEqual(support.Shape.Length, original_length, places=9)
        self.assertLess(support.Shape.CenterOfMass.sub(original_center).Length, 1.0e-9)
        self.assertEqual(len(support.Shape.Vertexes), len(original_vertices))
        for vertex in support.Shape.Vertexes:
            self.assertLess(
                min(vertex.Point.sub(point).Length for point in original_vertices),
                1.0e-9,
            )
        self.assertEqual(obj.MatchSupport[0], support)
        self.assertEqual(len(obj.MatchParameters), 2 * len(obj.MatchBoundary))
        self.assertIn(obj.Shape.ShapeType, ("Face", "Shell"))
        for index in obj.MatchBoundary:
            distance, _points, _info = Part.Vertex(obj.ControlPoints[index]).distToShape(
                support.Shape
            )
            self.assertLess(distance, 1.0e-7)

        center = sum(target_points, App.Vector()).multiply(1.0 / len(target_points))
        resized_points = [
            App.Vector(
                center.x + (point.x - center.x) * 1.5,
                center.y + (point.y - center.y) * 0.6,
                point.z + 3.0,
            )
            for point in target_points
        ]
        support.Shape = Part.Face(Part.makePolygon(resized_points + [resized_points[0]]))
        document.recompute()
        matched = [obj.ControlPoints[index] for index in obj.MatchBoundary]
        matched_box = Part.makePolygon(matched + [matched[0]]).BoundBox
        support_box = support.Shape.BoundBox
        for actual, expected in (
            (matched_box.XMin, support_box.XMin),
            (matched_box.XMax, support_box.XMax),
            (matched_box.YMin, support_box.YMin),
            (matched_box.YMax, support_box.YMax),
            (matched_box.ZMin, support_box.ZMin),
            (matched_box.ZMax, support_box.ZMax),
        ):
            self.assertAlmostEqual(actual, expected, places=7)
        for index in obj.MatchBoundary:
            distance, _points, _info = Part.Vertex(obj.ControlPoints[index]).distToShape(
                support.Shape
            )
            self.assertLess(distance, 1.0e-7)

    def test_deleted_match_support_clears_stale_form_shape(self):
        document = App.newDocument("FormsTestDeletedMatchSupport")
        obj = create_box(document)
        document.recompute()
        delete_faces(obj, [0])
        cage = ControlCage.from_object(obj)
        loop = cage.boundary_loops()[0]
        target_points = [
            App.Vector(*cage.vertices[index]).add(App.Vector(0, 0, 4)) for index in loop
        ]
        support = document.addObject("Part::Feature", "TemporaryMatchSupport")
        support.Shape = Part.Face(Part.makePolygon(target_points + [target_points[0]]))
        match_boundary(obj, cage.boundary_edges, (support, ["Face1"]))
        document.recompute()
        self.assertFalse(obj.Shape.isNull())

        document.removeObject(support.Name)
        document.recompute()
        self.assertTrue(obj.Shape.isNull())
        self.assertIn("support is no longer valid", obj.ConversionStatus)

    def test_standalone_form_matches_whole_closed_wire(self):
        document = App.newDocument("FormsTestExternalWireMatch")
        obj = create_box(document)
        document.recompute()
        delete_faces(obj, [0])
        cage = ControlCage.from_object(obj)
        loop = cage.boundary_loops()[0]
        target_points = [
            App.Vector(*cage.vertices[index]).add(App.Vector(0, 0, -6)) for index in loop
        ]
        support = document.addObject("Part::Feature", "SupportWire")
        support.Shape = Part.makePolygon(target_points + [target_points[0]])
        original_length = support.Shape.Length
        original_vertices = [App.Vector(vertex.Point) for vertex in support.Shape.Vertexes]

        match_boundary(
            obj,
            [cage.boundary_edges[0]],
            (support, []),
            "Connected",
        )
        document.recompute()

        self.assertAlmostEqual(support.Shape.Length, original_length, places=9)
        self.assertEqual(len(support.Shape.Vertexes), len(original_vertices))
        for vertex in support.Shape.Vertexes:
            self.assertLess(
                min(vertex.Point.sub(point).Length for point in original_vertices),
                1.0e-9,
            )
        self.assertEqual(obj.MatchSupport[0], support)
        for index in obj.MatchBoundary:
            distance, _points, _info = Part.Vertex(obj.ControlPoints[index]).distToShape(
                support.Shape
            )
            self.assertLess(distance, 1.0e-7)

        edge_matched = create_box(document, "EdgeMatchedForm")
        document.recompute()
        delete_faces(edge_matched, [0])
        edge_cage = ControlCage.from_object(edge_matched)
        edge_names = [f"Edge{index}" for index in range(len(support.Shape.Edges), 0, -1)]
        match_boundary(
            edge_matched,
            [edge_cage.boundary_edges[0]],
            (support, edge_names),
            "Connected",
        )
        document.recompute()
        for index in edge_matched.MatchBoundary:
            distance, _points, _info = Part.Vertex(edge_matched.ControlPoints[index]).distToShape(
                support.Shape
            )
            self.assertLess(distance, 1.0e-7)

    @unittest.skipUnless(App.GuiUp, "interactive Match selection requires FreeCADGui")
    def test_standalone_face_selection_enables_and_executes_match_command(self):
        import FreeCADGui as Gui
        from PySide import QtWidgets

        import CommandTopology
        from Forms.edit import active_form_session

        document = App.newDocument("FormsTestExternalMatchCommand")
        obj = create_box(document)
        document.recompute()
        delete_faces(obj, [0])
        cage = ControlCage.from_object(obj)
        loop = cage.boundary_loops()[0]
        target_points = [
            App.Vector(*cage.vertices[index]).add(App.Vector(0, 0, 4)) for index in loop
        ]
        support = document.addObject("Part::Feature", "UnrelatedSupport")
        support.Shape = Part.Face(Part.makePolygon(target_points + [target_points[0]]))
        original_area = support.Shape.Area
        document.recompute()

        gui_document = Gui.getDocument(document.Name)
        try:
            gui_document.setEdit(obj, 0)
            QtWidgets.QApplication.processEvents()
            session = active_form_session(obj)
            self.assertIsNotNone(session)
            mapper = ControlElementMapper(obj)
            boundary_edge = cage.boundary_edges[0]
            edge_names = [
                f"Edge{index}"
                for index, edge in enumerate(obj.Shape.Edges, 1)
                if tuple(sorted(mapper.indices(edge))) == boundary_edge
            ]
            self.assertTrue(edge_names)
            session._select_edge_loop(edge_names[0])
            Gui.Selection.addSelection(support, "Face1")
            QtWidgets.QApplication.processEvents()

            command = CommandTopology.CommandMatch()
            self.assertTrue(command.IsActive())
            command.Activated()
            self.assertTrue(session.match_tool_active)
            self.assertFalse(session.match_preview_shape.isNull())
            self.assertIsNotNone(session.match_preview_root)
            self.assertFalse(obj.ViewObject.Visibility)
            self.assertFalse(obj.MatchBoundary)
            self.assertEqual(
                {session.match_mode.itemData(index) for index in range(session.match_mode.count())},
                {"AdjacentFaces", "SelectedFace", "Connected"},
            )
            session.match_mode.setCurrentIndex(session.match_mode.findData("SelectedFace"))
            QtWidgets.QApplication.processEvents()
            self.assertFalse(session.match_preview_shape.isNull())
            self.assertFalse(obj.MatchBoundary)
            session.apply_match_tool()
            self.assertEqual(obj.MatchSupport[0], support)
            self.assertEqual(obj.MatchTangentMode, "SelectedFace")
            self.assertTrue(obj.ViewObject.Visibility)
            self.assertAlmostEqual(support.Shape.Area, original_area, places=9)
            for index in obj.MatchBoundary:
                distance, _points, _info = Part.Vertex(obj.ControlPoints[index]).distToShape(
                    support.Shape
                )
                self.assertLess(distance, 1.0e-7)
        finally:
            gui_document.resetEdit()

    def test_delete_faces_compacts_vertices_and_remaps_sharpness(self):
        vertices, faces = face_control_cage(20, 10, 2, 1)
        cage = ControlCage(
            vertices,
            faces,
            [float(index) for index in range(len(vertices))],
            {cage_edges(faces)[-1]: 4.0},
        )
        edited = cage.delete_faces([0])

        self.assertEqual(len(edited.faces), 1)
        self.assertEqual(len(edited.vertices), 4)
        self.assertTrue(all(index < 4 for index in edited.faces[0]))
        self.assertEqual(len(edited.vertex_sharpness), 4)
        self.assertTrue(set(edited.edge_sharpness).issubset(set(cage_edges(edited.faces))))

    def test_invalid_second_face_deletion_leaves_the_form_unchanged(self):
        document = App.newDocument("FormsTestRejectedFaceDeletion")
        obj = create_face(document)
        obj.XSegments = 5
        obj.YSegments = 5
        document.recompute()
        delete_faces(obj, [0])
        document.recompute()
        points_before = list(obj.ControlPoints)
        faces_before = list(obj.ControlFaces)
        shape_hash_before = obj.Shape.hashCode()

        with self.assertRaisesRegex(ValueError, "boundaries meeting at a vertex"):
            delete_faces(obj, [5])

        self.assertEqual(list(obj.ControlPoints), points_before)
        self.assertEqual(list(obj.ControlFaces), faces_before)
        self.assertEqual(obj.Shape.hashCode(), shape_hash_before)

    def test_generated_faces_map_back_to_unique_control_faces(self):
        document = App.newDocument("FormsTestFaceMapping")
        obj = create_box(document)
        document.recompute()
        cage = ControlCage.from_object(obj)
        mapped = {
            cage.face_index(control_indices_for_element(obj, face)) for face in obj.Shape.Faces
        }
        self.assertEqual(mapped, set(range(len(cage.faces))))

    def test_logical_element_names_survive_control_point_changes(self):
        document = App.newDocument("FormsTestStableElementNames")
        obj = create_box(document)
        document.recompute()
        original_names = {str(name) for name in obj.Shape.ElementMap}
        self.assertTrue(original_names)

        make_editable(obj)
        points = list(obj.ControlPoints)
        points[0] = points[0].add(App.Vector(1.0, 2.0, 0.5))
        obj.ControlPoints = points
        document.recompute()

        edited_names = {str(name) for name in obj.Shape.ElementMap}
        self.assertEqual(edited_names, original_names)

    def test_logical_face_names_use_forms_opcode(self):
        document = App.newDocument("FormsTestElementNameOpcode")
        obj = create_box(document)
        document.recompute()

        names = [str(name) for name in obj.Shape.ElementMap if str(name).startswith("FormsFace")]
        self.assertEqual(len(names), len(obj.Shape.Faces))
        self.assertTrue(all(name.split(";", 2)[1] == "FRM" for name in names))

    def test_delete_face_is_undoable_as_one_document_transaction(self):
        document = App.newDocument("FormsTestDeleteUndo")
        obj = create_box(document)
        document.recompute()
        document.openTransaction("Delete form face")
        delete_faces(obj, [0])
        document.recompute()
        document.commitTransaction()
        self.assertEqual(ControlCage.from_object(obj).is_closed, False)

        document.undo()
        document.recompute()
        self.assertTrue(ControlCage.from_object(obj).is_closed)
        self.assertEqual(obj.Shape.ShapeType, "Solid")

    def test_fill_hole_restores_a_deleted_quad_and_solid_output(self):
        document = App.newDocument("FormsTestFillHole")
        obj = create_box(document)
        document.recompute()
        delete_faces(obj, [0])
        document.recompute()
        open_cage = ControlCage.from_object(obj)

        fill_holes(obj, [open_cage.boundary_edges[0]])
        document.recompute()

        filled = ControlCage.from_object(obj)
        self.assertTrue(filled.is_closed)
        self.assertEqual(filled.boundary_edges, [])
        self.assertEqual(obj.Shape.ShapeType, "Solid")
        self.assertTrue(obj.Shape.isValid())
        self.assertEqual(obj.ConversionStatus, "Valid solid")

    def test_erase_and_fill_removes_an_extrusion_and_restores_a_valid_solid(self):
        document = App.newDocument("FormsTestEraseAndFill")
        obj = create_box(document)
        make_editable(obj)
        original = ControlCage.from_object(obj)
        extruded, _top, sides = original.extrude_face(0)
        extruded.write(obj)
        document.recompute()

        erase_and_fill(obj, (0,) + sides)
        document.recompute()

        restored = ControlCage.from_object(obj)
        self.assertEqual(len(restored.vertices), len(original.vertices))
        self.assertEqual(len(restored.faces), len(original.faces))
        self.assertTrue(restored.is_closed)
        self.assertEqual(obj.Shape.ShapeType, "Solid")
        self.assertTrue(obj.Shape.isValid())

    def test_reduced_star_fills_an_even_multi_face_boundary_with_quads(self):
        vertices, faces = box_control_cage(20, 20, 20, 2, 2, 2)
        cage = ControlCage(vertices, faces).delete_faces([0, 2])
        loops = cage.boundary_loops()
        self.assertEqual([len(loop) for loop in loops], [6])

        filled = cage.fill_boundaries([cage.boundary_edges[0]])

        self.assertTrue(filled.is_closed)
        self.assertEqual(len(filled.vertices), len(cage.vertices) + 1)
        self.assertTrue(all(len(face) == 4 for face in filled.faces))

    def test_insert_edge_ring_keeps_a_closed_all_quad_cage(self):
        vertices, faces = box_control_cage(20, 20, 20)
        cage = ControlCage(vertices, faces)
        selected = cage_edges(faces)[0]

        inserted, new_edges = cage.insert_edge_ring(selected, 0.25)

        self.assertTrue(inserted.is_closed)
        self.assertEqual(len(inserted.vertices), 12)
        self.assertEqual(len(inserted.faces), 10)
        self.assertEqual(len(new_edges), 4)
        self.assertTrue(new_edges.issubset(set(cage_edges(inserted.faces))))
        self.assertTrue(all(len(face) == 4 for face in inserted.faces))

    def test_extruded_face_converts_to_a_valid_solid(self):
        vertices, faces = box_control_cage(20, 20, 20)
        cage, top, _side_faces = ControlCage(vertices, faces).extrude_face(0)
        face = faces[0]
        first = App.Vector(*vertices[face[0]])
        second = App.Vector(*vertices[face[1]])
        third = App.Vector(*vertices[face[2]])
        normal = second.sub(first).cross(third.sub(first))
        normal.normalize()
        moved = list(cage.vertices)
        for index in top:
            moved[index] = tuple(App.Vector(*moved[index]).add(normal.multiply(5.0)))

        solid, _deviation, _level = cage_to_solid(
            moved,
            cage.faces,
            0.05,
            3,
            cage.edge_sharpness,
            cage.vertex_sharpness,
        )

        self.assertEqual(solid.ShapeType, "Solid")
        self.assertTrue(solid.isValid())
        self.assertGreater(solid.Volume, 0.0)

    def test_extruded_surface_boundary_converts_to_a_valid_surface(self):
        vertices, faces = face_control_cage(20, 20, 1, 1)
        cage = ControlCage(vertices, faces)
        cage, outer_edges, side_faces = cage.extrude_boundary_edges(cage.boundary_edges[:2])
        moved = list(cage.vertices)
        outer_vertices = {vertex for edge in outer_edges for vertex in edge}
        for index in outer_vertices:
            point = App.Vector(*moved[index])
            moved[index] = tuple(point.add(App.Vector(0.0, 0.0, 5.0)))

        surface, _deviation, _level = cage_to_surface(
            moved,
            cage.faces,
            0.05,
            3,
            cage.edge_sharpness,
            cage.vertex_sharpness,
        )

        self.assertFalse(surface.isNull())
        self.assertTrue(surface.isValid())
        self.assertEqual(len(outer_edges), 2)
        self.assertEqual(len(side_faces), 2)

    def test_bridged_quad_boundaries_convert_to_a_valid_solid(self):
        vertices = [
            (-5, -5, -5),
            (5, -5, -5),
            (5, 5, -5),
            (-5, 5, -5),
            (-5, -5, 5),
            (5, -5, 5),
            (5, 5, 5),
            (-5, 5, 5),
        ]
        cage = ControlCage(vertices, [(0, 3, 2, 1), (4, 5, 6, 7)])
        loops = cage.boundary_loops()
        cage = cage.bridge_boundaries(
            [
                tuple(sorted((loops[0][0], loops[0][1]))),
                tuple(sorted((loops[1][0], loops[1][1]))),
            ]
        )

        solid, _deviation, _level = cage_to_solid(cage.vertices, cage.faces, 0.05, 3)

        self.assertEqual(solid.ShapeType, "Solid")
        self.assertTrue(solid.isValid())

    def test_local_insert_adds_one_editable_brep_face_without_refining_base_cage(self):
        document = App.newDocument("FormsTestLocalizedInsert")
        obj = create_box(document)
        document.recompute()
        original = ControlCage.from_object(obj)
        original_shape = obj.Shape.copy()
        selected = cage_edges(original.faces)[0]

        _obj, chosen_faces = insert_edge(obj, selected, 0.5, "left")
        document.recompute()

        edited = ControlCage.from_object(obj)
        self.assertEqual(edited.vertices, original.vertices)
        self.assertEqual(edited.faces, original.faces)
        self.assertEqual(len(chosen_faces), 1)
        self.assertTrue(obj.TMeshData)
        self.assertFalse(obj.LocalEdgeInserts)
        self.assertEqual(len(obj.LocalControlPoints), 2)
        self.assertTrue(edited.is_closed)
        self.assertEqual(obj.Shape.ShapeType, "Solid")
        self.assertTrue(obj.Shape.isValid())
        self.assertEqual(len(obj.Shape.Faces), len(original_shape.Faces) + 1)
        self.assertEqual(len(obj.Shape.Edges), len(original_shape.Edges) + 3)
        self.assertAlmostEqual(obj.Shape.Volume, original_shape.Volume, places=2)
        mapper = ControlElementMapper(obj)
        base_count = len(obj.ControlPoints)
        inserted_edges = [
            edge
            for edge in obj.Shape.Edges
            if all(index >= base_count for index in mapper.target(edge)[0])
        ]
        self.assertEqual(len(inserted_edges), 1)
        indices, anchor = mapper.target(inserted_edges[0])
        self.assertEqual(set(indices), {base_count, base_count + 1})
        self.assertIsNone(anchor)
        for face in obj.Shape.Faces:
            indices, _anchor = mapper.target(face)
            self.assertTrue(indices)

        old_shape = obj.Shape.copy()
        old_second = App.Vector(obj.LocalControlPoints[1])
        local = list(obj.LocalControlPoints)
        local[0] = local[0].add(App.Vector(-2.0, 0.0, 0.0))
        obj.LocalControlPoints = local
        document.recompute()
        self.assertEqual(obj.ConversionStatus, "Valid solid")
        self.assertTrue(obj.Shape.isValid())
        self.assertGreater(abs(obj.Shape.Volume - old_shape.Volume), 1.0e-4)
        self.assertEqual(obj.LocalControlPoints[1], old_second)
        mapper = ControlElementMapper(obj)
        self.assertTrue(
            any(
                set(mapper.indices(edge)) == {base_count, base_count + 1}
                for edge in obj.Shape.Edges
            )
        )

    def test_local_tmesh_crease_is_rejected_and_legacy_value_can_be_cleared(self):
        document = App.newDocument("FormsTestLocalCrease")
        obj = create_box(document)
        document.recompute()
        cage = ControlCage.from_object(obj)
        insert_edge(obj, cage_edges(cage.faces)[0], 0.5, "left")
        document.recompute()

        mesh = HierarchicalTMesh.decode(obj.TMeshData)
        base_count = len(obj.ControlPoints)
        local_edge = next(
            edge for edge in mesh.atomic_edges() if all(index >= base_count for index in edge)
        )
        with self.assertRaisesRegex(ValueError, "not supported"):
            set_edge_crease(obj, {local_edge}, 10.0)
        self.assertNotIn(local_edge, ControlCage.from_object(obj).edge_sharpness)
        obj.EdgeSharpness = [f"{local_edge[0]} {local_edge[1]} 10"]

        set_edge_crease(obj, {local_edge}, 0.0)
        document.recompute()
        self.assertNotIn(local_edge, ControlCage.from_object(obj).edge_sharpness)
        self.assertTrue(obj.Shape.isValid())

    def test_hover_insert_resolves_both_orientations_and_allows_nested_splits(self):
        document = App.newDocument("FormsTestHoverInsert")
        obj = create_box(document)
        document.recompute()
        cage = ControlCage.from_object(obj)

        first_edge, first_targets, first_side = local_insert_target(cage, 0, 0, "left")
        second_edge, second_targets, second_side = local_insert_target(cage, 0, 1, "left")

        self.assertNotEqual(first_edge, second_edge)
        self.assertEqual(first_targets, (0,))
        self.assertEqual(second_targets, (0,))
        self.assertIn(first_side, ("left", "right"))
        self.assertIn(second_side, ("left", "right"))

        insert_edge_on_face(obj, 0, 0, "left")
        document.recompute()
        first_count = len(obj.Shape.Faces)
        self.assertTrue(obj.TMeshData)
        self.assertTrue(obj.Shape.isValid())
        mesh = HierarchicalTMesh.decode(obj.TMeshData)
        insert_edge_on_face(obj, 0, 1, "left")
        document.recompute()
        self.assertGreater(len(HierarchicalTMesh.decode(obj.TMeshData).faces), len(mesh.faces))
        self.assertEqual(len(obj.Shape.Faces), first_count + 1)
        self.assertTrue(obj.Shape.isValid())

    def test_local_insert_both_sides_stays_local(self):
        document = App.newDocument("FormsTestLocalizedInsertBoth")
        obj = create_box(document)
        obj.XSegments = 1
        obj.YSegments = 1
        obj.ZSegments = 1
        document.recompute()
        cage = ControlCage.from_object(obj)
        original_face_count = len(obj.Shape.Faces)
        selected = cage_edges(cage.faces)[0]

        _obj, chosen_faces = insert_edge(obj, selected, 0.5, "both")
        document.recompute()

        self.assertEqual(len(chosen_faces), 2)
        self.assertTrue(obj.TMeshData)
        self.assertFalse(obj.LocalEdgeInserts)
        self.assertEqual(len(obj.Shape.Faces), original_face_count + 2)
        self.assertTrue(obj.Shape.isValid())

    def test_symmetric_local_insert_creates_and_pairs_opposite_controls(self):
        document = App.newDocument("FormsTestSymmetricInsert")
        obj = create_box(document)
        obj.Symmetric = True
        obj.SymmetryPlane = "YZ"
        document.recompute()
        cage = ControlCage.from_object(obj)
        mesh = HierarchicalTMesh.from_quad_cage(cage.vertices, cage.faces)
        face_id = next(
            candidate
            for candidate in mesh.faces
            if len(
                mirror_faces(
                    mesh.vertices,
                    {index: face.boundary for index, face in mesh.faces.items()},
                    [candidate],
                    0,
                )
            )
            == 2
        )
        edge = mesh.faces[face_id].sides[0][:2]

        insert_edge(obj, edge, 0.5, "left")
        document.recompute()

        edited = HierarchicalTMesh.decode(obj.TMeshData)
        mapping = vertex_map(edited.vertices, 0)
        local_ids = range(len(cage.vertices), edited.next_vertex_id)
        self.assertTrue(local_ids)
        self.assertTrue(all(mapping[index] in local_ids for index in local_ids))
        self.assertEqual(len(edited.faces), len(mesh.faces) + 2)
        self.assertEqual(obj.ConversionStatus, "Valid solid")

    def test_symmetric_delete_face_opens_both_sides(self):
        document = App.newDocument("FormsTestSymmetricHole")
        obj = create_box(document)
        obj.Symmetric = True
        obj.SymmetryPlane = "YZ"
        document.recompute()
        cage = ControlCage.from_object(obj)
        face_id = next(
            candidate
            for candidate in range(len(cage.faces))
            if len(mirror_faces(enumerate(cage.vertices), enumerate(cage.faces), [candidate], 0))
            == 2
        )

        delete_faces(obj, [face_id])
        document.recompute()

        self.assertEqual(len(ControlCage.from_object(obj).faces), len(cage.faces) - 2)
        self.assertEqual(obj.ConversionStatus, "Valid surface")
        self.assertTrue(obj.Shape.isValid())

    def test_symmetric_delete_logical_face_keeps_tmesh_hole_editable(self):
        document = App.newDocument("FormsTestSymmetricTMeshHole")
        obj = create_box(document)
        obj.Symmetric = True
        obj.SymmetryPlane = "YZ"
        document.recompute()
        cage = ControlCage.from_object(obj)
        mesh = HierarchicalTMesh.from_quad_cage(cage.vertices, cage.faces)
        face_id = next(
            candidate
            for candidate in mesh.faces
            if len(
                mirror_faces(
                    mesh.vertices,
                    {index: face.boundary for index, face in mesh.faces.items()},
                    [candidate],
                    0,
                )
            )
            == 2
        )
        insert_edge(obj, mesh.faces[face_id].sides[0][:2])
        document.recompute()
        edited = HierarchicalTMesh.decode(obj.TMeshData)
        logical_face = next(
            candidate
            for candidate in edited.faces
            if len(
                mirror_faces(
                    edited.vertices,
                    {index: face.boundary for index, face in edited.faces.items()},
                    [candidate],
                    0,
                )
            )
            == 2
        )

        delete_faces(obj, [logical_face])
        document.recompute()

        opened = HierarchicalTMesh.decode(obj.TMeshData)
        self.assertEqual(len(opened.faces), len(edited.faces) - 2)
        self.assertFalse(opened.is_closed)
        self.assertEqual(obj.ConversionStatus, "Valid surface")
        self.assertTrue(obj.Shape.isValid())

    def test_subdivide_adds_only_four_logical_leaf_faces(self):
        document = App.newDocument("FormsTestSubdivide")
        obj = create_box(document)
        document.recompute()
        original_face_count = len(obj.Shape.Faces)

        _obj, descendants = subdivide_faces(obj, [0])
        document.recompute()

        self.assertEqual(len(descendants), 4)
        self.assertEqual(len(obj.Shape.Faces), original_face_count + 3)
        self.assertEqual(len(obj.LocalControlPoints), 5)
        self.assertTrue(obj.Shape.isValid())
        mapper = ControlElementMapper(obj)
        self.assertTrue(all(mapper.indices(face) for face in obj.Shape.Faces))

    def _attach_lcs_to_form_faces(self, obj):
        mapper = ControlElementMapper(obj)
        attachments = {}
        for index, face in enumerate(obj.Shape.Faces, 1):
            logical_id = mapper.cage.face_index(mapper.indices(face))
            lcs = obj.Document.addObject("PartDesign::CoordinateSystem", "LCS")
            lcs.AttachmentSupport = [(obj, (f"Face{index}",))]
            # Form patches are curved; FlatFace would make the attachment invalid.
            lcs.MapMode = "InertialCS"
            attachments[logical_id] = lcs.Name
        obj.Document.recompute()
        return attachments

    def _assert_lcs_logical_faces(self, obj, attachments):
        mapper = ControlElementMapper(obj)
        for logical_id, name in attachments.items():
            with self.subTest(logical_face=logical_id):
                lcs = obj.Document.getObject(name)
                self.assertNotIn("Invalid", lcs.State)
                support, subelements = lcs.AttachmentSupport[0]
                self.assertEqual(support, obj)
                self.assertEqual(len(subelements), 1)
                face = obj.Shape.getElement(subelements[0])
                controls = set(mapper.indices(face))
                expected = (
                    mapper.mesh.faces[logical_id].boundary
                    if mapper.mesh is not None
                    else mapper.cage.faces[logical_id]
                )
                self.assertEqual(controls, set(expected))

    def test_subdivision_preserves_lcs_face_identity(self):
        document = App.newDocument("FormsTestSubdivisionLCS")
        obj = create_box(document)
        document.recompute()
        attachments = self._attach_lcs_to_form_faces(obj)
        self._assert_lcs_logical_faces(obj, attachments)

        # Include the split faces: one child deliberately inherits the parent's
        # ID. Unrelated faces must keep their identities even if OCC reorders them.
        for face_id, u_count, v_count in ((0, 2, 2), (2, 4, 2), (0, 2, 1)):
            subdivide_faces(obj, [face_id], u_count, v_count)
            document.recompute()
            self._assert_lcs_logical_faces(obj, attachments)

    def test_subdivision_lcs_support_survives_undo_redo(self):
        document = App.newDocument("FormsTestSubdivisionLCSUndo")
        obj = create_box(document)
        document.recompute()
        attachments = self._attach_lcs_to_form_faces(obj)
        document.openTransaction("Subdivide form")
        subdivide_faces(obj, [0])
        document.recompute()
        document.commitTransaction()
        self._assert_lcs_logical_faces(obj, attachments)

        document.undo()
        document.recompute()
        self.assertFalse(obj.TMeshData)
        self._assert_lcs_logical_faces(obj, attachments)
        document.redo()
        document.recompute()
        self.assertTrue(obj.TMeshData)
        self._assert_lcs_logical_faces(obj, attachments)

    def test_subdivision_lcs_support_survives_save_restore(self):
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "form.FCStd")
            document = App.newDocument("FormsTestSubdivisionLCSSave")
            obj = create_box(document)
            document.recompute()
            attachments = self._attach_lcs_to_form_faces(obj)
            name = obj.Name

            for face_id in (0, 2):
                document.saveAs(path)
                App.closeDocument(document.Name)
                document = App.openDocument(path)
                obj = document.getObject(name)
                obj.touch()
                document.recompute()
                self._assert_lcs_logical_faces(obj, attachments)
                subdivide_faces(obj, [face_id])
                document.recompute()
                self._assert_lcs_logical_faces(obj, attachments)
            App.closeDocument(document.Name)

    def test_subdivide_supports_independent_dyadic_counts(self):
        document = App.newDocument("FormsTestRectangularSubdivide")
        obj = create_box(document)
        document.recompute()
        original_face_count = len(obj.Shape.Faces)

        _obj, descendants = subdivide_faces(obj, [0], 4, 2)
        document.recompute()

        self.assertEqual(len(descendants), 8)
        self.assertEqual(len(obj.Shape.Faces), original_face_count + 7)
        self.assertTrue(obj.Shape.isValid())
        with self.assertRaisesRegex(ValueError, "powers of two"):
            subdivide_faces(obj, [1], 3, 2)

    def test_local_insert_controls_survive_save_and_restore(self):
        handle, path = tempfile.mkstemp(suffix=".FCStd")
        os.close(handle)
        try:
            document = App.newDocument("FormsTestLocalSave")
            obj = create_box(document)
            document.recompute()
            cage = ControlCage.from_object(obj)
            insert_edge(obj, cage_edges(cage.faces)[0], 0.5, "left")
            local = list(obj.LocalControlPoints)
            local[0] = local[0].add(App.Vector(-1.0, 0.5, 0.25))
            obj.LocalControlPoints = local
            document.recompute()
            expected = [App.Vector(point) for point in obj.LocalControlPoints]
            expected_face_count = len(obj.Shape.Faces)
            document.saveAs(path)
            App.closeDocument(document.Name)

            restored = App.openDocument(path)
            restored.recompute()
            restored_obj = restored.getObject("FormBox")
            self.assertEqual(
                [App.Vector(point) for point in restored_obj.LocalControlPoints],
                expected,
            )
            self.assertEqual(restored_obj.ConversionStatus, "Valid solid")
            self.assertEqual(len(restored_obj.Shape.Faces), expected_face_count)
        finally:
            if os.path.exists(path):
                os.remove(path)

    def test_insert_edge_preserves_split_edge_sharpness_and_valid_solid(self):
        document = App.newDocument("FormsTestInsertEdge")
        obj = create_box(document)
        obj.XSegments = 1
        obj.YSegments = 1
        obj.ZSegments = 1
        document.recompute()
        cage = ControlCage.from_object(obj)
        selected = cage_edges(cage.faces)[0]
        make_editable(obj)
        obj.EdgeSharpness = [f"{selected[0]} {selected[1]} 5"]
        document.recompute()

        _obj, new_edges = insert_edge_loop(obj, selected, 0.25)
        document.recompute()

        edited = ControlCage.from_object(obj)
        split_sharpness = [
            value
            for edge, value in edited.edge_sharpness.items()
            if selected[0] in edge or selected[1] in edge
        ]
        self.assertEqual(split_sharpness.count(5.0), 2)
        self.assertTrue(new_edges.issubset(set(cage_edges(edited.faces))))
        self.assertTrue(edited.is_closed)
        self.assertEqual(obj.Shape.ShapeType, "Solid")
        self.assertTrue(obj.Shape.isValid())

    def test_insert_edge_is_undoable_as_one_document_transaction(self):
        document = App.newDocument("FormsTestInsertUndo")
        obj = create_box(document)
        document.recompute()
        original_face_count = len(ControlCage.from_object(obj).faces)
        edge = cage_edges(ControlCage.from_object(obj).faces)[0]
        document.openTransaction("Insert form edge")
        insert_edge_loop(obj, edge)
        document.recompute()
        document.commitTransaction()
        self.assertGreater(len(ControlCage.from_object(obj).faces), original_face_count)

        document.undo()
        document.recompute()
        self.assertEqual(len(ControlCage.from_object(obj).faces), original_face_count)


def suite():
    return unittest.defaultTestLoader.loadTestsFromTestCase(BRepConversionTest)
