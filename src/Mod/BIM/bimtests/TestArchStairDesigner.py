# SPDX-License-Identifier: LGPL-2.1-or-later

"""Tests for the BIM Stair Designer object model."""

import math

import FreeCAD
import Part

from bimtests import TestArchBase
from stairdesigner import make_stair
from stairdesigner.geometry import (
    BalancedSection,
    _circular_stringer_data,
    _section_band_faces,
    _stringer_elevations,
    balanced_partition_is_valid,
    balanced_tread_faces,
    balanced_winder_sections,
    distribute_treads,
    fit_balanced_sections_to_footprint,
    fit_tangent_sections_to_footprint,
    make_balanced_concrete_shape,
    make_balanced_riser_shape,
    make_balanced_tread_shape,
    make_housed_stringer_shape,
    make_stair_footprint,
    make_tangent_stair_footprint,
    straight_stair_metrics,
    stringer_flight_runs,
    tangent_flight_sections,
    tangent_tread_faces,
)
from stairdesigner.objects import (
    StairProxy,
    ViewProviderComponentGroup,
    get_flights,
    resize_flights,
    sync_all_flight_side_lengths,
)


class TestArchStairDesigner(TestArchBase.TestArchBase):

    def test_component_group_icons_are_available(self):
        import Arch_rc
        from PySide import QtCore

        paths = {
            "handrails": ":/icons/Arch_Handrail_Tree.svg",
            "stringers": ":/icons/Arch_Stringer_Tree.svg",
        }
        for path in (
            ":/icons/Arch_Handrail.svg",
            ":/icons/Arch_Stringer.svg",
            *paths.values(),
        ):
            self.assertTrue(QtCore.QResource(path).isValid(), path)

        provider = ViewProviderComponentGroup.__new__(ViewProviderComponentGroup)
        provider.Object = type("ComponentGroup", (), {})()
        for section, path in paths.items():
            provider.Object.PanelSection = section
            self.assertEqual(provider.getIcon(), path)

    def test_straight_metrics(self):
        metrics = straight_stair_metrics(2800.0, 3500.0, 15)
        self.assertEqual(metrics.tread_count, 14)
        self.assertAlmostEqual(metrics.riser_height, 2800.0 / 15.0)
        self.assertAlmostEqual(metrics.tread_width, 250.0)
        self.assertAlmostEqual(metrics.blondel_value, 623.3333333333334)
        self.assertTrue(metrics.blondel_compliant)

    def test_deprecated_model_property_is_removed(self):
        stair = self.document.addObject("App::FeaturePython", "Stair")
        StairProxy(stair)
        stair.addProperty("App::PropertyString", "Model", "Stair")
        stair.Model = "Quarter turn"

        stair.Proxy.set_properties(stair)

        self.assertNotIn("Model", stair.PropertiesList)

    def test_treads_are_distributed_over_flights(self):
        self.assertEqual(distribute_treads([2000.0, 1000.0], 9), [6, 3])
        self.assertEqual(distribute_treads([1000.0, 1000.0, 1000.0], 8), [3, 3, 2])

    def test_flight_side_lengths_are_linked(self):
        stair = make_stair(flight_length=1600.0, width=800.0)
        flights = resize_flights(stair, 2, length=1600.0, width=800.0, rotations=["Right"])

        self.assertAlmostEqual(flights[0].LeftLength.Value, 2000.0)
        self.assertAlmostEqual(flights[0].RightLength.Value, 1200.0)
        self.assertAlmostEqual(flights[1].LeftLength.Value, 2000.0)
        self.assertAlmostEqual(flights[1].RightLength.Value, 1200.0)
        flights[0].RightLength = 1300.0
        self.assertAlmostEqual(flights[0].LeftLength.Value, 2100.0)
        flights[0].LeftLength = 2300.0
        self.assertAlmostEqual(flights[0].RightLength.Value, 1500.0)

        flights[1].Width = 1000.0
        self.assertAlmostEqual(flights[0].LeftLength.Value, 2400.0)
        self.assertAlmostEqual(flights[0].RightLength.Value, 1400.0)
        self.assertAlmostEqual(flights[1].LeftLength.Value, 2000.0)
        self.assertAlmostEqual(flights[1].RightLength.Value, 1200.0)
        flights[1].Rotation = "Left"
        self.assertAlmostEqual(flights[0].LeftLength.Value, 1400.0)
        self.assertAlmostEqual(flights[0].RightLength.Value, 2400.0)
        self.assertAlmostEqual(flights[1].LeftLength.Value, 1200.0)
        self.assertAlmostEqual(flights[1].RightLength.Value, 2000.0)

        flights[1].Angle = 45.0
        expected_difference = (1000.0 - 800.0 * math.cos(math.radians(45.0))) / math.sin(
            math.radians(45.0)
        )
        self.assertAlmostEqual(
            flights[0].RightLength.Value - flights[0].LeftLength.Value,
            expected_difference,
        )
        expected_incoming_difference = (800.0 - 1000.0 * math.cos(math.radians(45.0))) / math.sin(
            math.radians(45.0)
        )
        self.assertAlmostEqual(
            flights[1].RightLength.Value - flights[1].LeftLength.Value,
            expected_incoming_difference,
        )
        flights[1].Angle = 90.0

        flights[1].LeftLength = 1400.0
        self.assertAlmostEqual(flights[1].RightLength.Value, 2200.0)

        flights[1].FlightType = "Circular"
        self.assertAlmostEqual(flights[1].Angle.Value, 90.0)
        self.assertIn("Hidden", flights[1].getEditorMode("LeftLength"))
        self.assertIn("Hidden", flights[1].getEditorMode("RightLength"))
        self.assertNotIn("Hidden", flights[1].getEditorMode("InnerRadius"))
        self.assertNotIn("Hidden", flights[1].getEditorMode("OuterRadius"))
        self.assertAlmostEqual(flights[0].LeftLength.Value, 1900.0)
        self.assertAlmostEqual(flights[0].RightLength.Value, 1900.0)
        self.assertAlmostEqual(flights[1].InnerRadius.Value, 500.0)
        self.assertAlmostEqual(flights[1].OuterRadius.Value, 1500.0)
        flights[1].InnerRadius = 650.0
        self.assertAlmostEqual(flights[1].OuterRadius.Value, 1650.0)
        flights[1].OuterRadius = 1800.0
        self.assertAlmostEqual(flights[1].InnerRadius.Value, 800.0)
        flights[1].Width = 1100.0
        self.assertAlmostEqual(flights[1].OuterRadius.Value, 1900.0)
        flights[1].Angle = 60.0
        self.assertAlmostEqual(flights[1].Angle.Value, 60.0)
        inner_radius = flights[1].InnerRadius.Value
        outer_radius = flights[1].OuterRadius.Value
        flights[1].Rotation = "Right"
        self.assertAlmostEqual(
            flights[1].InnerRadius.Value,
            inner_radius,
        )
        self.assertAlmostEqual(
            flights[1].OuterRadius.Value,
            outer_radius,
        )
        self.assertEqual(stair.GeometryStatus, "Tangential circular stair")
        circular_treads = [
            child
            for child in stair.StepsGroup.Group
            if getattr(child, "StairDesignerRole", "") == "Tread"
        ]
        self.assertTrue(all(tread.Shape.isValid() for tread in circular_treads))
        self.assertTrue(all(len(tread.Shape.Solids) == 1 for tread in circular_treads))
        self.assertTrue(
            any(isinstance(item, Part.ArcOfCircle) for item in stair.PlanSketch.Geometry)
        )

    def test_winding_parameters_belong_to_junction_flights(self):
        stair = make_stair(flight_length=1800.0, width=800.0)
        flights = resize_flights(stair, 3, length=1800.0, width=800.0)

        self.assertNotIn("WalkingLineOffset", stair.PropertiesList)
        self.assertNotIn("WindingCoefficient", stair.PropertiesList)
        self.assertEqual(stair.StepsGroup.PanelSection, "steps")
        for flight in flights:
            self.assertIn("WindingLocal", flight.PropertiesList)
            self.assertIn("WindingDistant", flight.PropertiesList)
            self.assertEqual(flight.WindingLocal, 50)
            self.assertEqual(flight.WindingDistant, 50)
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.assertIn("Hidden", flights[0].getEditorMode("WindingLocal"))
        self.assertNotIn("Hidden", flights[1].getEditorMode("WindingLocal"))

        flights[1].WindingLocal = 80
        flights[2].WindingDistant = 20
        self.document.recompute()
        self.assertTrue(
            all(
                not child.Shape.isNull()
                for child in stair.StepsGroup.Group
                if hasattr(child, "Shape")
            )
        )

    def test_endpoint_angles_and_side_directions(self):
        width = 1000.0
        left_length = 2500.0
        difference = width * math.tan(math.radians(30.0))
        center_length = left_length + difference / 2.0
        specs = [(center_length, width, 0.0)]

        angled_footprint = make_stair_footprint(specs, 30.0, 0.0)
        angled_sections, _going = balanced_winder_sections(
            specs,
            12,
            1.0,
            None,
            30.0,
            0.0,
        )
        angled_sections = fit_balanced_sections_to_footprint(angled_sections, angled_footprint)
        angled_faces = balanced_tread_faces(angled_sections, angled_footprint)
        self.assertTrue(balanced_partition_is_valid(angled_faces, angled_footprint, 12))
        self.assertAlmostEqual(
            angled_sections[0].left[0] - angled_sections[0].right[0],
            difference,
        )
        self.assertTrue(all(abs(section.tangent[1]) < 1e-7 for section in angled_sections[6:]))

        rectangular_footprint = make_stair_footprint([(3500.0, width, 0.0)])
        side_sections, _going = balanced_winder_sections(
            [(3500.0, width, 0.0)],
            14,
            1.0,
            None,
            0.0,
            0.0,
            "From right",
            "To left",
        )
        side_sections = fit_balanced_sections_to_footprint(side_sections, rectangular_footprint)
        side_faces = balanced_tread_faces(side_sections, rectangular_footprint)
        self.assertTrue(balanced_partition_is_valid(side_faces, rectangular_footprint, 14))
        self.assertAlmostEqual(side_sections[0].tangent[0], 0.0)
        self.assertAlmostEqual(side_sections[0].tangent[1], 1.0)
        self.assertAlmostEqual(side_sections[-1].tangent[0], 0.0)
        self.assertAlmostEqual(side_sections[-1].tangent[1], 1.0)
        self.assertTrue(
            any(
                section.locked_to_flight and abs(section.tangent[0] - 1.0) < 1e-7
                for section in side_sections
            )
        )
        first_tread = make_balanced_tread_shape(
            side_sections[0],
            side_sections[1],
            rectangular_footprint,
            40.0,
            40.0,
            30.0,
            28.0,
            side_faces[0],
        )
        first_riser = make_balanced_riser_shape(
            side_sections[0],
            0.0,
            100.0,
            18.0,
            rectangular_footprint,
        )
        self.assertTrue(first_tread.isValid())
        self.assertEqual(len(first_tread.Solids), 1)
        self.assertAlmostEqual(first_tread.BoundBox.YMin, -30.0)
        self.assertTrue(first_riser.isValid())
        self.assertEqual(len(first_riser.Solids), 1)
        self.assertAlmostEqual(first_riser.BoundBox.XLength, width)
        self.assertAlmostEqual(first_riser.BoundBox.YLength, 18.0)

        stair = make_stair(
            flight_length=left_length,
            width=width,
            steps=13,
        )
        flight = get_flights(stair)[0]
        flight.StartAngle = 30.0
        self.assertAlmostEqual(flight.LeftLength.Value, left_length)
        self.assertAlmostEqual(flight.RightLength.Value, left_length + difference)
        flight.EndAngle = 10.0
        self.assertAlmostEqual(flight.LeftLength.Value, left_length)
        self.assertAlmostEqual(
            flight.RightLength.Value,
            left_length + difference - width * math.tan(math.radians(10.0)),
        )
        flight.EndAngle = 0.0
        flight.StartAngle = 0.0
        self.assertAlmostEqual(flight.LeftLength.Value, left_length)
        self.assertAlmostEqual(flight.RightLength.Value, left_length)
        flight.StartAngle = 30.0
        flight.EndAngle = 10.0
        flight.EntryDirection = "From right"
        flight.ExitDirection = "To left"
        self.document.recompute()
        self.assertEqual(stair.GeometryStatus, "Balanced entry/exit stair")
        generated_parts = [
            child
            for child in stair.StepsGroup.Group
            if getattr(child, "StairDesignerRole", "") in ("Tread", "Riser")
        ]
        self.assertTrue(generated_parts)
        self.assertTrue(
            all(part.Shape.isValid() and len(part.Shape.Solids) == 1 for part in generated_parts)
        )

    def test_tangential_circular_flight_geometry(self):
        radius = 1000.0
        inner_radius = 600.0
        arc_length = math.pi * radius / 2.0
        specs = [
            ("Straight", 1200.0, 800.0, 0.0, "Left"),
            ("Circular", inner_radius, 800.0, 90.0, "Left"),
            ("Straight", 1000.0, 800.0, 90.0, "Left"),
        ]
        sections, going = tangent_flight_sections(specs, 12)
        footprint = make_tangent_stair_footprint(specs)
        sections = fit_tangent_sections_to_footprint(sections, footprint)

        self.assertEqual(len(sections), 13)
        self.assertAlmostEqual(sections[0].tangent[0], 1.0, places=3)
        self.assertAlmostEqual(sections[0].tangent[1], 0.0, places=3)
        self.assertAlmostEqual(sections[-1].tangent[0], 0.0, places=3)
        self.assertAlmostEqual(sections[-1].tangent[1], 1.0, places=3)
        self.assertTrue(
            any(
                abs(section.tangent[0]) > 0.1 and abs(section.tangent[1]) > 0.1
                for section in sections
            )
        )
        self.assertAlmostEqual(going, (2200.0 + arc_length) / 12.0, delta=0.1)
        self.assertAlmostEqual(footprint.BoundBox.XMin, 0.0, delta=0.1)
        self.assertAlmostEqual(footprint.BoundBox.XMax, 2600.0, delta=0.1)
        self.assertAlmostEqual(footprint.BoundBox.YMin, 0.0, delta=0.1)
        self.assertAlmostEqual(footprint.BoundBox.YMax, 2400.0, delta=0.1)
        self.assertTrue(
            any(
                abs(edge.curvatureAt((edge.FirstParameter + edge.LastParameter) / 2.0)) > 1e-9
                for edge in footprint.Edges
            )
        )
        faces = balanced_tread_faces(sections, footprint)
        self.assertTrue(balanced_partition_is_valid(faces, footprint, 12))
        self.assertTrue(all(face.isValid() for face in faces))

        concrete = make_balanced_concrete_shape(sections, footprint, 180.0, 150.0)
        self.assertTrue(concrete.isValid())
        self.assertEqual(len(concrete.Solids), 1)
        cylindrical_sides = [
            face for face in concrete.Faces if isinstance(face.Surface, Part.Cylinder)
        ]
        helical_bottoms = [
            face for face in concrete.Faces if isinstance(face.Surface, Part.BSplineSurface)
        ]
        self.assertEqual(len(cylindrical_sides), 2)
        self.assertEqual(len(helical_bottoms), 1)
        self.assertEqual(
            sorted(round(face.Surface.Radius, 3) for face in cylindrical_sides),
            [inner_radius, inner_radius + 800.0],
        )

        unequal_specs = [
            ("Straight", 1900.0, 800.0, 0.0, "Left"),
            ("Circular", 400.0, 1000.0, 90.0, "Right"),
        ]
        unequal_sections, _going = tangent_flight_sections(unequal_specs, 14)
        unequal_footprint = make_tangent_stair_footprint(unequal_specs)
        unequal_sections = fit_tangent_sections_to_footprint(unequal_sections, unequal_footprint)
        unequal_faces = balanced_tread_faces(unequal_sections, unequal_footprint)
        self.assertAlmostEqual(
            sum(face.Area for face in unequal_faces),
            unequal_footprint.Area,
            delta=1.0,
        )
        self.assertTrue(
            balanced_partition_is_valid(unequal_faces, unequal_footprint, 14),
            (
                unequal_footprint.Area,
                sum(face.Area for face in unequal_faces),
                [face.isValid() for face in unequal_faces],
            ),
        )

        circular_pair = [
            ("Circular", inner_radius, 800.0, 90.0, "Left"),
            ("Circular", inner_radius, 800.0, 90.0, "Right"),
        ]
        pair_sections, _going = tangent_flight_sections(circular_pair, 10, turn_types=["Landing"])
        pair_footprint = make_tangent_stair_footprint(circular_pair, ["Landing"])
        pair_sections = fit_tangent_sections_to_footprint(pair_sections, pair_footprint)
        pair_faces = balanced_tread_faces(pair_sections, pair_footprint)
        self.assertFalse(any(section.landing_to_next for section in pair_sections))
        self.assertTrue(balanced_partition_is_valid(pair_faces, pair_footprint, 10))

        angled_specs = [
            ("Straight", 1000.0, 800.0, 0.0, "Left"),
            ("Circular", inner_radius, 800.0, 60.0, "Left"),
            ("Straight", 500.0, 800.0, 90.0, "Left"),
        ]
        angled_sections, angled_going = tangent_flight_sections(angled_specs, 10)
        angled_footprint = make_tangent_stair_footprint(angled_specs)
        angled_sections = fit_tangent_sections_to_footprint(angled_sections, angled_footprint)
        angled_faces = balanced_tread_faces(angled_sections, angled_footprint)
        self.assertAlmostEqual(
            angled_sections[-1].tangent[0], math.cos(math.radians(60.0)), places=3
        )
        self.assertAlmostEqual(
            angled_sections[-1].tangent[1], math.sin(math.radians(60.0)), places=3
        )
        self.assertAlmostEqual(
            angled_going,
            (1500.0 + radius * math.radians(60.0)) / 10.0,
            delta=0.1,
        )
        self.assertTrue(balanced_partition_is_valid(angled_faces, angled_footprint, 10))

    def test_explicit_straight_and_circular_landings_are_level(self):
        def sloped_faces(shape):
            result = []
            for face in shape.Faces:
                parameters = face.ParameterRange
                normal = face.normalAt(
                    (parameters[0] + parameters[1]) / 2.0,
                    (parameters[2] + parameters[3]) / 2.0,
                )
                if 0.01 < abs(normal.z) < 0.999999:
                    result.append(face)
            return result

        straight_landing_specs = [
            (
                "Straight",
                1600.0,
                800.0,
                0.0,
                "Left",
                "Straight",
                "Straight",
            ),
            (
                "Straight landing",
                2400.0,
                800.0,
                90.0,
                "Left",
                "From left",
                "To left",
            ),
            (
                "Straight",
                1600.0,
                800.0,
                0.0,
                "Left",
                "Straight",
                "Straight",
            ),
        ]

        def build(specs, landing_replaces_tread=True):
            sections, _going = tangent_flight_sections(
                specs,
                8,
                1.0,
                ["Landing", "Landing"],
                landing_replaces_tread=landing_replaces_tread,
            )
            footprint = make_tangent_stair_footprint(specs, ["Landing", "Landing"])
            sections = fit_tangent_sections_to_footprint(sections, footprint)
            faces = tangent_tread_faces(sections, specs)
            concrete = make_balanced_concrete_shape(sections, footprint, 180.0, 150.0, faces)
            return sections, faces, concrete

        sections, faces, concrete = build(straight_landing_specs)
        level_indices = [
            index for index, section in enumerate(sections[:-1]) if section.level_to_next
        ]
        self.assertEqual(level_indices, [4])
        self.assertEqual(len(sections), 9)
        self.assertEqual(sections[4].riser_index, 5)
        self.assertEqual(sections[5].riser_index, 6)
        self.assertAlmostEqual(faces[4].Area, 2400.0 * 800.0, delta=1.0)
        self.assertLess(faces[3].common(faces[4]).Area, 0.01)
        self.assertLess(faces[4].common(faces[5]).Area, 0.01)
        self.assertTrue(all(face.isValid() for face in faces))
        concrete_sections, concrete_faces, concrete = build(straight_landing_specs, False)
        concrete_index = next(
            index for index, section in enumerate(concrete_sections[:-1]) if section.level_to_next
        )
        self.assertEqual(len(concrete_sections), 10)
        self.assertEqual(
            concrete_sections[concrete_index - 1].riser_index,
            concrete_sections[concrete_index].riser_index,
        )
        self.assertEqual(
            concrete_sections[concrete_index + 1].riser_index,
            concrete_sections[concrete_index].riser_index + 1,
        )
        self.assertTrue(concrete.isValid())
        self.assertEqual(len(concrete.Solids), 1)
        self.assertEqual(len(sloped_faces(concrete)), 2)
        landing_bottom = concrete_sections[concrete_index].riser_index * 180.0 - 150.0
        landing_bottoms = [
            face
            for face in concrete.Faces
            if face.BoundBox.ZLength < 1e-7 and abs(face.BoundBox.ZMin - landing_bottom) < 0.01
        ]
        self.assertTrue(landing_bottoms)
        self.assertGreater(
            max(face.Area for face in landing_bottoms),
            concrete_faces[concrete_index].Area * 0.99,
        )

        circular_landing_specs = list(straight_landing_specs)
        circular_landing_specs[1] = (
            "Circular landing",
            500.0,
            800.0,
            90.0,
            "Left",
            "Straight",
            "Straight",
        )
        circular_sections, circular_faces, _circular_wood_concrete = build(circular_landing_specs)
        circular_index = next(
            index for index, section in enumerate(circular_sections[:-1]) if section.level_to_next
        )
        self.assertEqual(circular_sections[circular_index].flight_index, 1)
        self.assertTrue(
            any(
                abs(edge.curvatureAt((edge.FirstParameter + edge.LastParameter) / 2.0)) > 1e-9
                for edge in circular_faces[circular_index].Edges
            )
        )
        (
            circular_concrete_sections,
            circular_concrete_faces,
            circular_concrete,
        ) = build(circular_landing_specs, False)
        circular_concrete_index = next(
            index
            for index, section in enumerate(circular_concrete_sections[:-1])
            if section.level_to_next
        )
        self.assertEqual(
            circular_concrete_sections[circular_concrete_index - 1].riser_index,
            circular_concrete_sections[circular_concrete_index].riser_index,
        )
        self.assertEqual(
            circular_concrete_sections[circular_concrete_index + 1].riser_index,
            circular_concrete_sections[circular_concrete_index].riser_index + 1,
        )
        self.assertTrue(circular_concrete.isValid())
        self.assertEqual(len(circular_concrete.Solids), 1)
        self.assertEqual(len(sloped_faces(circular_concrete)), 2)
        circular_bottom = (
            circular_concrete_sections[circular_concrete_index].riser_index * 180.0 - 150.0
        )
        circular_bottoms = [
            face
            for face in circular_concrete.Faces
            if face.BoundBox.ZLength < 1e-7 and abs(face.BoundBox.ZMin - circular_bottom) < 0.01
        ]
        self.assertTrue(circular_bottoms)
        self.assertGreater(
            max(face.Area for face in circular_bottoms),
            circular_concrete_faces[circular_concrete_index].Area * 0.99,
        )

        outgoing = circular_concrete_sections[circular_concrete_index + 1]
        outgoing_probe = (
            outgoing.center[0] + outgoing.tangent[0] * 0.01,
            outgoing.center[1] + outgoing.tangent[1] * 0.01,
        )
        outgoing_cut = circular_concrete.common(
            Part.makeLine(
                FreeCAD.Vector(*outgoing_probe, -1.0),
                FreeCAD.Vector(*outgoing_probe, 4000.0),
            )
        )
        self.assertGreaterEqual(
            min(vertex.Point.z for vertex in outgoing_cut.Vertexes),
            circular_bottom - 0.1,
        )

        end = circular_concrete_sections[-1]
        end_probe = (
            end.center[0] - end.tangent[0] * 0.01,
            end.center[1] - end.tangent[1] * 0.01,
        )
        end_cut = circular_concrete.common(
            Part.makeLine(
                FreeCAD.Vector(*end_probe, -1.0),
                FreeCAD.Vector(*end_probe, 4000.0),
            )
        )
        end_elevations = [vertex.Point.z for vertex in end_cut.Vertexes]
        self.assertAlmostEqual(max(end_elevations) - min(end_elevations), 150.0, delta=0.1)

    def test_balanced_winder_sections_have_equal_goings(self):
        sections, going = balanced_winder_sections(
            [(1600.0, 800.0, 0.0), (1600.0, 800.0, 90.0)],
            9,
            1.0,
        )
        self.assertEqual(len(sections), 10)
        for first, second in zip(sections, sections[1:]):
            self.assertAlmostEqual(second.station - first.station, going)
        winding_sections = [
            section
            for section in sections
            if abs(section.tangent[0]) > 0.1 and abs(section.tangent[1]) > 0.1
        ]
        self.assertGreaterEqual(len(winding_sections), 2)
        self.assertTrue(
            any(
                abs(
                    (rear.left[0] - front.left[0]) ** 2
                    + (rear.left[1] - front.left[1]) ** 2
                    - going**2
                )
                > 1.0
                for front, rear in zip(sections, sections[1:])
            )
        )

        long_sections, _long_going = balanced_winder_sections(
            [(3000.0, 800.0, 0.0), (3000.0, 800.0, 90.0)],
            20,
            1.0,
        )
        long_footprint = make_stair_footprint([(3000.0, 800.0, 0.0), (3000.0, 800.0, 90.0)])
        long_sections = fit_balanced_sections_to_footprint(long_sections, long_footprint)
        self.assertTrue(any(not section.locked_to_flight for section in long_sections))
        self.assertLess(
            sum(not section.locked_to_flight for section in long_sections),
            len(long_sections) / 2,
        )
        for section in long_sections:
            far_from_turn = (section.flight_index == 0 and section.center[0] < 2200.0) or (
                section.flight_index == 1 and section.center[1] > 1200.0
            )
            if not far_from_turn:
                continue
            self.assertTrue(section.locked_to_flight, section)
            direction = ((1.0, 0.0), (0.0, 1.0))[section.flight_index]
            self.assertAlmostEqual(
                section.tangent[0] * direction[1] - section.tangent[1] * direction[0],
                0.0,
            )

        footprint = make_stair_footprint([(1600.0, 800.0, 0.0), (1600.0, 800.0, 90.0)])
        sections = fit_balanced_sections_to_footprint(sections, footprint)
        for section in sections:
            self.assertAlmostEqual(
                (section.left[0] - section.center[0]) * section.tangent[0]
                + (section.left[1] - section.center[1]) * section.tangent[1],
                0.0,
            )
            self.assertAlmostEqual(
                (section.right[0] - section.center[0]) * section.tangent[0]
                + (section.right[1] - section.center[1]) * section.tangent[1],
                0.0,
            )
        tread_faces = balanced_tread_faces(sections, footprint)
        self.assertAlmostEqual(footprint.Area, 2560000.0)
        tread_volume = sum(
            make_balanced_tread_shape(
                front,
                rear,
                footprint,
                1.0,
                1.0,
                0.0,
                base_face=base_face,
            ).Volume
            for front, rear, base_face in zip(sections, sections[1:], tread_faces)
        )
        self.assertAlmostEqual(tread_volume, footprint.Area, delta=1.0)
        self.assertAlmostEqual(sum(face.Area for face in tread_faces), footprint.Area, delta=1.0)
        for front, rear, base_face in zip(sections[:-2], sections[1:-1], tread_faces[:-1]):
            tread = make_balanced_tread_shape(
                front,
                rear,
                footprint,
                40.0,
                40.0,
                30.0,
                28.0,
                base_face=base_face,
            )
            required_rear_support = make_balanced_riser_shape(
                rear,
                0.0,
                1.0,
                28.0,
                footprint,
            )
            actual_riser = make_balanced_riser_shape(
                rear,
                0.0,
                100.0,
                18.0,
                footprint,
            )
            self.assertTrue(tread.isValid())
            self.assertEqual(len(tread.Solids), 1)
            self.assertTrue(actual_riser.isValid())
            self.assertAlmostEqual(
                required_rear_support.cut(tread).Volume,
                0.0,
                delta=0.1,
            )

        half_turn_specs = [
            (1200.0, 800.0, 0.0),
            (1200.0, 800.0, 90.0),
            (1200.0, 800.0, 180.0),
        ]
        half_turn_sections, _going = balanced_winder_sections(
            half_turn_specs,
            9,
            1.0,
        )
        half_turn_footprint = make_stair_footprint(half_turn_specs)
        half_turn_sections = fit_balanced_sections_to_footprint(
            half_turn_sections, half_turn_footprint
        )
        half_turn_faces = balanced_tread_faces(half_turn_sections, half_turn_footprint)
        half_turn_treads = [
            make_balanced_tread_shape(
                front,
                rear,
                half_turn_footprint,
                1.0,
                1.0,
                0.0,
                base_face=base_face,
            )
            for front, rear, base_face in zip(
                half_turn_sections, half_turn_sections[1:], half_turn_faces
            )
        ]
        self.assertAlmostEqual(half_turn_footprint.Area, 2880000.0)
        self.assertAlmostEqual(
            sum(tread.Volume for tread in half_turn_treads),
            half_turn_footprint.Area,
            delta=1.0,
        )
        self.assertTrue(all(tread.isValid() for tread in half_turn_treads))
        self.assertTrue(all(len(tread.Solids) == 1 for tread in half_turn_treads))

        short_turn_specs = [(700.0, 800.0, 0.0), (700.0, 800.0, 90.0)]
        short_turn_sections, _going = balanced_winder_sections(short_turn_specs, 7, 1.0)
        short_turn_footprint = make_stair_footprint(short_turn_specs)
        short_turn_sections = fit_balanced_sections_to_footprint(
            short_turn_sections, short_turn_footprint
        )
        short_turn_faces = balanced_tread_faces(short_turn_sections, short_turn_footprint)
        short_turn_union = short_turn_faces[0]
        for face in short_turn_faces[1:]:
            short_turn_union = short_turn_union.fuse(face)
        self.assertAlmostEqual(
            sum(face.Area for face in short_turn_faces),
            short_turn_footprint.Area,
            delta=1.0,
        )
        self.assertAlmostEqual(short_turn_union.Area, short_turn_footprint.Area, delta=1.0)
        self.assertTrue(all(face.isValid() for face in short_turn_faces))

        angled_specs = [(1600.0, 800.0, 0.0), (1600.0, 800.0, 45.0)]
        angled_sections, _going = balanced_winder_sections(angled_specs, 9, 1.0)
        angled_footprint = make_stair_footprint(angled_specs)
        angled_sections = fit_balanced_sections_to_footprint(angled_sections, angled_footprint)
        angled_faces = balanced_tread_faces(angled_sections, angled_footprint)
        angled_vertices = [
            (vertex.Point.x, vertex.Point.y)
            for vertex in angled_footprint.OuterWire.OrderedVertexes
        ]
        miter_offset = 400.0 * math.tan(math.radians(22.5))
        self.assertEqual(len(angled_vertices), 6)
        self.assertTrue(
            any(
                abs(x - (1600.0 - miter_offset)) < 0.01 and abs(y - 800.0) < 0.01
                for x, y in angled_vertices
            )
        )
        self.assertTrue(
            any(
                abs(x - (1600.0 + miter_offset)) < 0.01 and abs(y) < 0.01
                for x, y in angled_vertices
            )
        )
        self.assertAlmostEqual(angled_footprint.Area, 2560000.0, delta=1.0)
        self.assertTrue(balanced_partition_is_valid(angled_faces, angled_footprint, 9))

        angled_landing_sections, _going = balanced_winder_sections(
            angled_specs, 9, 1.0, ["Landing"]
        )
        angled_landing_sections = fit_balanced_sections_to_footprint(
            angled_landing_sections, angled_footprint
        )
        angled_landing_faces = balanced_tread_faces(angled_landing_sections, angled_footprint)
        self.assertEqual(
            sum(section.landing_to_next for section in angled_landing_sections),
            1,
        )
        self.assertTrue(balanced_partition_is_valid(angled_landing_faces, angled_footprint, 9))

        variable_turn_specs = [
            (2200.0, 600.0, 0.0),
            (700.0, 1100.0, 90.0),
            (1500.0, 500.0, 180.0),
        ]
        variable_sections, _going = balanced_winder_sections(variable_turn_specs, 7, 1.0)
        variable_footprint = make_stair_footprint(variable_turn_specs)
        variable_sections = fit_balanced_sections_to_footprint(
            variable_sections, variable_footprint
        )
        variable_faces = balanced_tread_faces(variable_sections, variable_footprint)
        self.assertAlmostEqual(
            sum(face.Area for face in variable_faces),
            variable_footprint.Area,
            delta=1.0,
        )
        self.assertTrue(all(face.isValid() for face in variable_faces))

        overlapping_specs = [
            (700.0, 800.0, 0.0),
            (700.0, 800.0, 90.0),
            (700.0, 800.0, 180.0),
        ]
        overlapping_sections, _going = balanced_winder_sections(overlapping_specs, 12, 1.0)
        overlapping_footprint = make_stair_footprint(overlapping_specs)
        overlapping_sections = fit_balanced_sections_to_footprint(
            overlapping_sections, overlapping_footprint
        )
        overlapping_faces = balanced_tread_faces(overlapping_sections, overlapping_footprint)
        self.assertFalse(balanced_partition_is_valid(overlapping_faces, overlapping_footprint, 12))

        wider_balance, _going = balanced_winder_sections(
            [(3000.0, 800.0, 0.0), (3000.0, 800.0, 90.0)],
            9,
            1.5,
        )
        reference_balance, _going = balanced_winder_sections(
            [(3000.0, 800.0, 0.0), (3000.0, 800.0, 90.0)],
            9,
            1.0,
        )
        self.assertTrue(
            any(
                abs(first.center[0] - second.center[0]) > 1.0
                or abs(first.center[1] - second.center[1]) > 1.0
                for first, second in zip(reference_balance, wider_balance)
            )
        )
        local_balance, _going = balanced_winder_sections(
            [(3000.0, 800.0, 0.0), (3000.0, 800.0, 90.0)],
            9,
            winding_parameters=[(100, 50)],
        )
        distant_balance, _going = balanced_winder_sections(
            [(3000.0, 800.0, 0.0), (3000.0, 800.0, 90.0)],
            9,
            winding_parameters=[(50, 100)],
        )
        for adjusted in (local_balance, distant_balance):
            self.assertTrue(
                any(
                    math.hypot(
                        first.center[0] - second.center[0],
                        first.center[1] - second.center[1],
                    )
                    > 1.0
                    for first, second in zip(reference_balance, adjusted)
                )
            )

    def test_balanced_nosings_do_not_cross_the_stair_sides(self):
        specs = [(1600.0, 800.0, 0.0), (1600.0, 800.0, 90.0)]
        footprint = make_stair_footprint(specs)
        sections, _going = balanced_winder_sections(specs, 9, 1.0)
        sections = fit_balanced_sections_to_footprint(sections, footprint)
        faces = balanced_tread_faces(sections, footprint)

        envelope_face = footprint.copy()
        envelope_face.translate(FreeCAD.Vector(0.0, 0.0, -1.0))
        envelope = envelope_face.extrude(FreeCAD.Vector(0.0, 0.0, 42.0))
        checked = 0
        for index, (front, rear, face) in enumerate(zip(sections, sections[1:], faces)):
            if index == 0 or front.locked_to_flight:
                continue
            tread = make_balanced_tread_shape(
                front,
                rear,
                footprint,
                40.0,
                40.0,
                30.0,
                base_face=face,
            )
            self.assertLess(tread.cut(envelope).Volume, 1e-3)
            for band_face in _section_band_faces(front, footprint, -30.0):
                expected_band = band_face.extrude(FreeCAD.Vector(0.0, 0.0, 40.0))
                self.assertLess(expected_band.cut(tread).Volume, 1e-3)
            checked += 1
        self.assertGreater(checked, 0)

        corner_index = 5
        corner = (
            footprint.BoundBox.XMax,
            footprint.BoundBox.YMin,
        )
        corner_front = sections[corner_index]
        corner_nosing = -(
            (corner[0] - corner_front.center[0]) * corner_front.tangent[0]
            + (corner[1] - corner_front.center[1]) * corner_front.tangent[1]
        )
        self.assertGreater(corner_nosing, 0.0)
        corner_tread = make_balanced_tread_shape(
            corner_front,
            sections[corner_index + 1],
            footprint,
            40.0,
            40.0,
            corner_nosing,
            base_face=faces[corner_index],
        )
        for band_face in _section_band_faces(corner_front, footprint, -corner_nosing):
            expected_band = band_face.extrude(FreeCAD.Vector(0.0, 0.0, 40.0))
            self.assertLess(expected_band.cut(corner_tread).Volume, 1e-3)

    def test_landing_is_one_rectangular_corner_tread(self):
        specs = [(1600.0, 800.0, 0.0), (1600.0, 800.0, 90.0)]
        sections, _going = balanced_winder_sections(
            specs,
            9,
            1.0,
            ["Landing"],
        )
        self.assertEqual(len(sections), 10)
        landing_indices = [
            index for index, section in enumerate(sections[:-1]) if section.landing_to_next
        ]
        self.assertEqual(len(landing_indices), 1)
        landing_index = landing_indices[0]

        footprint = make_stair_footprint(specs)
        sections = fit_balanced_sections_to_footprint(sections, footprint)
        directions = ((1.0, 0.0), (0.0, 1.0))
        for section in sections:
            direction = directions[section.flight_index]
            self.assertTrue(section.locked_to_flight)
            self.assertAlmostEqual(
                section.tangent[0] * direction[1] - section.tangent[1] * direction[0],
                0.0,
            )
        faces = balanced_tread_faces(sections, footprint)
        landing = faces[landing_indices[0]]
        self.assertAlmostEqual(landing.Area, 800.0 * 800.0, delta=1.0)
        self.assertAlmostEqual(landing.BoundBox.XLength, 800.0, delta=0.1)
        self.assertAlmostEqual(landing.BoundBox.YLength, 800.0, delta=0.1)
        self.assertTrue(balanced_partition_is_valid(faces, footprint, 9))

        concrete = make_balanced_concrete_shape(sections, footprint, 180.0, 150.0)
        self.assertTrue(concrete.isValid())
        self.assertEqual(len(concrete.Solids), 1)
        landing_top = (landing_index + 1) * 180.0
        expected_landing_bottom = landing_top - 150.0
        landing_bottoms = [
            face
            for face in concrete.Faces
            if face.BoundBox.ZLength < 1e-7
            and abs(face.BoundBox.ZMin - expected_landing_bottom) < 0.01
            and face.BoundBox.XMax > landing.BoundBox.XMin + 0.01
            and face.BoundBox.XMin < landing.BoundBox.XMax - 0.01
            and face.BoundBox.YMax > landing.BoundBox.YMin + 0.01
            and face.BoundBox.YMin < landing.BoundBox.YMax - 0.01
        ]
        self.assertEqual(len(landing_bottoms), 1)
        landing_bottom = landing_bottoms[0]
        self.assertGreater(landing_bottom.Area, landing.Area * 0.5)
        self.assertAlmostEqual(
            landing_top - landing_bottom.BoundBox.ZMin,
            150.0,
        )

        concrete_sections, _going = balanced_winder_sections(
            specs,
            9,
            1.0,
            ["Landing"],
            landing_replaces_tread=False,
        )
        concrete_sections = fit_balanced_sections_to_footprint(concrete_sections, footprint)
        concrete_faces = balanced_tread_faces(concrete_sections, footprint)
        concrete_landing_index = next(
            index for index, section in enumerate(concrete_sections[:-1]) if section.landing_to_next
        )
        self.assertEqual(len(concrete_sections), 11)
        self.assertEqual(
            concrete_sections[concrete_landing_index - 1].riser_index,
            concrete_sections[concrete_landing_index].riser_index,
        )
        self.assertEqual(
            concrete_sections[concrete_landing_index + 1].riser_index,
            concrete_sections[concrete_landing_index].riser_index + 1,
        )
        self.assertTrue(
            balanced_partition_is_valid(concrete_faces, footprint, len(concrete_sections) - 1)
        )
        concrete_landing = make_balanced_concrete_shape(
            concrete_sections,
            footprint,
            180.0,
            150.0,
            concrete_faces,
        )
        self.assertTrue(concrete_landing.isValid())
        self.assertEqual(len(concrete_landing.Solids), 1)
        sloped_faces = []
        for face in concrete_landing.Faces:
            parameters = face.ParameterRange
            normal = face.normalAt(
                (parameters[0] + parameters[1]) / 2.0,
                (parameters[2] + parameters[3]) / 2.0,
            )
            if 0.01 < abs(normal.z) < 0.999999:
                sloped_faces.append(face)
        self.assertEqual(len(sloped_faces), 2)

        expected_end_bottom = (len(sections) - 1) * 180.0 - 150.0
        for point in (sections[-1].left, sections[-1].right):
            self.assertTrue(
                any(
                    math.hypot(
                        vertex.Point.x - point[0],
                        vertex.Point.y - point[1],
                    )
                    < 0.01
                    and abs(vertex.Point.z - expected_end_bottom) < 0.01
                    for vertex in concrete.Vertexes
                )
            )

        expanded_treads = []
        for index, (front, rear, face) in enumerate(zip(sections, sections[1:], faces)):
            expanded_treads.append(
                make_balanced_tread_shape(
                    front,
                    rear,
                    footprint,
                    40.0,
                    40.0,
                    30.0,
                    28.0,
                    base_face=face,
                    local_expansion=(
                        front.landing_to_next
                        or rear.landing_to_next
                        or (index > 0 and sections[index - 1].landing_to_next)
                    ),
                )
            )
        self.assertLessEqual(expanded_treads[landing_index - 1].BoundBox.YMax, 800.1)
        self.assertGreaterEqual(expanded_treads[landing_index + 1].BoundBox.XMin, 1199.9)
        for index in (landing_index - 1, landing_index + 1):
            front = sections[index]
            rear = sections[index + 1]
            direction = front.tangent
            self.assertAlmostEqual(
                direction[0] * rear.tangent[1] - direction[1] * rear.tangent[0],
                0.0,
            )

            def projection_bounds(shape):
                projections = [
                    vertex.Point.x * direction[0] + vertex.Point.y * direction[1]
                    for vertex in shape.Vertexes
                ]
                return min(projections), max(projections)

            tread_min, tread_max = projection_bounds(expanded_treads[index])
            front_riser_min, _front_riser_max = projection_bounds(
                make_balanced_riser_shape(front, 0.0, 100.0, 18.0)
            )
            _rear_riser_min, rear_riser_max = projection_bounds(
                make_balanced_riser_shape(rear, 0.0, 100.0, 18.0)
            )
            self.assertAlmostEqual(front_riser_min - tread_min, 30.0)
            self.assertAlmostEqual(tread_max - rear_riser_max, 10.0)

        mixed_specs = [
            (1600.0, 800.0, 0.0),
            (1600.0, 800.0, 90.0),
            (1600.0, 800.0, 180.0),
        ]
        mixed_sections, _going = balanced_winder_sections(
            mixed_specs,
            12,
            1.0,
            ["Landing", "Herse balancing"],
        )
        self.assertEqual(sum(section.landing_to_next for section in mixed_sections), 1)
        mixed_footprint = make_stair_footprint(mixed_specs)
        mixed_sections = fit_balanced_sections_to_footprint(mixed_sections, mixed_footprint)
        mixed_directions = ((1.0, 0.0), (0.0, 1.0), (-1.0, 0.0))
        self.assertTrue(any(not section.locked_to_flight for section in mixed_sections))
        for section in mixed_sections:
            if not section.locked_to_flight:
                continue
            direction = mixed_directions[section.flight_index]
            self.assertAlmostEqual(
                section.tangent[0] * direction[1] - section.tangent[1] * direction[0],
                0.0,
            )
        mixed_faces = balanced_tread_faces(mixed_sections, mixed_footprint)
        self.assertTrue(balanced_partition_is_valid(mixed_faces, mixed_footprint, 12))

    def test_turn_and_half_turn_flights(self):
        stair = make_stair(
            floor_height=1800.0,
            flight_length=1600.0,
            width=800.0,
            steps=10,
        )
        stair.Proxy._updating = True
        try:
            flights = resize_flights(stair, 2, length=1600.0, width=800.0, rotations=["Left"])
            flights[0].Angle = 0.0
            flights[1].Angle = 90.0
        finally:
            stair.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()

        flights = get_flights(stair)
        self.assertEqual(len(flights), 2)
        self.assertEqual(str(flights[1].Rotation), "Left")
        self.assertAlmostEqual(flights[1].Angle.Value, 90.0)
        self.assertEqual(sum(flight.NumberOfTreads for flight in flights), 9)
        second_flight_treads = [
            child
            for child in stair.StepsGroup.Group
            if getattr(child, "StairDesignerRole", "") == "Tread" and child.FlightIndex == 2
        ]
        self.assertTrue(second_flight_treads)
        self.assertTrue(all(tread.Shape.isValid() for tread in second_flight_treads))
        self.assertTrue(all(len(tread.Shape.Solids) == 1 for tread in second_flight_treads))
        herse_treads = [
            child
            for child in stair.StepsGroup.Group
            if getattr(child, "StairDesignerRole", "") == "Tread"
        ]
        self.assertTrue(all(len(tread.Shape.Solids) == 1 for tread in herse_treads))
        self.assertGreater(
            max(tread.Shape.BoundBox.YMax for tread in second_flight_treads),
            800.0,
        )

        flights[1].TurnType = "Landing"
        self.document.recompute()
        self.assertEqual(stair.GeometryStatus, "Multi-flight stair with landing")
        self.assertTrue(
            all(
                child.Shape.isValid()
                for child in stair.StepsGroup.Group
                if getattr(child, "StairDesignerRole", "") in ("Tread", "Riser")
            )
        )
        landing_treads = [
            child
            for child in stair.StepsGroup.Group
            if getattr(child, "StairDesignerRole", "") == "Tread"
            and 800.0 < child.Shape.BoundBox.XLength <= 830.1
            and 800.0 < child.Shape.BoundBox.YLength <= 828.1
        ]
        self.assertEqual(len(landing_treads), 1)
        generated_treads = [
            child
            for child in stair.StepsGroup.Group
            if getattr(child, "StairDesignerRole", "") == "Tread"
        ]
        self.assertTrue(
            all(
                tread.Shape.BoundBox.XLength <= 830.1 and tread.Shape.BoundBox.YLength <= 828.1
                for tread in generated_treads
            )
        )

        flights[1].TurnType = "Herse balancing"
        flights[1].Rotation = "Right"
        self.document.recompute()
        self.assertLess(
            min(tread.Shape.BoundBox.YMin for tread in second_flight_treads),
            0.0,
        )

        stair.Proxy._updating = True
        try:
            flights = resize_flights(
                stair,
                3,
                length=1200.0,
                width=800.0,
                rotations=["Left", "Left"],
            )
            for index, flight in enumerate(flights):
                flight.Proxy._updating = True
                try:
                    flight.LeftLength = 1200.0
                    flight.RightLength = 1200.0
                    flight.Width = 800.0
                    flight.Angle = 0.0 if index == 0 else 90.0
                    if index:
                        flight.Rotation = "Left"
                finally:
                    flight.Proxy._updating = False
            sync_all_flight_side_lengths(stair)
        finally:
            stair.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()
        flights = get_flights(stair)
        self.assertEqual(len(flights), 3)
        self.assertEqual([str(flight.Rotation) for flight in flights[1:]], ["Left", "Left"])
        self.assertEqual(sum(flight.NumberOfTreads for flight in flights), 9)
        self.assertGreater(stair.PlanSketch.GeometryCount, stair.NumberOfTreads)

    def test_balanced_wood_risers_can_be_toggled(self):
        stair = make_stair(
            floor_height=1800.0,
            flight_length=1600.0,
            width=800.0,
            steps=10,
            stair_type="Wood",
        )
        stair.Proxy._updating = True
        try:
            flights = resize_flights(stair, 2, length=1600.0, width=800.0, rotations=["Left"])
            flights[0].Angle = 0.0
            flights[1].Angle = 90.0
        finally:
            stair.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)

        stair.RisersEnabled = False
        self.document.recompute()
        self.assertFalse(
            any(
                getattr(child, "StairDesignerRole", "") == "Riser"
                for child in stair.StepsGroup.Group
            )
        )

        stair.RisersEnabled = True
        self.document.recompute()
        risers = [
            child
            for child in stair.StepsGroup.Group
            if getattr(child, "StairDesignerRole", "") == "Riser"
        ]
        self.assertEqual(len(risers), stair.NumberOfTreads)
        self.assertTrue(all(riser.Shape.isValid() for riser in risers))

    def test_wood_stair_supports_per_side_stringer_types(self):
        stair = make_stair(
            floor_height=1800.0,
            flight_length=1600.0,
            width=800.0,
            steps=10,
            stair_type="Wood",
        )
        self.document.recompute()

        self.assertIsNone(stair.StringersGroup)
        self.assertNotIn("StepMaterial", stair.PropertiesList)
        self.assertNotIn("RiserMaterial", stair.PropertiesList)
        self.assertTrue(
            all("Material" not in child.PropertiesList for child in stair.StepsGroup.Group)
        )

        first_flight = get_flights(stair)[0]
        first_flight.Proxy._updating = True
        try:
            first_flight.RightStringerType = [
                "None",
                "Housed stringer",
                "Cut stringer",
            ]
            first_flight.RightStringerType = "Cut stringer"
        finally:
            first_flight.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.assertEqual(str(first_flight.RightStringerType), "Notched stringer")

        stair.Proxy._updating = True
        first_flight.Proxy._updating = True
        try:
            first_flight.LeftStringerType = "Housed stringer"
            first_flight.RightStringerType = "Notched stringer"
        finally:
            first_flight.Proxy._updating = False
            stair.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()

        self.assertIsNotNone(stair.StringersGroup)
        self.assertNotIn("LeftStringerType", stair.PropertiesList)
        self.assertNotIn("RightStringerType", stair.PropertiesList)
        stringers = stair.StringersGroup.Group
        self.assertEqual(
            {part.StairDesignerRole for part in stringers},
            {"LeftStringer", "RightStringer"},
        )
        self.assertTrue(
            all(
                part.Shape.isValid()
                and len(part.Shape.Solids) == 1
                and part.Shape.Volume > 0.0
                and "Material" not in part.PropertiesList
                for part in stringers
            )
        )
        self.assertFalse(stair.StringerCustomWidth)
        self.assertGreaterEqual(stair.StringerWidth.Value, 235.0)

        left_stringer = next(part for part in stringers if part.StairDesignerRole == "LeftStringer")
        right_stringer = next(
            part for part in stringers if part.StairDesignerRole == "RightStringer"
        )
        broad_faces = [
            face
            for face in right_stringer.Shape.Faces
            if face.Vertexes
            and max(vertex.Point.y for vertex in face.Vertexes)
            - min(vertex.Point.y for vertex in face.Vertexes)
            < 1e-7
        ]
        self.assertEqual(len(broad_faces), 2)

        stair.RisersEnabled = True
        self.document.recompute()
        risers = [child for child in stair.StepsGroup.Group if child.StairDesignerRole == "Riser"]
        self.assertTrue(risers)
        self.assertTrue(
            all(right_stringer.Shape.common(riser.Shape).Volume < 1e-7 for riser in risers)
        )
        stair.RisersEnabled = False
        self.document.recompute()
        stair.Proxy._updating = True
        try:
            stair.StringerNosingOffset = 0.0
            stair.StringerStartExtension = 100.0
        finally:
            stair.Proxy._updating = False
        for direction in ("Perpendicular", "Vertical"):
            stair.Proxy._updating = True
            try:
                stair.StringerNosingOffsetDirection = direction
            finally:
                stair.Proxy._updating = False
            stair.Proxy.rebuild(stair, allow_structure_changes=True)
            section_plane = Part.makePlane(
                20000.0,
                20000.0,
                FreeCAD.Vector(-stair.Nosing.Value, 10000.0, -10000.0),
                FreeCAD.Vector(1.0, 0.0, 0.0),
            )
            section = left_stringer.Shape.section(section_plane)
            self.assertFalse(section.isNull())
            self.assertAlmostEqual(
                section.BoundBox.ZMax,
                stair.RiserHeight.Value,
                places=5,
            )

        stair.StringerCustomWidth = True
        stair.StringerWidth = 260.0
        stair.NumberOfSteps = 11
        self.document.recompute()
        self.assertAlmostEqual(stair.StringerWidth.Value, 260.0)
        self.assertTrue(
            all(abs(part.Width.Value - 260.0) < 1e-7 for part in stair.StringersGroup.Group)
        )

        left_stringer.OverrideThickness = True
        left_stringer.Thickness = 55.0
        stair.StringerThickness = 45.0
        self.document.recompute()
        self.assertAlmostEqual(left_stringer.Thickness.Value, 55.0)
        self.assertTrue(
            all(
                abs(part.Thickness.Value - 45.0) < 1e-7
                for part in stair.StringersGroup.Group
                if part != left_stringer
            )
        )

        stair.Proxy._updating = True
        try:
            flights = resize_flights(stair, 2, length=1600.0, width=800.0, rotations=["Left"])
            flights[0].Angle = 0.0
            flights[1].Angle = 90.0
            flights[1].Proxy._updating = True
            flights[1].LeftStringerType = "Housed stringer"
            flights[1].RightStringerType = "None"
            flights[1].Proxy._updating = False
        finally:
            stair.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()
        self.assertEqual(len(stair.StringersGroup.Group), 3)
        self.assertEqual(
            sum(part.StairDesignerRole == "LeftStringer" for part in stair.StringersGroup.Group),
            2,
        )
        flights[1].RightStringerType = "Notched stringer"
        self.document.recompute()
        self.assertEqual(len(stair.StringersGroup.Group), 4)
        self.assertEqual(left_stringer.SourceFlight.Name, flights[0].Name)
        self.assertAlmostEqual(left_stringer.Thickness.Value, 55.0)
        for role in ("LeftStringer", "RightStringer"):
            parts = sorted(
                (part for part in stair.StringersGroup.Group if part.StairDesignerRole == role),
                key=lambda part: part.FlightIndex,
            )
            self.assertEqual(len(parts), 2)
            self.assertLess(
                parts[0].Shape.distToShape(parts[1].Shape)[0],
                1e-7,
            )
            self.assertLess(
                parts[0].Shape.common(parts[1].Shape).Volume,
                1e-7,
            )
            for part in parts:
                broad_face_offsets = {
                    round(
                        vertex.Point.y if part.FlightIndex == 1 else vertex.Point.x,
                        6,
                    )
                    for vertex in part.Shape.Vertexes
                }
                self.assertEqual(len(broad_face_offsets), 2)
        self.assertTrue(
            all(
                part.Shape.isValid() and len(part.Shape.Solids) == 1 and part.Shape.Volume > 0.0
                for part in stair.StringersGroup.Group
            )
        )

        stair.Proxy._updating = True
        try:
            flights[1].Proxy._updating = True
            flights[1].FlightType = "Circular"
            flights[1].InnerRadius = 450.0
            flights[1].OuterRadius = 1250.0
            flights[1].Angle = 90.0
            flights[1].Proxy._updating = False
        finally:
            stair.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()
        self.assertEqual(len(stair.StringersGroup.Group), 4)
        tangent_x = (flights[0].LeftLength.Value + flights[0].RightLength.Value) / 2.0
        for role in ("LeftStringer", "RightStringer"):
            parts = sorted(
                (part for part in stair.StringersGroup.Group if part.StairDesignerRole == role),
                key=lambda part: part.FlightIndex,
            )
            self.assertEqual(len(parts), 2)
            self.assertLessEqual(parts[0].Shape.BoundBox.XMax, tangent_x + 1e-5)
            self.assertLess(parts[0].Shape.distToShape(parts[1].Shape)[0], 1e-5)
            cylindrical_faces = [
                face for face in parts[1].Shape.Faces if isinstance(face.Surface, Part.Cylinder)
            ]
            cylindrical_radii = {round(face.Surface.Radius, 5) for face in cylindrical_faces}
            self.assertEqual(len(cylindrical_radii), 2, role)
            self.assertAlmostEqual(
                max(cylindrical_radii) - min(cylindrical_radii),
                parts[1].Thickness.Value,
                places=4,
            )
        self.assertTrue(
            all(
                part.Shape.isValid() and len(part.Shape.Solids) == 1 and part.Shape.Volume > 0.0
                for part in stair.StringersGroup.Group
            )
        )

        stair.Proxy._updating = True
        try:
            flights = resize_flights(
                stair, 3, length=1600.0, width=800.0, rotations=["Left", "Left"]
            )
            flight_types = ("Straight", "Straight landing", "Straight")
            lengths = (1600.0, 2400.0, 1600.0)
            angles = (0.0, 90.0, 0.0)
            for flight, flight_type, length, angle in zip(flights, flight_types, lengths, angles):
                flight.Proxy._updating = True
                try:
                    flight.FlightType = flight_type
                    flight.LeftLength = length
                    flight.RightLength = length
                    flight.Width = 800.0
                    flight.Angle = angle
                    if flight_type == "Straight landing":
                        flight.EntryDirection = "From left"
                        flight.ExitDirection = "To left"
                        flight.LeftStringerType = "Housed stringer"
                        flight.RightStringerType = "Notched stringer"
                    else:
                        flight.LeftStringerType = "Housed stringer"
                        flight.RightStringerType = "Notched stringer"
                finally:
                    flight.Proxy._updating = False
            sync_all_flight_side_lengths(stair)
        finally:
            stair.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()
        self.assertEqual(len(stair.StringersGroup.Group), 6)
        self.assertTrue(
            all(
                part.Shape.isValid() and len(part.Shape.Solids) == 1 and part.Shape.Volume > 0.0
                for part in stair.StringersGroup.Group
            )
        )

        stair.StairType = "Concrete"
        self.document.recompute()
        self.assertIsNone(stair.StringersGroup)

    def test_circular_housed_stringer_keeps_nosing_clearance(self):
        specs = [
            ("Straight", 1600.0, 800.0, 0.0, "Left", "Straight", "Straight"),
            ("Circular", 500.0, 800.0, 90.0, "Left", "Straight", "Straight"),
        ]
        sections, _going = tangent_flight_sections(specs, 9)
        footprint = make_tangent_stair_footprint(specs)
        sections = fit_tangent_sections_to_footprint(sections, footprint)
        circular_run = stringer_flight_runs(sections, [spec[0] for spec in specs])[1][1]
        elevations = _stringer_elevations(circular_run, 180.0)

        for side in ("Left", "Right"):
            shape = make_housed_stringer_shape(
                circular_run,
                180.0,
                side,
                40.0,
                330.0,
                20.0,
                0.0,
                0.0,
                50.0,
                "Perpendicular",
                20.0,
            )
            circular = _circular_stringer_data(circular_run, side, 40.0, 20.0, True)
            profile = circular["profile"]
            radius = (profile.inner_radius + profile.outer_radius) / 2.0
            clearances = []
            top_elevations = []
            for angle in circular["angles"]:
                x = profile.center[0] + radius * math.cos(angle)
                y = profile.center[1] + radius * math.sin(angle)
                probe = Part.makeLine(
                    FreeCAD.Vector(x, y, -1000.0),
                    FreeCAD.Vector(x, y, 3000.0),
                )
                intersection = shape.common(probe)
                self.assertTrue(intersection.Vertexes, side)
                top_elevations.append(max(vertex.Point.z for vertex in intersection.Vertexes))

            clearances = [
                top - elevation for top, elevation in zip(top_elevations[:-1], elevations[:-1])
            ]

            self.assertTrue(shape.isValid(), side)
            self.assertEqual(len(shape.Solids), 1, side)
            for clearance in clearances[1:]:
                self.assertAlmostEqual(clearance, clearances[0], places=5)
            self.assertAlmostEqual(top_elevations[-1] - top_elevations[-2], 180.0, places=5)

    def test_linear_stringer_after_circular_flight_has_straight_pitch(self):
        specs = [
            ("Straight", 1600.0, 800.0, 0.0, "Left", "Straight", "Straight"),
            ("Circular", 500.0, 800.0, 90.0, "Left", "Straight", "Straight"),
            ("Straight", 1600.0, 800.0, 0.0, "Left", "Straight", "Straight"),
        ]
        sections, _going = tangent_flight_sections(specs, 10)
        footprint = make_tangent_stair_footprint(specs)
        sections = fit_tangent_sections_to_footprint(sections, footprint)
        final_run = stringer_flight_runs(sections, [spec[0] for spec in specs])[2][1]
        shape = make_housed_stringer_shape(
            final_run,
            180.0,
            "Left",
            40.0,
            330.0,
            20.0,
            0.0,
            100.0,
            50.0,
            "Perpendicular",
            20.0,
        )

        def top_at(point):
            probe = Part.makeLine(
                FreeCAD.Vector(point[0], point[1], -1000.0),
                FreeCAD.Vector(point[0], point[1], 3000.0),
            )
            intersection = shape.common(probe)
            self.assertTrue(intersection.Vertexes)
            return max(vertex.Point.z for vertex in intersection.Vertexes)

        rail_points = [section.left for section in final_run]
        tops = [top_at(point) for point in rail_points]
        for first, second in zip(tops, tops[1:]):
            self.assertAlmostEqual(second - first, 180.0, places=5)
        for first_point, second_point, first_top, second_top in zip(
            rail_points, rail_points[1:], tops, tops[1:]
        ):
            midpoint = (
                (first_point[0] + second_point[0]) / 2.0,
                (first_point[1] + second_point[1]) / 2.0,
            )
            self.assertAlmostEqual(top_at(midpoint), (first_top + second_top) / 2.0, places=5)

    def test_landing_after_circular_flight_rebuilds_and_has_stringers(self):
        for landing_type in ("Straight landing", "Circular landing"):
            stair = make_stair(
                floor_height=1800.0,
                flight_length=1600.0,
                width=800.0,
                steps=10,
                stair_type="Wood",
            )
            stair.Proxy._updating = True
            try:
                flights = resize_flights(
                    stair,
                    2,
                    length=1600.0,
                    width=800.0,
                    rotations=["Left"],
                )
                flights[1].Proxy._updating = True
                flights[1].FlightType = "Circular"
                flights[1].Proxy._updating = False
            finally:
                stair.Proxy._updating = False
            stair.Proxy.rebuild(stair, allow_structure_changes=True)

            stair.Proxy._updating = True
            try:
                flights = resize_flights(
                    stair,
                    3,
                    length=1600.0,
                    width=800.0,
                    rotations=["Left", "Left"],
                )
            finally:
                stair.Proxy._updating = False
            stair.Proxy.rebuild(stair, allow_structure_changes=True)
            self.document.recompute()
            self.assertEqual(str(flights[2].FlightType), "Straight")

            flights[2].FlightType = landing_type
            self.document.recompute()
            self.assertEqual(stair.GeometryStatus, "Multi-flight stair with landing")
            self.assertTrue(
                all(
                    part.Shape.isValid() and len(part.Shape.Solids) == 1 and part.Shape.Volume > 0.0
                    for part in stair.StepsGroup.Group
                )
            )

            stair.Proxy._updating = True
            try:
                for flight in flights:
                    flight.Proxy._updating = True
                    try:
                        flight.LeftStringerType = "Housed stringer"
                        flight.RightStringerType = "Notched stringer"
                    finally:
                        flight.Proxy._updating = False
            finally:
                stair.Proxy._updating = False
            stair.Proxy.rebuild(stair, allow_structure_changes=True)
            self.document.recompute()

            stringers = stair.StringersGroup.Group
            self.assertEqual(len(stringers), 6)
            self.assertTrue(
                all(
                    part.Shape.isValid() and len(part.Shape.Solids) == 1 and part.Shape.Volume > 0.0
                    for part in stringers
                )
            )
            self.assertEqual(
                sum(part.SourceFlight == flights[2] for part in stringers),
                2,
            )
            for role in ("LeftStringer", "RightStringer"):
                parts = sorted(
                    (part for part in stringers if part.StairDesignerRole == role),
                    key=lambda part: part.FlightIndex,
                )
                self.assertLess(
                    parts[1].Shape.distToShape(parts[2].Shape)[0],
                    1e-5,
                    f"{landing_type}: {role}",
                )
            if landing_type == "Circular landing":
                for part in (part for part in stringers if part.FlightIndex == 3):
                    radii = {
                        round(face.Surface.Radius, 5)
                        for face in part.Shape.Faces
                        if isinstance(face.Surface, Part.Cylinder)
                    }
                    self.assertEqual(len(radii), 2)

            stair.StairType = "Concrete"
            self.document.recompute()
            shape = stair.ConcreteGeometry.Shape
            self.assertFalse(shape.isNull())
            self.assertTrue(shape.isValid())
            self.assertEqual(len(shape.Solids), 1)
            self.assertIsNone(stair.StringersGroup)

    def test_handrails_support_linear_circular_wood_and_concrete_stairs(self):
        stair = make_stair(
            floor_height=1800.0,
            flight_length=2400.0,
            width=900.0,
            steps=10,
            stair_type="Wood",
        )
        first_flight = get_flights(stair)[0]
        first_flight.Proxy._updating = True
        try:
            first_flight.LeftStringerType = "Housed stringer"
            first_flight.LeftHandrailEnabled = True
        finally:
            first_flight.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()

        self.assertIsNotNone(stair.HandrailsGroup)
        parts = stair.HandrailsGroup.Group
        top_rails = [part for part in parts if part.StairDesignerRole == "HandrailTopRail"]
        posts = [part for part in parts if part.StairDesignerRole == "HandrailPost"]
        pickets = [part for part in parts if part.StairDesignerRole == "HandrailPicket"]
        self.assertEqual(len(top_rails), 1)
        self.assertEqual(len(posts), 2)
        self.assertGreater(len(pickets), 1)
        self.assertTrue(
            all(
                part.Shape.isValid() and len(part.Shape.Solids) == 1 and part.Shape.Volume > 0.0
                for part in parts
            )
        )
        self.assertTrue(
            all(isinstance(face.Surface, Part.Plane) for face in top_rails[0].Shape.Faces)
        )
        first_post = next(post for post in posts if post.ElementIndex == 0)
        self.assertAlmostEqual(first_post.Shape.BoundBox.ZMin, 0.0)
        post_centers = sorted(
            (post.Shape.BoundBox.XMin + post.Shape.BoundBox.XMax) / 2.0 for post in posts
        )
        picket_centers = sorted(
            (picket.Shape.BoundBox.XMin + picket.Shape.BoundBox.XMax) / 2.0 for picket in pickets
        )
        clear_gaps = [
            picket_centers[0]
            - post_centers[0]
            - (stair.HandrailPostThickness.Value + stair.HandrailPicketThickness.Value) / 2.0,
            *[
                second - first - stair.HandrailPicketThickness.Value
                for first, second in zip(picket_centers, picket_centers[1:])
            ],
            post_centers[-1]
            - picket_centers[-1]
            - (stair.HandrailPostThickness.Value + stair.HandrailPicketThickness.Value) / 2.0,
        ]
        self.assertLessEqual(
            max(clear_gaps),
            stair.HandrailPicketMaximumSpacing.Value + 1e-7,
        )

        initial_picket_count = len(pickets)
        stair.HandrailPicketMaximumSpacing = 50.0
        self.document.recompute()
        tighter_pickets = [
            part
            for part in stair.HandrailsGroup.Group
            if part.StairDesignerRole == "HandrailPicket"
        ]
        self.assertGreater(len(tighter_pickets), initial_picket_count)

        stair.Proxy._updating = True
        try:
            flights = resize_flights(
                stair,
                2,
                length=1800.0,
                width=900.0,
                rotations=["Left"],
            )
            flights[1].Proxy._updating = True
            flights[1].FlightType = "Circular"
            flights[1].InnerRadius = 500.0
            flights[1].OuterRadius = 1400.0
            flights[1].Angle = 90.0
            flights[1].LeftHandrailEnabled = True
            flights[1].RightHandrailEnabled = True
            flights[1].Proxy._updating = False
        finally:
            stair.Proxy._updating = False
        stair.HandrailPicketMaximumSpacing = 100.0
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()

        circular_rails = [
            part
            for part in stair.HandrailsGroup.Group
            if part.StairDesignerRole == "HandrailTopRail" and part.FlightIndex == 2
        ]
        self.assertEqual(len(circular_rails), 2)
        for rail in circular_rails:
            cylindrical_radii = {
                round(face.Surface.Radius, 5)
                for face in rail.Shape.Faces
                if isinstance(face.Surface, Part.Cylinder)
            }
            self.assertEqual(len(cylindrical_radii), 2)
        corner_posts = [
            part for part in stair.HandrailsGroup.Group if part.StairDesignerRole == "HandrailPost"
        ]
        post_positions = {
            (
                round(
                    (post.Shape.BoundBox.XMin + post.Shape.BoundBox.XMax) / 2.0,
                    5,
                ),
                round(
                    (post.Shape.BoundBox.YMin + post.Shape.BoundBox.YMax) / 2.0,
                    5,
                ),
            )
            for post in corner_posts
        }
        self.assertEqual(len(post_positions), len(corner_posts))

        stair.Proxy._updating = True
        try:
            stair.HandrailTopRailShape = "Circular"
            stair.HandrailPicketShape = "Circular"
            stair.HandrailPostShape = "Circular"
        finally:
            stair.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()
        self.assertTrue(
            all(
                part.Shape.isValid() and len(part.Shape.Solids) == 1
                for part in stair.HandrailsGroup.Group
            )
        )
        for role in ("HandrailPost", "HandrailPicket"):
            self.assertTrue(
                all(
                    any(isinstance(face.Surface, Part.Cylinder) for face in part.Shape.Faces)
                    for part in stair.HandrailsGroup.Group
                    if part.StairDesignerRole == role
                )
            )

        stair.StairType = "Concrete"
        self.document.recompute()
        self.assertIsNone(stair.StringersGroup)
        self.assertIsNotNone(stair.HandrailsGroup)
        self.assertTrue(
            all(
                part.Shape.isValid()
                and len(part.Shape.Solids) == 1
                and part.Shape.BoundBox.ZMin >= -1e-7
                for part in stair.HandrailsGroup.Group
            )
        )

        for flight in flights:
            flight.Proxy._updating = True
            try:
                flight.LeftHandrailEnabled = False
                flight.RightHandrailEnabled = False
            finally:
                flight.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()
        self.assertIsNone(stair.HandrailsGroup)

    def test_concrete_balanced_pickets_meet_their_local_treads(self):
        stair = make_stair(
            floor_height=2800.0,
            flight_length=2600.0,
            width=900.0,
            steps=16,
            stair_type="Concrete",
        )
        stair.Proxy._updating = True
        try:
            flights = resize_flights(
                stair,
                2,
                length=2600.0,
                width=900.0,
                rotations=["Left"],
            )
            for flight in flights:
                flight.Proxy._updating = True
                try:
                    flight.LeftHandrailEnabled = True
                    flight.RightHandrailEnabled = True
                finally:
                    flight.Proxy._updating = False
        finally:
            stair.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()

        concrete = stair.ConcreteGeometry.Shape
        pickets = [
            part
            for part in stair.HandrailsGroup.Group
            if part.StairDesignerRole == "HandrailPicket"
        ]
        self.assertGreater(len(pickets), 4)
        for picket in pickets:
            self.assertLess(
                picket.Shape.distToShape(concrete)[0],
                1e-5,
                picket.Label,
            )
            self.assertLess(
                picket.Shape.common(concrete).Volume,
                1e-5,
                picket.Label,
            )

    def test_single_flight_handrail_and_stringer_end_conditions(self):
        stair = make_stair(
            floor_height=1800.0,
            flight_length=2400.0,
            width=900.0,
            steps=10,
            stair_type="Wood",
        )
        self.assertAlmostEqual(stair.StringerStartExtension.Value, 0.0)
        self.assertAlmostEqual(stair.StringerEndExtension.Value, 0.0)
        self.assertAlmostEqual(stair.HandrailPostAboveTopRail.Value, 70.0)

        flight = get_flights(stair)[0]
        flight.Proxy._updating = True
        try:
            flight.LeftStringerType = "Housed stringer"
            flight.LeftHandrailEnabled = True
        finally:
            flight.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()

        stringer = stair.StringersGroup.Group[0]
        self.assertGreaterEqual(stringer.Shape.BoundBox.ZMin, -1e-7)
        posts = sorted(
            (
                part
                for part in stair.HandrailsGroup.Group
                if part.StairDesignerRole == "HandrailPost"
            ),
            key=lambda part: part.ElementIndex,
        )
        top_rail = next(
            part
            for part in stair.HandrailsGroup.Group
            if part.StairDesignerRole == "HandrailTopRail"
        )
        post_centers = [
            (post.Shape.BoundBox.XMin + post.Shape.BoundBox.XMax) / 2.0 for post in posts
        ]
        self.assertAlmostEqual(post_centers[0], 0.0)
        self.assertAlmostEqual(post_centers[1], 2400.0)
        # A 35 mm penetration into a 70 mm post ends at its center.
        self.assertAlmostEqual(top_rail.Shape.BoundBox.XMin, 0.0)
        self.assertAlmostEqual(top_rail.Shape.BoundBox.XMax, 2400.0)

        stair.Proxy._updating = True
        try:
            stair.StringerStartExtension = 100.0
            stair.StringerEndExtension = 100.0
        finally:
            stair.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()

        self.assertGreaterEqual(stringer.Shape.BoundBox.ZMin, -1e-7)
        posts = sorted(
            (
                part
                for part in stair.HandrailsGroup.Group
                if part.StairDesignerRole == "HandrailPost"
            ),
            key=lambda part: part.ElementIndex,
        )
        post_centers = [
            (post.Shape.BoundBox.XMin + post.Shape.BoundBox.XMax) / 2.0 for post in posts
        ]
        self.assertAlmostEqual(stringer.Shape.BoundBox.XMin, -100.0)
        self.assertAlmostEqual(stringer.Shape.BoundBox.XMax, 2500.0)
        self.assertAlmostEqual(post_centers[0], -100.0)
        self.assertAlmostEqual(post_centers[1], 2500.0)
        self.assertAlmostEqual(top_rail.Shape.BoundBox.XMin, -100.0)
        self.assertAlmostEqual(top_rail.Shape.BoundBox.XMax, 2500.0)

        stair.StairType = "Concrete"
        self.document.recompute()
        posts = sorted(
            (
                part
                for part in stair.HandrailsGroup.Group
                if part.StairDesignerRole == "HandrailPost"
            ),
            key=lambda part: part.ElementIndex,
        )
        top_post = posts[-1]
        self.assertAlmostEqual(top_post.Shape.BoundBox.ZMin, 1620.0)
        self.assertLess(
            top_post.Shape.distToShape(stair.ConcreteGeometry.Shape)[0],
            1e-7,
        )

    def test_housed_stringer_profile_does_not_loop_on_backtracking_sections(
        self,
    ):
        stations = (0.0, 850.0, 250.0, 1200.0, 1600.0)
        sections = [
            BalancedSection(
                center=(station, 450.0),
                tangent=(1.0, 0.0),
                left=(station, 900.0),
                right=(station, 0.0),
                station=float(index) * 400.0,
                width=900.0,
                flight_index=0,
                riser_index=index + 1,
            )
            for index, station in enumerate(stations)
        ]
        shape = make_housed_stringer_shape(
            sections,
            180.0,
            "Left",
            40.0,
            300.0,
            20.0,
            0.0,
            0.0,
            50.0,
            "Perpendicular",
            30.0,
        )

        self.assertFalse(shape.isNull())
        self.assertTrue(shape.isValid())
        self.assertEqual(len(shape.Solids), 1)
        self.assertEqual(len(shape.Faces), 6)
        self.assertGreaterEqual(shape.BoundBox.XMin, -1e-7)
        self.assertLessEqual(shape.BoundBox.XMax, 1600.0 + 1e-7)
        self.assertGreaterEqual(shape.BoundBox.ZMin, -1e-7)

    def test_balanced_corner_posts_align_with_both_stringers(
        self,
    ):
        stair = make_stair(
            floor_height=2800.0,
            flight_length=3500.0,
            width=1000.0,
            steps=15,
            stair_type="Wood",
        )
        stair.Proxy._updating = True
        try:
            flights = resize_flights(
                stair,
                2,
                length=3500.0,
                width=1000.0,
                rotations=["Left"],
            )
            for flight in flights:
                flight.Proxy._updating = True
                for side in ("Left", "Right"):
                    setattr(
                        flight,
                        f"{side}StringerType",
                        "Housed stringer",
                    )
                    setattr(
                        flight,
                        f"{side}HandrailEnabled",
                        True,
                    )
                flight.Proxy._updating = False
        finally:
            stair.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()

        for side in ("Left", "Right"):
            incoming_stringer = next(
                part
                for part in stair.StringersGroup.Group
                if part.SourceFlight == flights[0] and part.Side == side
            )
            outgoing_stringer = next(
                part
                for part in stair.StringersGroup.Group
                if part.SourceFlight == flights[1] and part.Side == side
            )
            corner_post = next(
                part
                for part in stair.HandrailsGroup.Group
                if part.StairDesignerRole == "HandrailPost"
                and part.SourceFlight == flights[0]
                and part.Side == side
                and part.ElementIndex == 1
            )
            bounds = corner_post.Shape.BoundBox
            distances = [
                corner_post.Shape.distToShape(stringer.Shape)[0]
                for stringer in (
                    incoming_stringer,
                    outgoing_stringer,
                )
            ]
            self.assertTrue(
                all(distance < 1e-7 for distance in distances),
                side,
            )
            self.assertLessEqual(
                bounds.ZMin,
                min(
                    incoming_stringer.Shape.BoundBox.ZMax,
                    outgoing_stringer.Shape.BoundBox.ZMax,
                )
                - stair.HandrailPostBelowStringer.Value,
            )

    def test_concrete_quarter_turn_is_monolithic(self):
        stair = make_stair(
            floor_height=1200.0,
            flight_length=1200.0,
            width=700.0,
            steps=7,
            stair_type="Concrete",
        )
        stair.Proxy._updating = True
        try:
            flights = resize_flights(stair, 2, length=1200.0, width=700.0, rotations=["Left"])
            flights[0].Angle = 0.0
            flights[1].Angle = 90.0
        finally:
            stair.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()

        shape = stair.ConcreteGeometry.Shape
        self.assertFalse(shape.isNull())
        self.assertTrue(shape.isValid())
        self.assertEqual(len(shape.Solids), 1)
        self.assertEqual(sum(flight.NumberOfTreads for flight in get_flights(stair)), 6)

        flights[1].FlightType = "Circular"
        self.document.recompute()
        shape = stair.ConcreteGeometry.Shape
        self.assertEqual(stair.GeometryStatus, "Tangential circular stair")
        self.assertFalse(shape.isNull())
        self.assertTrue(shape.isValid())
        self.assertEqual(len(shape.Solids), 1)
        self.assertLess(len(shape.Faces), 80)

        flights[1].Angle = 60.0
        flights[1].Rotation = "Right"
        self.document.recompute()
        shape = stair.ConcreteGeometry.Shape
        self.assertTrue(shape.isValid())
        self.assertEqual(len(shape.Solids), 1)
        self.assertLess(len(shape.Faces), 80)

    def test_make_wood_stair(self):
        stair = make_stair(
            floor_height=2800.0,
            flight_length=3500.0,
            width=1000.0,
            steps=15,
            stair_type="Wood",
            name="Test Stair Designer",
        )
        self.document.recompute()

        self.assertIsNotNone(stair)
        self.assertEqual(stair.Label, "Test Stair Designer")
        self.assertTrue(stair.Name.startswith("Stair"))
        self.assertEqual(stair.Proxy.Type, "StairDesigner")
        self.assertNotIn("Model", stair.PropertiesList)
        self.assertIsNotNone(stair.PlanSketch)
        self.assertEqual(stair.PlanSketch.GeometryCount, 18)
        self.assertIsNotNone(stair.FlightsGroup)
        self.assertIsNotNone(stair.StepsGroup)
        self.assertIsNone(stair.StringersGroup)
        self.assertIsNone(stair.HandrailsGroup)
        self.assertNotIn("LinksGroup", stair.PropertiesList)
        self.assertEqual(str(get_flights(stair)[0].TurnType), "Herse balancing")

        treads = [
            child
            for child in stair.StepsGroup.Group
            if getattr(child, "StairDesignerRole", "") == "Tread"
        ]
        risers = [
            child
            for child in stair.StepsGroup.Group
            if getattr(child, "StairDesignerRole", "") == "Riser"
        ]
        self.assertEqual(len(treads), 14)
        self.assertEqual(len(risers), 14)
        self.assertTrue(all(not child.Shape.isNull() for child in treads + risers))
        self.assertTrue(stair.BlondelCompliant)

        first_riser = sorted(risers, key=lambda child: child.Index)[0]
        riser_box = first_riser.Shape.BoundBox
        self.assertAlmostEqual(riser_box.XMin, 0.0)
        self.assertAlmostEqual(riser_box.XMax, stair.RiserThickness.Value)
        self.assertAlmostEqual(
            riser_box.ZMax,
            stair.RiserHeight.Value - stair.StepThickness.Value,
        )

        ordered_treads = sorted(treads, key=lambda child: child.Index)
        ordered_risers = sorted(risers, key=lambda child: child.Index)
        self.assertAlmostEqual(
            ordered_treads[0].Shape.BoundBox.XMax,
            ordered_risers[1].Shape.BoundBox.XMax + stair.StepRiserOverlap.Value,
        )
        self.assertAlmostEqual(
            ordered_treads[-1].Shape.BoundBox.XMax,
            3500.0,
        )

        stair.PriorityToRiser = True
        self.document.recompute()
        lower_tread_box = ordered_treads[0].Shape.BoundBox
        next_riser_box = ordered_risers[1].Shape.BoundBox
        self.assertAlmostEqual(
            lower_tread_box.XMax,
            next_riser_box.XMin + stair.StepRiserOverlap.Value,
        )
        self.assertAlmostEqual(lower_tread_box.ZMin, next_riser_box.ZMin)

        stair.Proxy._updating = True
        try:
            stair.Nosing = 0.0
            stair.RiserUpperOffset = -10.0
            stair.RiserLowerOffset = -5.0
        finally:
            stair.Proxy._updating = False
        stair.Proxy.rebuild(stair, allow_structure_changes=True)
        self.document.recompute()
        riser_box = first_riser.Shape.BoundBox
        self.assertAlmostEqual(riser_box.XMin, 0.0)
        self.assertAlmostEqual(riser_box.ZMin, -5.0)
        self.assertAlmostEqual(
            riser_box.ZMax,
            stair.RiserHeight.Value - stair.StepThickness.Value + 10.0,
        )

        stair.NumberOfSteps = 16
        self.document.recompute()
        treads = [
            child
            for child in stair.StepsGroup.Group
            if getattr(child, "StairDesignerRole", "") == "Tread"
        ]
        self.assertEqual(len(treads), 15)

    def test_concrete_stair_is_monolithic(self):
        stair = make_stair(stair_type="Concrete")
        self.document.recompute()

        self.assertEqual(stair.Label, "Stair")
        self.assertIsNone(stair.StepsGroup)
        self.assertIsNone(stair.StringersGroup)
        self.assertIsNone(stair.HandrailsGroup)
        self.assertIsNotNone(stair.ConcreteGeometry)
        self.assertFalse(stair.ConcreteGeometry.Shape.isNull())
        self.assertEqual(len(stair.ConcreteGeometry.Shape.Solids), 1)

        shape = stair.ConcreteGeometry.Shape
        self.assertTrue(shape.isValid())
        self.assertAlmostEqual(shape.BoundBox.ZMin, 0.0)
        self.assertAlmostEqual(
            shape.BoundBox.ZMax,
            stair.NumberOfTreads * stair.RiserHeight.Value,
        )
        sloped_edges = []
        for edge in shape.Edges:
            if len(edge.Vertexes) != 2:
                continue
            first = edge.Vertexes[0].Point
            last = edge.Vertexes[1].Point
            delta_x = abs(last.x - first.x)
            delta_z = abs(last.z - first.z)
            if delta_x > 1.0 and delta_z > 1.0 and edge.BoundBox.YLength < 0.01:
                sloped_edges.append((first, last))
        self.assertEqual(len(sloped_edges), 2)
        for first, last in sloped_edges:
            self.assertAlmostEqual(
                abs(last.z - first.z) / abs(last.x - first.x),
                stair.RiserHeight.Value / stair.TreadWidth.Value,
            )
            self.assertAlmostEqual(min(first.x, last.x), stair.TreadWidth.Value)
        enclosing_volume = shape.BoundBox.XLength * shape.BoundBox.YLength * shape.BoundBox.ZLength
        self.assertLess(shape.Volume, enclosing_volume * 0.7)
        initial_volume = shape.Volume

        stair.ConcreteThickness = 0.0
        self.document.recompute()
        minimum_shape = stair.ConcreteGeometry.Shape
        self.assertTrue(minimum_shape.isValid())
        minimum_slopes = []
        for edge in minimum_shape.Edges:
            if len(edge.Vertexes) != 2:
                continue
            first = edge.Vertexes[0].Point
            last = edge.Vertexes[1].Point
            if (
                abs(last.x - first.x) > 1.0
                and abs(last.z - first.z) > 1.0
                and edge.BoundBox.YLength < 0.01
            ):
                minimum_slopes.append((first, last))
        self.assertEqual(len(minimum_slopes), 2)
        for first, last in minimum_slopes:
            self.assertAlmostEqual(min(first.x, last.x), 0.0, delta=0.1)

        stair.ConcreteThickness = stair.RiserHeight.Value * (
            stair.TreadWidth.Value / (stair.TreadWidth.Value**2 + stair.RiserHeight.Value**2) ** 0.5
        )
        self.document.recompute()
        self.assertAlmostEqual(stair.ConcreteGeometry.Shape.Volume, initial_volume)

        stair.ConcreteThickness = stair.ConcreteThickness.Value + 60.0
        self.document.recompute()
        self.assertAlmostEqual(stair.ConcreteGeometry.Shape.BoundBox.ZMin, 0.0)
        self.assertGreater(stair.ConcreteGeometry.Shape.Volume, initial_volume)

        stair.ConcreteThickness = stair.ConcreteThickness.Value - 120.0
        self.document.recompute()
        thinner_shape = stair.ConcreteGeometry.Shape
        self.assertTrue(thinner_shape.isValid())
        self.assertAlmostEqual(thinner_shape.BoundBox.ZMin, 0.0)
        thinner_slopes = []
        for edge in thinner_shape.Edges:
            if len(edge.Vertexes) != 2:
                continue
            first = edge.Vertexes[0].Point
            last = edge.Vertexes[1].Point
            if (
                abs(last.x - first.x) > 1.0
                and abs(last.z - first.z) > 1.0
                and edge.BoundBox.YLength < 0.01
            ):
                thinner_slopes.append((first, last))
        self.assertEqual(len(thinner_slopes), 2)
        for first, last in thinner_slopes:
            self.assertLess(min(first.x, last.x), stair.TreadWidth.Value)
            self.assertAlmostEqual(
                abs(last.z - first.z) / abs(last.x - first.x),
                stair.RiserHeight.Value / stair.TreadWidth.Value,
            )

    def test_abort_transaction_removes_generated_tree(self):
        self.document.UndoMode = 1
        self.document.openTransaction("Create Stair Designer")
        stair = make_stair(
            floor_height=600.0,
            flight_length=800.0,
            width=700.0,
            steps=3,
        )
        resize_flights(stair, 2, length=800.0, width=700.0, rotations=["Right"])
        object_names = [obj.Name for obj in self.document.Objects]
        self.assertNotEqual(self.document.getBookedTransactionID(), 0)

        self.document.abortTransaction()
        self.document.recompute()

        for name in object_names:
            self.assertIsNone(self.document.getObject(name))
        self.assertEqual(self.document.Objects, [])
        self.assertEqual(self.document.getUniqueObjectName("Stair"), "Stair")
