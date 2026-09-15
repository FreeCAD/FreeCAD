# SPDX-License-Identifier: LGPL-2.1-or-later

"""Physical geometry parity tests for NativeIFC export."""

import math
from pathlib import Path
import tempfile
import unittest

import FreeCAD

from nativeifc import backend


class TestIfcGeometryParity(unittest.TestCase):
    """Compare exported IFC geometry with its FreeCAD source geometry."""

    def setUp(self):
        backend.invalidate()
        status = backend.get_status()
        if not status.available:
            self.skipTest(f"IfcOpenShell is unavailable: {status.error}")
        self.ifcopenshell = backend.get_backend()
        self.geom = backend.get_module("ifcopenshell.geom")

    def _reopen(self, model, name):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / name
            model.write(output)
            return self.ifcopenshell.open(output)

    def _ifc_metrics(self, product):
        settings = self.geom.settings()
        settings.set("use-world-coords", True)
        shape = self.geom.create_shape(settings, product)
        vertices = list(zip(*(iter(shape.geometry.verts),) * 3))
        triangles = list(zip(*(iter(shape.geometry.faces),) * 3))
        bounds = tuple(value for axis in zip(*vertices) for value in (min(axis), max(axis)))
        signed_volume = 0.0
        for first, second, third in triangles:
            a = vertices[first]
            b = vertices[second]
            c = vertices[third]
            signed_volume += (
                a[0] * (b[1] * c[2] - b[2] * c[1])
                + a[1] * (b[2] * c[0] - b[0] * c[2])
                + a[2] * (b[0] * c[1] - b[1] * c[0])
            ) / 6.0
        return bounds, abs(signed_volume)

    @staticmethod
    def _shape_metrics(shape):
        bounds = shape.BoundBox
        return (
            tuple(
                value / 1000.0
                for value in (
                    bounds.XMin,
                    bounds.XMax,
                    bounds.YMin,
                    bounds.YMax,
                    bounds.ZMin,
                    bounds.ZMax,
                )
            ),
            shape.Volume / 1_000_000_000.0,
        )

    @classmethod
    def _freecad_metrics(cls, obj):
        return cls._shape_metrics(obj.Shape)

    def _assert_metrics_close(self, expected, actual, *, bounds_absolute=1e-3):
        expected_bounds, expected_volume = expected
        actual_bounds, actual_volume = actual
        for expected_value, actual_value in zip(expected_bounds, actual_bounds):
            self.assertAlmostEqual(expected_value, actual_value, delta=bounds_absolute)
        self.assertTrue(
            math.isclose(expected_volume, actual_volume, rel_tol=3e-3, abs_tol=1e-9),
            f"volume mismatch: expected {expected_volume}, got {actual_volume}",
        )

    @staticmethod
    def _feature(document, name, ifc_type, shape, placement=None):
        obj = document.addObject("Part::Feature", name)
        obj.addProperty("App::PropertyString", "IfcType")
        obj.IfcType = ifc_type
        obj.Shape = shape
        if placement is not None:
            obj.Placement = placement
        return obj

    def test_composite_model_preserves_world_bounds_and_volumes(self):
        import Part

        from nativeifc import ifc_export
        from nativeifc import ifc_tools

        document = FreeCAD.newDocument("IfcCompositeParity")
        try:
            objects = [
                self._feature(
                    document,
                    "Slab",
                    "Slab",
                    Part.makeBox(4000, 2500, 250),
                    FreeCAD.Placement(FreeCAD.Vector(1000, 2000, 500), FreeCAD.Rotation()),
                ),
                self._feature(
                    document,
                    "Column",
                    "Column",
                    Part.makeCylinder(300, 3500),
                    FreeCAD.Placement(FreeCAD.Vector(-1500, 500, 0), FreeCAD.Rotation()),
                ),
                self._feature(
                    document,
                    "Beam",
                    "Beam",
                    Part.makeBox(2500, 200, 300),
                    FreeCAD.Placement(
                        FreeCAD.Vector(500, -1200, 2800),
                        FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 30),
                    ),
                ),
            ]
            document.recompute()
            expected = {obj.Name: self._freecad_metrics(obj) for obj in objects}

            model = ifc_tools.create_ifcfile()
            exported = ifc_export.export_objects(objects, model)
            reopened = self._reopen(model, "composite-parity.ifc")

            self.assertEqual(
                {reopened.by_guid(exported[obj].GlobalId).is_a() for obj in objects},
                {"IfcSlab", "IfcColumn", "IfcBeam"},
            )
            for obj in objects:
                with self.subTest(product=obj.Name):
                    product = reopened.by_guid(exported[obj].GlobalId)
                    self._assert_metrics_close(expected[obj.Name], self._ifc_metrics(product))
        finally:
            FreeCAD.closeDocument(document.Name)

    def test_solid_entity_matrix_preserves_class_and_physical_geometry(self):
        """Exercise the common BIM product classes across both solid exporters."""

        import Part

        from nativeifc import ifc_export
        from nativeifc import ifc_tools

        # The profile-based classes use IfcExtrudedAreaSolid. The other common
        # IFC4 BIM product classes deliberately exercise the faceted-BRep
        # fallback. Schema-specific later additions, such as IfcFence, belong
        # in a separate schema matrix once NativeIFC can select IFC4X3.
        cases = {
            "Beam": "SweptSolid",
            "Column": "SweptSolid",
            "Footing": "SweptSolid",
            "Member": "SweptSolid",
            "Plate": "SweptSolid",
            "Slab": "SweptSolid",
            "Wall": "Brep",
            "Building Element Part": "Brep",
            "Door": "Brep",
            "Window": "Brep",
            "Roof": "Brep",
            "Covering": "Brep",
            "Curtain Wall": "Brep",
            "Railing": "Brep",
            "Ramp": "Brep",
            "Stair": "Brep",
            "Stair Flight": "Brep",
            "Pipe Segment": "Brep",
            "Pipe Fitting": "Brep",
            "Reinforcing Bar": "Brep",
            "Furnishing Element": "Brep",
            "Space": "Brep",
            "Building Element Proxy": "Brep",
        }
        document = FreeCAD.newDocument("IfcEntityMatrixParity")
        try:
            objects = []
            for index, ifc_type in enumerate(cases):
                shape = Part.makeBox(700 + index * 11, 430 + index * 7, 510 + index * 13)
                placement = FreeCAD.Placement(
                    FreeCAD.Vector(index * 1400, (index % 3) * 1100, index * 25),
                    FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), (index % 4) * 7),
                )
                objects.append(
                    self._feature(document, f"Matrix{ifc_type}", ifc_type, shape, placement)
                )
            document.recompute()
            expected = {obj.Name: self._freecad_metrics(obj) for obj in objects}

            model = ifc_tools.create_ifcfile()
            exported = ifc_export.export_objects(objects, model)
            reopened = self._reopen(model, "entity-matrix-parity.ifc")

            for obj in objects:
                with self.subTest(ifc_type=obj.IfcType):
                    product = reopened.by_guid(exported[obj].GlobalId)
                    representation = product.Representation.Representations[0]
                    self.assertEqual(product.is_a(), f"Ifc{obj.IfcType.replace(' ', '')}")
                    self.assertEqual(representation.RepresentationType, cases[obj.IfcType])
                    self._assert_metrics_close(expected[obj.Name], self._ifc_metrics(product))
        finally:
            FreeCAD.closeDocument(document.Name)

    def test_multi_solid_object_preserves_each_solid_and_combined_geometry(self):
        import Part

        from nativeifc import ifc_export
        from nativeifc import ifc_tools

        document = FreeCAD.newDocument("IfcMultiSolidParity")
        try:
            shape = Part.makeCompound(
                [
                    Part.makeBox(900, 600, 500),
                    Part.makeBox(450, 350, 800, FreeCAD.Vector(1300, 250, 100)),
                ]
            )
            proxy = self._feature(
                document,
                "MultiSolidProxy",
                "BuildingElementProxy",
                shape,
                FreeCAD.Placement(
                    FreeCAD.Vector(1700, -900, 350),
                    FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 18),
                ),
            )
            document.recompute()
            expected = self._freecad_metrics(proxy)

            model = ifc_tools.create_ifcfile()
            product = ifc_export.create_product(proxy, None, model)
            reopened = self._reopen(model, "multi-solid-parity.ifc")
            reopened_proxy = reopened.by_guid(product.GlobalId)
            representation = reopened_proxy.Representation.Representations[0]

            self.assertEqual(representation.RepresentationType, "Brep")
            self.assertEqual(len(representation.Items), 2)
            self._assert_metrics_close(expected, self._ifc_metrics(reopened_proxy))
        finally:
            FreeCAD.closeDocument(document.Name)

    def test_opening_preserves_bounds_and_subtracted_volume(self):
        import Part

        from nativeifc import ifc_export
        from nativeifc import ifc_tools

        document = FreeCAD.newDocument("IfcOpeningParity")
        try:
            wall = self._feature(document, "Wall", "Wall", Part.makeBox(4000, 200, 3000))
            wall.addProperty("App::PropertyLength", "Length")
            wall.addProperty("App::PropertyLength", "Width")
            wall.addProperty("App::PropertyLength", "Height")
            wall.addProperty("App::PropertyLink", "Base")
            wall.addProperty("App::PropertyLinkList", "Additions")
            wall.addProperty("App::PropertyLinkList", "Subtractions")
            wall.Length = 4000
            wall.Width = 200
            wall.Height = 3000
            opening = self._feature(
                document,
                "Opening",
                "Opening Element",
                Part.makeBox(900, 400, 2100),
                FreeCAD.Placement(FreeCAD.Vector(500, -100, 0), FreeCAD.Rotation()),
            )
            wall.Subtractions = [opening]
            document.recompute()
            expected_shape = wall.Shape.cut(opening.Shape)
            expected = (
                self._freecad_metrics(wall)[0],
                expected_shape.Volume / 1_000_000_000.0,
            )

            model = ifc_tools.create_ifcfile()
            product = ifc_export.create_product(wall, None, model)
            reopened = self._reopen(model, "opening-parity.ifc")
            reopened_wall = reopened.by_guid(product.GlobalId)

            self.assertEqual(len(reopened_wall.HasOpenings), 1)
            relationship = reopened_wall.HasOpenings[0]
            self.assertEqual(relationship.is_a(), "IfcRelVoidsElement")
            self.assertEqual(relationship.RelatedOpeningElement.is_a(), "IfcOpeningElement")
            self._assert_metrics_close(expected, self._ifc_metrics(reopened_wall))
        finally:
            FreeCAD.closeDocument(document.Name)

    def test_nested_spatial_placements_preserve_world_geometry(self):
        import Part

        from nativeifc import ifc_export
        from nativeifc import ifc_tools

        document = FreeCAD.newDocument("IfcNestedPlacementParity")
        try:
            wall = self._feature(document, "NestedWall", "Wall", Part.makeBox(2000, 200, 2800))

            def container(name, ifc_type):
                obj = document.addObject("App::Part", name)
                obj.addProperty("App::PropertyString", "IfcType")
                obj.IfcType = ifc_type
                return obj

            storey = container("Storey", "Building Storey")
            building = container("Building", "Building")
            site = container("Site", "Site")
            storey.addObject(wall)
            building.addObject(storey)
            site.addObject(building)
            site.Placement = FreeCAD.Placement(
                FreeCAD.Vector(10000, -4000, 0),
                FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 10),
            )
            building.Placement = FreeCAD.Placement(
                FreeCAD.Vector(1500, 500, 0),
                FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 20),
            )
            storey.Placement.Base.z = 3200
            wall.Placement = FreeCAD.Placement(
                FreeCAD.Vector(700, 300, 0),
                FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), -15),
            )
            document.recompute()

            model = ifc_tools.create_ifcfile()
            exported = ifc_export.export_objects([site], model)
            reopened = self._reopen(model, "nested-placement-parity.ifc")

            reopened_wall = reopened.by_guid(exported[wall].GlobalId)
            reopened_storey = reopened.by_guid(exported[storey].GlobalId)
            reopened_building = reopened.by_guid(exported[building].GlobalId)
            reopened_site = reopened.by_guid(exported[site].GlobalId)
            self.assertEqual(
                reopened_wall.ContainedInStructure[0].RelatingStructure,
                reopened_storey,
            )
            self.assertEqual(reopened_storey.Decomposes[0].RelatingObject, reopened_building)
            self.assertEqual(reopened_building.Decomposes[0].RelatingObject, reopened_site)

            expected_wall_shape = Part.makeBox(2000, 200, 2800)
            expected_wall_shape.Placement = wall.getGlobalPlacement()
            self._assert_metrics_close(
                self._shape_metrics(expected_wall_shape), self._ifc_metrics(reopened_wall)
            )
        finally:
            FreeCAD.closeDocument(document.Name)

    def test_circular_profile_preserves_semantics_and_physical_geometry(self):
        import Part

        from nativeifc import ifc_export
        from nativeifc import ifc_tools

        document = FreeCAD.newDocument("IfcCircularParity")
        try:
            column = self._feature(
                document,
                "CircularColumn",
                "Column",
                Part.makeCylinder(350, 4200),
                FreeCAD.Placement(
                    FreeCAD.Vector(1250, -750, 300),
                    FreeCAD.Rotation(FreeCAD.Vector(1, 0, 0), 12),
                ),
            )
            document.recompute()
            expected = self._freecad_metrics(column)

            model = ifc_tools.create_ifcfile()
            product = ifc_export.create_product(column, None, model)
            reopened = self._reopen(model, "circular-parity.ifc")
            reopened_column = reopened.by_guid(product.GlobalId)
            item = reopened_column.Representation.Representations[0].Items[0]

            self.assertEqual(item.SweptArea.is_a(), "IfcCircleProfileDef")
            self.assertAlmostEqual(item.SweptArea.Radius, 0.35)
            self._assert_metrics_close(expected, self._ifc_metrics(reopened_column))
        finally:
            FreeCAD.closeDocument(document.Name)


if __name__ == "__main__":
    unittest.main()
