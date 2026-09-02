# SPDX-License-Identifier: LGPL-2.1-or-later

"""Deterministic IfcOpenShell and NativeIFC round-trip tests."""

from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import FreeCAD

from nativeifc import backend

FIXTURES = Path(__file__).parent / "fixtures"


class TestIfcRoundtrip(unittest.TestCase):
    def setUp(self):
        backend.invalidate()
        status = backend.get_status()
        if not status.available:
            self.skipTest(f"IfcOpenShell is unavailable: {status.error}")
        self.ifcopenshell = backend.get_backend()

    def test_schema_geometry_and_units(self):
        import ifcopenshell.geom
        import ifcopenshell.util.unit

        fixtures = (("roundtrip_ifc2x3.ifc", "IFC2X3"), ("roundtrip_ifc4.ifc", "IFC4"))
        for filename, schema in fixtures:
            with self.subTest(schema=schema):
                model = self.ifcopenshell.open(FIXTURES / filename)
                self.assertEqual(model.schema, schema)
                self.assertAlmostEqual(ifcopenshell.util.unit.calculate_unit_scale(model), 0.001)

                settings = ifcopenshell.geom.settings()
                shape = ifcopenshell.geom.create_shape(settings, model.by_type("IfcWall")[0])
                self.assertGreater(len(shape.geometry.verts), 0)
                self.assertGreater(len(shape.geometry.faces), 0)

    def test_ifcopenshell_edit_write_reopen(self):
        model = self.ifcopenshell.open(FIXTURES / "roundtrip_ifc4.ifc")
        wall = model.by_type("IfcWall")[0]
        global_id = wall.GlobalId
        wall.Name = "Edited Wall"

        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "roundtrip.ifc"
            model.write(output)
            reopened = self.ifcopenshell.open(output)

        reopened_wall = reopened.by_guid(global_id)
        self.assertEqual(reopened_wall.Name, "Edited Wall")
        self.assertEqual(len(reopened.by_type("IfcWall")), 1)

    def test_nativeifc_import_edit_save_reopen(self):
        from nativeifc import ifc_import
        from nativeifc import ifc_tools

        document = FreeCAD.newDocument("IfcRoundtrip")
        try:
            ifc_import.insert(
                str(FIXTURES / "roundtrip_ifc4.ifc"),
                document.Name,
                strategy=2,
                shapemode=2,
                switchwb=False,
                silent=True,
                singledoc=False,
            )
            projects = [
                obj for obj in document.Objects if hasattr(getattr(obj, "Proxy", None), "ifcfile")
            ]
            self.assertEqual(len(projects), 1)

            project = projects[0]
            wall = project.Proxy.ifcfile.by_type("IfcWall")[0]
            global_id = wall.GlobalId
            ifc_tools.set_attribute(project.Proxy.ifcfile, wall, "Name", "NativeIFC Edited Wall")

            with tempfile.TemporaryDirectory() as directory:
                output = Path(directory) / "nativeifc-roundtrip.ifc"
                ifc_tools.save_ifc(project, str(output))
                reopened = self.ifcopenshell.open(output)

            self.assertEqual(reopened.by_guid(global_id).Name, "NativeIFC Edited Wall")
        finally:
            FreeCAD.closeDocument(document.Name)

    def test_simple_wall_uses_direct_geometry_api(self):
        import Part
        import ifcopenshell.geom

        from nativeifc import ifc_geometry_export
        from nativeifc import ifc_tools

        document = FreeCAD.newDocument("IfcDirectWall")
        try:
            wall = document.addObject("Part::Feature", "Wall")
            wall.addProperty("App::PropertyString", "IfcType")
            wall.addProperty("App::PropertyLength", "Length")
            wall.addProperty("App::PropertyLength", "Width")
            wall.addProperty("App::PropertyLength", "Height")
            wall.addProperty("App::PropertyLink", "Base")
            wall.addProperty("App::PropertyLinkList", "Additions")
            wall.addProperty("App::PropertyLinkList", "Subtractions")
            wall.IfcType = "Wall"
            wall.Length = 2000
            wall.Width = 200
            wall.Height = 3000
            shape = Part.makeBox(2000, 200, 3000)
            shape.translate(FreeCAD.Vector(-1000, -100, 0))
            wall.Shape = shape
            document.recompute()
            model = ifc_tools.create_ifcfile()
            context = next(
                item
                for item in model.by_type("IfcGeometricRepresentationSubContext")
                if item.ContextIdentifier == "Body"
            )

            exporter = ifc_geometry_export.GeometryExporter(model)
            representation, placement = exporter.create_representation(context, wall, {})

            product = model.createIfcWall(
                self.ifcopenshell.guid.new(),
                None,
                "Direct Wall",
                None,
                None,
                placement,
                representation,
                None,
                None,
            )
            self.assertEqual(representation.is_a(), "IfcProductDefinitionShape")
            self.assertEqual(
                representation.Representations[0].RepresentationType,
                "SweptSolid",
            )

            settings = ifcopenshell.geom.settings()
            settings.set("use-world-coords", True)
            shape = ifcopenshell.geom.create_shape(settings, product)
            vertices = list(zip(*(iter(shape.geometry.verts),) * 3))
            extents = [max(axis) - min(axis) for axis in zip(*vertices)]
            self.assertAlmostEqual(extents[0], 2.0)
            self.assertAlmostEqual(extents[1], 0.2)
            self.assertAlmostEqual(extents[2], 3.0)
        finally:
            FreeCAD.closeDocument(document.Name)

    def test_rectangular_slab_uses_direct_geometry_api(self):
        import Part
        import ifcopenshell.geom

        from nativeifc import ifc_geometry_export
        from nativeifc import ifc_tools

        document = FreeCAD.newDocument("IfcDirectSlab")
        try:
            slab = document.addObject("Part::Feature", "Slab")
            slab.addProperty("App::PropertyString", "IfcType")
            slab.IfcType = "Slab"
            slab.Shape = Part.makeBox(4000, 2500, 250)
            slab.Placement.Base = FreeCAD.Vector(1000, 2000, 3000)
            document.recompute()
            model = ifc_tools.create_ifcfile()
            context = next(
                item
                for item in model.by_type("IfcGeometricRepresentationSubContext")
                if item.ContextIdentifier == "Body"
            )

            exporter = ifc_geometry_export.GeometryExporter(model)
            with patch(
                "importers.exportIFC.getRepresentation",
                side_effect=AssertionError("legacy geometry fallback was used"),
            ):
                representation, placement = exporter.create_representation(context, slab, {})

            product = model.createIfcSlab(
                self.ifcopenshell.guid.new(),
                None,
                "Direct Slab",
                None,
                None,
                placement,
                representation,
                None,
                None,
            )
            item = representation.Representations[0].Items[0]
            self.assertEqual(item.SweptArea.is_a(), "IfcRectangleProfileDef")

            settings = ifcopenshell.geom.settings()
            settings.set("use-world-coords", True)
            shape = ifcopenshell.geom.create_shape(settings, product)
            vertices = list(zip(*(iter(shape.geometry.verts),) * 3))
            extents = [max(axis) - min(axis) for axis in zip(*vertices)]
            self.assertAlmostEqual(extents[0], 4.0)
            self.assertAlmostEqual(extents[1], 2.5)
            self.assertAlmostEqual(extents[2], 0.25)
        finally:
            FreeCAD.closeDocument(document.Name)

    def test_rotated_profile_with_void_uses_direct_geometry_api(self):
        import Part
        import ifcopenshell.geom

        from nativeifc import ifc_geometry_export
        from nativeifc import ifc_tools

        document = FreeCAD.newDocument("IfcDirectProfile")
        try:
            vector = FreeCAD.Vector
            outer = Part.makePolygon(
                [
                    vector(0, 0, 0),
                    vector(0, 3000, 0),
                    vector(0, 3000, 2000),
                    vector(0, 0, 2000),
                    vector(0, 0, 0),
                ]
            )
            inner = Part.makePolygon(
                [
                    vector(0, 1000, 500),
                    vector(0, 2000, 500),
                    vector(0, 2000, 1500),
                    vector(0, 1000, 1500),
                    vector(0, 1000, 500),
                ]
            )
            beam = document.addObject("Part::Feature", "Beam")
            beam.addProperty("App::PropertyString", "IfcType")
            beam.IfcType = "Beam"
            beam.Shape = Part.Face([outer, inner]).extrude(vector(500, 0, 0))
            beam.Placement.Base = vector(100, 200, 300)
            document.recompute()
            model = ifc_tools.create_ifcfile()
            context = next(
                item
                for item in model.by_type("IfcGeometricRepresentationSubContext")
                if item.ContextIdentifier == "Body"
            )

            exporter = ifc_geometry_export.GeometryExporter(model)
            with patch(
                "importers.exportIFC.getRepresentation",
                side_effect=AssertionError("legacy geometry fallback was used"),
            ):
                representation, placement = exporter.create_representation(context, beam, {})

            product = model.createIfcBeam(
                self.ifcopenshell.guid.new(),
                None,
                "Direct Profile Beam",
                None,
                None,
                placement,
                representation,
                None,
                None,
            )
            profile = representation.Representations[0].Items[0].SweptArea
            self.assertEqual(profile.is_a(), "IfcArbitraryProfileDefWithVoids")
            self.assertEqual(len(profile.InnerCurves), 1)

            settings = ifcopenshell.geom.settings()
            settings.set("use-world-coords", True)
            shape = ifcopenshell.geom.create_shape(settings, product)
            vertices = list(zip(*(iter(shape.geometry.verts),) * 3))
            bounds = [(min(axis), max(axis)) for axis in zip(*vertices)]
            expected = [(0.1, 0.6), (0.2, 3.2), (0.3, 2.3)]
            for actual, target in zip(bounds, expected):
                self.assertAlmostEqual(actual[0], target[0])
                self.assertAlmostEqual(actual[1], target[1])
        finally:
            FreeCAD.closeDocument(document.Name)

    def test_circular_profiles_use_direct_geometry_api(self):
        import Part
        import ifcopenshell.geom

        from nativeifc import ifc_geometry_export
        from nativeifc import ifc_tools

        document = FreeCAD.newDocument("IfcDirectCircularProfiles")
        try:
            vector = FreeCAD.Vector
            solid = document.addObject("Part::Feature", "SolidColumn")
            solid.addProperty("App::PropertyString", "IfcType")
            solid.IfcType = "Column"
            solid.Shape = Part.makeCylinder(200, 3000)

            hollow = document.addObject("Part::Feature", "HollowBeam")
            hollow.addProperty("App::PropertyString", "IfcType")
            hollow.IfcType = "Beam"
            outer = Part.makeCylinder(300, 2000, vector(), vector(1, 0, 0))
            inner = Part.makeCylinder(250, 2000, vector(), vector(1, 0, 0))
            hollow.Shape = outer.cut(inner)
            hollow.Placement.Base = vector(1000, 2000, 3000)
            document.recompute()

            model = ifc_tools.create_ifcfile()
            context = next(
                item
                for item in model.by_type("IfcGeometricRepresentationSubContext")
                if item.ContextIdentifier == "Body"
            )
            exporter = ifc_geometry_export.GeometryExporter(model)
            with patch(
                "importers.exportIFC.getRepresentation",
                side_effect=AssertionError("legacy geometry fallback was used"),
            ):
                solid_representation, solid_placement = exporter.create_representation(
                    context, solid, {}
                )
                hollow_representation, hollow_placement = exporter.create_representation(
                    context, hollow, {}
                )

            solid_profile = solid_representation.Representations[0].Items[0].SweptArea
            hollow_profile = hollow_representation.Representations[0].Items[0].SweptArea
            self.assertEqual(solid_profile.is_a(), "IfcCircleProfileDef")
            self.assertEqual(hollow_profile.is_a(), "IfcCircleHollowProfileDef")
            self.assertAlmostEqual(hollow_profile.Radius, 0.3)
            self.assertAlmostEqual(hollow_profile.WallThickness, 0.05)

            product = model.createIfcBeam(
                self.ifcopenshell.guid.new(),
                None,
                "Direct Hollow Beam",
                None,
                None,
                hollow_placement,
                hollow_representation,
                None,
                None,
            )
            settings = ifcopenshell.geom.settings()
            settings.set("use-world-coords", True)
            shape = ifcopenshell.geom.create_shape(settings, product)
            vertices = list(zip(*(iter(shape.geometry.verts),) * 3))
            bounds = [(min(axis), max(axis)) for axis in zip(*vertices)]
            expected = [(1.0, 3.0), (1.7, 2.3), (2.7, 3.3)]
            for actual, target in zip(bounds, expected):
                self.assertAlmostEqual(actual[0], target[0], delta=0.001)
                self.assertAlmostEqual(actual[1], target[1], delta=0.001)
        finally:
            FreeCAD.closeDocument(document.Name)

    def test_structural_profiles_use_parameterized_ifc_profiles(self):
        import Arch
        import Part
        import ifcopenshell.util.element

        from nativeifc import ifc_export
        from nativeifc import ifc_geometry_export
        from nativeifc import ifc_tools

        document = FreeCAD.newDocument("IfcDirectStructuralProfiles")
        try:
            profiles = {
                "I": (
                    "IfcIShapeProfileDef",
                    [
                        (0, 0),
                        (400, 0),
                        (400, 50),
                        (225, 50),
                        (225, 550),
                        (400, 550),
                        (400, 600),
                        (0, 600),
                        (0, 550),
                        (175, 550),
                        (175, 50),
                        (0, 50),
                    ],
                ),
                "T": (
                    "IfcTShapeProfileDef",
                    [
                        (0, 600),
                        (400, 600),
                        (400, 550),
                        (225, 550),
                        (225, 0),
                        (175, 0),
                        (175, 550),
                        (0, 550),
                    ],
                ),
                "U": (
                    "IfcUShapeProfileDef",
                    [
                        (0, 0),
                        (400, 0),
                        (400, 50),
                        (50, 50),
                        (50, 550),
                        (400, 550),
                        (400, 600),
                        (0, 600),
                    ],
                ),
                "L": (
                    "IfcLShapeProfileDef",
                    [(0, 0), (400, 0), (400, 50), (50, 50), (50, 600), (0, 600)],
                ),
            }
            objects = []
            steel = Arch.makeMaterial(name="Structural steel")
            for name, (_ifc_profile, coordinates) in profiles.items():
                points = [FreeCAD.Vector(x, y, 0) for x, y in coordinates]
                points.append(points[0])
                obj = document.addObject("Part::Feature", name + "Profile")
                obj.addProperty("App::PropertyString", "IfcType")
                obj.addProperty("App::PropertyLink", "Material")
                obj.IfcType = "Beam"
                obj.Material = steel
                obj.Shape = Part.Face(Part.makePolygon(points)).extrude(FreeCAD.Vector(0, 0, 1000))
                objects.append((obj, profiles[name][0]))
            document.recompute()

            model = ifc_tools.create_ifcfile()
            context = next(
                item
                for item in model.by_type("IfcGeometricRepresentationSubContext")
                if item.ContextIdentifier == "Body"
            )
            exporter = ifc_geometry_export.GeometryExporter(model)
            with patch(
                "importers.exportIFC.getRepresentation",
                side_effect=AssertionError("legacy geometry fallback was used"),
            ):
                for obj, expected_profile in objects:
                    representation, _placement = exporter.create_representation(context, obj, {})
                    profile = representation.Representations[0].Items[0].SweptArea
                    self.assertEqual(profile.is_a(), expected_profile)
                    depth = getattr(profile, "Depth", None)
                    if depth is None:
                        depth = profile.OverallDepth
                    self.assertAlmostEqual(depth, 0.6)

            product = ifc_export.create_product(objects[0][0], None, model)
            usage = ifcopenshell.util.element.get_material(product)
            self.assertEqual(usage.is_a(), "IfcMaterialProfileSetUsage")
            material_profile = usage.ForProfileSet.MaterialProfiles[0]
            self.assertEqual(material_profile.Material.Name, "Structural steel")
            self.assertEqual(
                material_profile.Profile.id(),
                product.Representation.Representations[0].Items[0].SweptArea.id(),
            )
        finally:
            FreeCAD.closeDocument(document.Name)

    def test_wall_subtraction_exports_semantic_opening(self):
        import Part

        from nativeifc import ifc_export
        from nativeifc import ifc_tools

        document = FreeCAD.newDocument("IfcSemanticOpening")
        try:
            wall = document.addObject("Part::Feature", "Wall")
            wall.addProperty("App::PropertyString", "IfcType")
            wall.addProperty("App::PropertyLength", "Length")
            wall.addProperty("App::PropertyLength", "Width")
            wall.addProperty("App::PropertyLength", "Height")
            wall.addProperty("App::PropertyLink", "Base")
            wall.addProperty("App::PropertyLinkList", "Additions")
            wall.addProperty("App::PropertyLinkList", "Subtractions")
            wall.IfcType = "Wall"
            wall.Length = 4000
            wall.Width = 200
            wall.Height = 3000
            wall.Shape = Part.makeBox(4000, 200, 3000)

            subtraction = document.addObject("Part::Feature", "DoorOpening")
            opening_shape = Part.makeBox(900, 400, 2100)
            opening_shape.translate(FreeCAD.Vector(500, -100, 0))
            subtraction.Shape = opening_shape
            wall.Subtractions = [subtraction]
            document.recompute()

            model = ifc_tools.create_ifcfile()
            product = ifc_export.create_product(wall, None, model)

            self.assertEqual(product.is_a(), "IfcWall")
            self.assertEqual(len(product.HasOpenings), 1)
            relationship = product.HasOpenings[0]
            self.assertEqual(relationship.is_a(), "IfcRelVoidsElement")
            opening = relationship.RelatedOpeningElement
            self.assertEqual(opening.is_a(), "IfcOpeningElement")
            self.assertEqual(
                opening.Representation.Representations[0].RepresentationType,
                "SweptSolid",
            )

            with tempfile.TemporaryDirectory() as directory:
                output = Path(directory) / "semantic-opening.ifc"
                model.write(output)
                reopened = self.ifcopenshell.open(output)
            reopened_wall = reopened.by_guid(product.GlobalId)
            self.assertEqual(len(reopened_wall.HasOpenings), 1)
            self.assertEqual(
                reopened_wall.HasOpenings[0].RelatedOpeningElement.GlobalId,
                opening.GlobalId,
            )
        finally:
            FreeCAD.closeDocument(document.Name)

    def test_linked_material_is_reused_and_survives_roundtrip(self):
        import Arch
        import Part
        import ifcopenshell.util.element

        from nativeifc import ifc_export
        from nativeifc import ifc_tools

        document = FreeCAD.newDocument("IfcMaterialReuse")
        try:
            material = Arch.makeMaterial(name="Structural concrete")
            products = []
            model = ifc_tools.create_ifcfile()
            for index in range(2):
                slab = document.addObject("Part::Feature", f"Slab{index}")
                slab.addProperty("App::PropertyString", "IfcType")
                slab.addProperty("App::PropertyLink", "Material")
                slab.IfcType = "Slab"
                slab.Material = material
                slab.Shape = Part.makeBox(2000, 1000, 200)
                products.append(ifc_export.create_product(slab, None, model))

            self.assertEqual(len(model.by_type("IfcMaterial")), 1)
            for product in products:
                assigned = ifcopenshell.util.element.get_material(product)
                self.assertEqual(assigned.is_a(), "IfcMaterial")
                self.assertEqual(assigned.Name, "Structural concrete")

            with tempfile.TemporaryDirectory() as directory:
                output = Path(directory) / "material-reuse.ifc"
                model.write(output)
                reopened = self.ifcopenshell.open(output)
            self.assertEqual(len(reopened.by_type("IfcMaterial")), 1)
            self.assertEqual(
                ifcopenshell.util.element.get_material(reopened.by_guid(products[0].GlobalId)).Name,
                "Structural concrete",
            )
        finally:
            FreeCAD.closeDocument(document.Name)

    def test_explicit_property_sets_and_quantities_survive_roundtrip(self):
        import Part
        import ifcopenshell.util.element

        from nativeifc import ifc_export
        from nativeifc import ifc_tools

        document = FreeCAD.newDocument("IfcSemanticProperties")
        try:
            wall = document.addObject("Part::Feature", "PropertyWall")
            wall.addProperty("App::PropertyString", "IfcType")
            wall.IfcType = "Wall"
            wall.Shape = Part.makeBox(4000, 200, 3000)
            wall.addProperty(
                "App::PropertyString",
                "FireRating",
                "Pset_WallCommon",
                "IfcLabel:FireRating",
            )
            wall.addProperty(
                "App::PropertyBool",
                "IsExternal",
                "Pset_WallCommon",
                "IfcBoolean:IsExternal",
            )
            wall.addProperty("App::PropertyLength", "ReferenceLength", "Pset_FreeCADTest")
            wall.addProperty("App::PropertyLength", "Length", "Qto_WallBaseQuantities")
            wall.addProperty("App::PropertyArea", "NetSideArea", "Qto_WallBaseQuantities")
            wall.addProperty("App::PropertyVolume", "NetVolume", "Qto_WallBaseQuantities")
            wall.addProperty("App::PropertyString", "InternalNote", "Arch")
            wall.FireRating = "2HR"
            wall.IsExternal = True
            wall.ReferenceLength = 1250
            wall.Length = 4000
            wall.NetSideArea = 12_000_000
            wall.NetVolume = 2_400_000_000
            wall.InternalNote = "must not leak"
            document.recompute()

            model = ifc_tools.create_ifcfile()
            product = ifc_export.create_product(wall, None, model)
            data = ifcopenshell.util.element.get_psets(product, qtos_only=False)
            self.assertEqual(data["Pset_WallCommon"]["FireRating"], "2HR")
            self.assertTrue(data["Pset_WallCommon"]["IsExternal"])
            self.assertNotIn("InternalNote", data["Pset_WallCommon"])
            custom_pset = next(
                pset for pset in model.by_type("IfcPropertySet") if pset.Name == "Pset_FreeCADTest"
            )
            self.assertEqual(custom_pset.HasProperties[0].NominalValue.is_a(), "IfcLengthMeasure")
            self.assertAlmostEqual(data["Pset_FreeCADTest"]["ReferenceLength"], 1.25)
            self.assertAlmostEqual(data["Qto_WallBaseQuantities"]["Length"], 4.0)
            self.assertAlmostEqual(data["Qto_WallBaseQuantities"]["NetSideArea"], 12.0)
            self.assertAlmostEqual(data["Qto_WallBaseQuantities"]["NetVolume"], 2.4)

            with tempfile.TemporaryDirectory() as directory:
                output = Path(directory) / "semantic-properties.ifc"
                model.write(output)
                reopened = self.ifcopenshell.open(output)
            reopened_data = ifcopenshell.util.element.get_psets(
                reopened.by_guid(product.GlobalId), qtos_only=False
            )
            self.assertEqual(reopened_data["Pset_WallCommon"]["FireRating"], "2HR")
            self.assertAlmostEqual(reopened_data["Qto_WallBaseQuantities"]["NetVolume"], 2.4)
        finally:
            FreeCAD.closeDocument(document.Name)

    def test_classification_and_logical_group_survive_roundtrip(self):
        import Part

        from nativeifc import ifc_export
        from nativeifc import ifc_tools

        document = FreeCAD.newDocument("IfcClassificationGroup")
        try:
            system = document.addObject("App::FeaturePython", "Uniclass")
            system.addProperty("App::PropertyString", "ClassificationName")
            system.addProperty("App::PropertyString", "Source")
            system.addProperty("App::PropertyString", "Edition")
            system.addProperty("App::PropertyString", "EditionDate")
            system.addProperty("App::PropertyString", "Location")
            system.ClassificationName = "Uniclass 2015"
            system.Source = "NBS"
            system.Edition = "2015"
            system.EditionDate = "2024-01-01"
            system.Location = "https://uniclass.thenbs.com/"

            reference = document.addObject("App::FeaturePython", "WallClassification")
            reference.Label = "Wall classification"
            reference.addProperty("App::PropertyLink", "ReferencedSource")
            reference.addProperty("App::PropertyString", "Identification")
            reference.addProperty("App::PropertyString", "ReferenceName")
            reference.addProperty("App::PropertyString", "Location")
            reference.addProperty("App::PropertyString", "Description")
            reference.ReferencedSource = system
            reference.Identification = "EF_25_10"
            reference.ReferenceName = "Walls"
            reference.Location = "https://uniclass.thenbs.com/table/ef/25/10"
            reference.Description = "Wall functional systems"

            group = document.addObject("App::DocumentObjectGroup", "FireZoneA")
            group.Label = "Fire zone A"
            group.addProperty("App::PropertyString", "Description")
            group.Description = "Products in fire compartment A"

            model = ifc_tools.create_ifcfile()
            products = []
            for index in range(2):
                wall = document.addObject("Part::Feature", f"ClassifiedWall{index}")
                wall.addProperty("App::PropertyString", "IfcType")
                wall.addProperty("App::PropertyLink", "ClassificationReference")
                wall.IfcType = "Wall"
                wall.ClassificationReference = reference
                wall.Shape = Part.makeBox(2000, 200, 3000)
                group.addObject(wall)
                products.append(ifc_export.create_product(wall, None, model))

            classifications = model.by_type("IfcClassification")
            self.assertEqual(len(classifications), 1)
            self.assertEqual(classifications[0].Name, "Uniclass 2015")
            self.assertEqual(classifications[0].Source, "NBS")
            self.assertEqual(classifications[0].Edition, "2015")
            self.assertEqual(classifications[0].EditionDate, "2024-01-01")
            self.assertEqual(classifications[0].Location, "https://uniclass.thenbs.com/")

            references = model.by_type("IfcClassificationReference")
            self.assertEqual(len(references), 1)
            self.assertEqual(references[0].Identification, "EF_25_10")
            self.assertEqual(references[0].Name, "Walls")
            self.assertEqual(references[0].Description, "Wall functional systems")
            classification_rel = model.by_type("IfcRelAssociatesClassification")
            self.assertEqual(len(classification_rel), 1)
            self.assertEqual(set(classification_rel[0].RelatedObjects), set(products))

            groups = model.by_type("IfcGroup")
            self.assertEqual(len(groups), 1)
            self.assertEqual(groups[0].Name, "Fire zone A")
            group_rel = model.by_type("IfcRelAssignsToGroup")
            self.assertEqual(len(group_rel), 1)
            self.assertEqual(set(group_rel[0].RelatedObjects), set(products))

            with tempfile.TemporaryDirectory() as directory:
                output = Path(directory) / "classification-group.ifc"
                model.write(output)
                reopened = self.ifcopenshell.open(output)
            self.assertEqual(len(reopened.by_type("IfcClassification")), 1)
            reopened_reference = reopened.by_type("IfcClassificationReference")[0]
            self.assertEqual(reopened_reference.Identification, "EF_25_10")
            self.assertEqual(
                len(reopened.by_type("IfcRelAssociatesClassification")[0].RelatedObjects), 2
            )
            self.assertEqual(len(reopened.by_type("IfcGroup")), 1)
            self.assertEqual(len(reopened.by_type("IfcRelAssignsToGroup")[0].RelatedObjects), 2)
        finally:
            FreeCAD.closeDocument(document.Name)

    def test_ifc2x3_classification_uses_calendar_date(self):
        from nativeifc import ifc_classification

        model = self.ifcopenshell.file(schema="IFC2X3")
        data = {
            "system_name": "Uniclass 2015",
            "source": "NBS",
            "edition": "2015",
            "edition_date": "2024-01-01",
            "system_location": "https://uniclass.thenbs.com/",
            "identification": "EF_25_10",
            "name": "Walls",
            "location": "https://uniclass.thenbs.com/table/ef/25/10",
            "description": "Wall functional systems",
        }

        classification = ifc_classification._classification_system(model, data)
        exported_reference = ifc_classification._classification_reference(
            model, classification, data
        )

        self.assertEqual(classification.EditionDate.YearComponent, 2024)
        self.assertEqual(classification.EditionDate.MonthComponent, 1)
        self.assertEqual(classification.EditionDate.DayComponent, 1)
        self.assertEqual(exported_reference.ItemReference, "EF_25_10")

    def test_native_document_export_preserves_semantic_hierarchy(self):
        import Arch
        import Part

        from nativeifc import ifc_export
        from nativeifc import ifc_tools

        document = FreeCAD.newDocument("IfcDocumentExport")
        try:
            wall = document.addObject("Part::Feature", "HierarchyWall")
            wall.Label = "Classified wall"
            wall.addProperty("App::PropertyString", "IfcType")
            wall.addProperty("App::PropertyString", "Classification")
            wall.IfcType = "Wall"
            wall.Classification = "Uniclass EF_25_10"
            wall.Shape = Part.makeBox(3000, 200, 2800)

            group = document.addObject("App::DocumentObjectGroup", "FireZoneA")
            group.Label = "Fire zone A"
            group.addObject(wall)
            storey = Arch.makeFloor([group], name="GroundFloor")
            building = Arch.makeBuilding([storey], name="TestBuilding")
            site = Arch.makeSite([building], name="TestSite")
            document.recompute()

            model = ifc_tools.create_ifcfile()
            exported = ifc_export.export_objects([site], model)

            self.assertEqual(exported[site].is_a(), "IfcSite")
            self.assertEqual(exported[building].is_a(), "IfcBuilding")
            self.assertEqual(exported[storey].is_a(), "IfcBuildingStorey")
            self.assertEqual(exported[wall].is_a(), "IfcWall")
            self.assertEqual(exported[site].Decomposes[0].RelatingObject.is_a(), "IfcProject")
            self.assertEqual(exported[building].Decomposes[0].RelatingObject, exported[site])
            self.assertEqual(exported[storey].Decomposes[0].RelatingObject, exported[building])
            self.assertEqual(
                exported[wall].ContainedInStructure[0].RelatingStructure,
                exported[storey],
            )
            self.assertEqual(len(model.by_type("IfcClassificationReference")), 1)
            self.assertEqual(len(model.by_type("IfcGroup")), 1)
            self.assertEqual(
                model.by_type("IfcRelAssignsToGroup")[0].RelatedObjects,
                (exported[wall],),
            )

            with tempfile.TemporaryDirectory() as directory:
                output = Path(directory) / "native-document-export.ifc"
                model.write(output)
                reopened = self.ifcopenshell.open(output)
            reopened_wall = reopened.by_guid(exported[wall].GlobalId)
            self.assertEqual(
                reopened_wall.ContainedInStructure[0].RelatingStructure.is_a(),
                "IfcBuildingStorey",
            )
            self.assertEqual(len(reopened.by_type("IfcRelAggregates")), 3)
            self.assertEqual(len(reopened.by_type("IfcRelAssociatesClassification")), 1)
            self.assertEqual(len(reopened.by_type("IfcRelAssignsToGroup")), 1)
        finally:
            FreeCAD.closeDocument(document.Name)

    def test_native_file_export_writes_reopenable_ifc(self):
        import Part

        from nativeifc import ifc_export

        document = FreeCAD.newDocument("IfcFileExport")
        try:
            wall = document.addObject("Part::Feature", "FileWall")
            wall.Label = "File wall"
            wall.addProperty("App::PropertyString", "IfcType")
            wall.IfcType = "Wall"
            wall.Shape = Part.makeBox(2500, 200, 2800)
            document.recompute()

            with tempfile.TemporaryDirectory() as directory:
                output = Path(directory) / "native-file-export.ifc"
                exported = ifc_export.export_file([wall], output)
                reopened = self.ifcopenshell.open(output)

            self.assertIn(wall, exported)
            reopened_wall = reopened.by_guid(exported[wall].GlobalId)
            self.assertEqual(reopened_wall.Name, "File wall")
            self.assertIsNotNone(reopened_wall.Representation)
        finally:
            FreeCAD.closeDocument(document.Name)

    def test_annotations_use_native_ifc_geometry_and_survive_roundtrip(self):
        import Arch
        import Draft

        from nativeifc import ifc_export
        from nativeifc import ifc_tools

        document = FreeCAD.newDocument("IfcNativeAnnotations")
        try:
            wire = Draft.make_wire(
                [
                    FreeCAD.Vector(0, 0, 0),
                    FreeCAD.Vector(1000, 0, 0),
                    FreeCAD.Vector(1000, 500, 0),
                ]
            )
            wire.Label = "Plan polyline"
            text = Draft.make_text(["First line", "Second line"], FreeCAD.Vector(250, 300, 0))
            dimension = Draft.make_dimension(
                FreeCAD.Vector(0, 0, 0),
                FreeCAD.Vector(1000, 0, 0),
                FreeCAD.Vector(0, 250, 0),
            )
            section = Arch.makeSectionPlane(name="Test drawing plane")
            section.Depth = 500
            document.recompute()

            model = ifc_tools.create_ifcfile()
            annotations = [
                ifc_export.create_annotation(obj, model) for obj in (wire, text, dimension, section)
            ]

            wire_item = annotations[0].Representation.Representations[0].Items[0].Elements[0]
            self.assertEqual(wire_item.is_a(), "IfcIndexedPolyCurve")
            text_item = annotations[1].Representation.Representations[0].Items[0]
            self.assertEqual(text_item.is_a(), "IfcTextLiteral")
            self.assertEqual(text_item.Literal, "First line;Second line")
            dimension_items = annotations[2].Representation.Representations[0].Items
            self.assertEqual(
                [item.is_a() for item in dimension_items],
                ["IfcGeometricCurveSet", "IfcTextLiteral"],
            )
            self.assertEqual(annotations[2].ObjectType, "DIMENSION")
            section_representation = annotations[3].Representation.Representations[0]
            self.assertEqual(section_representation.RepresentationType, "CSG")
            self.assertEqual(section_representation.Items[0].is_a(), "IfcCsgSolid")

            with tempfile.TemporaryDirectory() as directory:
                output = Path(directory) / "native-annotations.ifc"
                model.write(output)
                reopened = self.ifcopenshell.open(output)
            for annotation in annotations:
                self.assertIsNotNone(reopened.by_guid(annotation.GlobalId))
        finally:
            FreeCAD.closeDocument(document.Name)

    def test_wall_multimaterial_exports_ordered_layer_usage(self):
        import Arch
        import Part
        import ifcopenshell.util.element

        from nativeifc import ifc_export
        from nativeifc import ifc_tools

        document = FreeCAD.newDocument("IfcLayeredMaterial")
        try:
            finish = Arch.makeMaterial(name="Finish")
            structure = Arch.makeMaterial(name="Masonry")
            assembly = Arch.makeMultiMaterial(name="Exterior wall build-up")
            assembly.Names = ["Outside finish", "Structure"]
            assembly.Materials = [finish, structure]
            assembly.Thicknesses = [20.0, 180.0]

            wall = document.addObject("Part::Feature", "LayeredWall")
            wall.addProperty("App::PropertyString", "IfcType")
            wall.addProperty("App::PropertyLink", "Material")
            wall.IfcType = "Wall"
            wall.Material = assembly
            wall.Shape = Part.makeBox(3000, 200, 2800)
            document.recompute()

            model = ifc_tools.create_ifcfile()
            product = ifc_export.create_product(wall, None, model)
            usage = ifcopenshell.util.element.get_material(product)
            self.assertEqual(usage.is_a(), "IfcMaterialLayerSetUsage")
            layers = usage.ForLayerSet.MaterialLayers
            self.assertEqual([layer.Material.Name for layer in layers], ["Finish", "Masonry"])
            self.assertEqual([layer.Name for layer in layers], ["Outside finish", "Structure"])
            self.assertEqual([layer.LayerThickness for layer in layers], [0.02, 0.18])

            with tempfile.TemporaryDirectory() as directory:
                output = Path(directory) / "layered-material.ifc"
                model.write(output)
                reopened = self.ifcopenshell.open(output)
            reopened_usage = ifcopenshell.util.element.get_material(
                reopened.by_guid(product.GlobalId)
            )
            self.assertEqual(reopened_usage.is_a(), "IfcMaterialLayerSetUsage")
            self.assertEqual(
                [layer.Material.Name for layer in reopened_usage.ForLayerSet.MaterialLayers],
                ["Finish", "Masonry"],
            )
        finally:
            FreeCAD.closeDocument(document.Name)

    def test_explicit_type_reuses_materials_and_properties(self):
        import Arch
        import Part
        import ifcopenshell.util.element

        from nativeifc import ifc_export
        from nativeifc import ifc_tools

        document = FreeCAD.newDocument("IfcReusableType")
        try:
            finish = Arch.makeMaterial(name="Type finish")
            structure = Arch.makeMaterial(name="Type masonry")
            assembly = Arch.makeMultiMaterial(name="Typed wall build-up")
            assembly.Materials = [finish, structure]
            assembly.Thicknesses = [20.0, 180.0]

            wall_type = document.addObject("App::FeaturePython", "ExteriorWallType")
            wall_type.Label = "Exterior wall 200 mm"
            wall_type.addProperty("App::PropertyString", "IfcClass")
            wall_type.addProperty("App::PropertyLink", "Material")
            wall_type.addProperty(
                "App::PropertyString",
                "FireRating",
                "Pset_WallCommon",
                "IfcLabel:FireRating",
            )
            wall_type.IfcClass = "IfcWallType"
            wall_type.Material = assembly
            wall_type.FireRating = "2HR"

            walls = []
            model = ifc_tools.create_ifcfile()
            for index, length in enumerate((3000, 5000)):
                wall = document.addObject("Part::Feature", f"TypedWall{index}")
                wall.addProperty("App::PropertyString", "IfcType")
                wall.addProperty("App::PropertyLink", "Type")
                wall.addProperty("App::PropertyLength", "Length")
                wall.addProperty("App::PropertyLength", "Width")
                wall.addProperty("App::PropertyLength", "Height")
                wall.addProperty("App::PropertyLink", "Base")
                wall.addProperty("App::PropertyLinkList", "Additions")
                wall.addProperty("App::PropertyLinkList", "Subtractions")
                wall.IfcType = "Wall"
                wall.Type = wall_type
                wall.Length = length
                wall.Width = 200
                wall.Height = 2800
                wall.Shape = Part.makeBox(length, 200, 2800)
                if index:
                    wall.addProperty(
                        "App::PropertyString",
                        "FireRating",
                        "Pset_WallCommon",
                        "IfcLabel:FireRating",
                    )
                    wall.FireRating = "3HR"
                walls.append(ifc_export.create_product(wall, None, model))

            self.assertEqual(len(model.by_type("IfcWallType")), 1)
            exported_type = model.by_type("IfcWallType")[0]
            self.assertEqual(len(exported_type.Types[0].RelatedObjects), 2)
            type_psets = ifcopenshell.util.element.get_psets(exported_type)
            self.assertEqual(type_psets["Pset_WallCommon"]["FireRating"], "2HR")
            self.assertEqual(
                ifcopenshell.util.element.get_psets(walls[0])["Pset_WallCommon"]["FireRating"],
                "2HR",
            )
            self.assertEqual(
                ifcopenshell.util.element.get_psets(walls[1])["Pset_WallCommon"]["FireRating"],
                "3HR",
            )
            type_material = ifcopenshell.util.element.get_material(exported_type)
            self.assertEqual(type_material.is_a(), "IfcMaterialLayerSet")
            for wall in walls:
                self.assertEqual(
                    ifcopenshell.util.element.get_material(wall).is_a(),
                    "IfcMaterialLayerSetUsage",
                )

            with tempfile.TemporaryDirectory() as directory:
                output = Path(directory) / "reusable-type.ifc"
                model.write(output)
                reopened = self.ifcopenshell.open(output)
            self.assertEqual(len(reopened.by_type("IfcWallType")), 1)
            self.assertEqual(len(reopened.by_type("IfcRelDefinesByType")), 1)
        finally:
            FreeCAD.closeDocument(document.Name)

    def test_mixed_line_arc_profile_preserves_ifc_arcs(self):
        import Part
        import ifcopenshell.geom

        from nativeifc import ifc_geometry_export
        from nativeifc import ifc_tools

        document = FreeCAD.newDocument("IfcMixedCurveProfile")
        try:
            vector = FreeCAD.Vector

            def d_wire(radius):
                bottom = vector(0, -radius, 0)
                top = vector(0, radius, 0)
                line = Part.makeLine(bottom, top)
                arc = Part.Arc(top, vector(radius, 0, 0), bottom).toShape()
                return Part.Wire([line, arc])

            member = document.addObject("Part::Feature", "CurvedMember")
            member.addProperty("App::PropertyString", "IfcType")
            member.IfcType = "Member"
            member.Shape = Part.Face([d_wire(1000), d_wire(400)]).extrude(vector(0, 0, 500))
            member.Placement.Base = vector(100, 200, 300)
            document.recompute()

            model = ifc_tools.create_ifcfile()
            context = next(
                item
                for item in model.by_type("IfcGeometricRepresentationSubContext")
                if item.ContextIdentifier == "Body"
            )
            exporter = ifc_geometry_export.GeometryExporter(model)
            with patch(
                "importers.exportIFC.getRepresentation",
                side_effect=AssertionError("legacy geometry fallback was used"),
            ):
                representation, placement = exporter.create_representation(context, member, {})

            profile = representation.Representations[0].Items[0].SweptArea
            self.assertEqual(profile.is_a(), "IfcArbitraryProfileDefWithVoids")
            curves = [profile.OuterCurve, *profile.InnerCurves]
            for curve in curves:
                self.assertEqual(curve.is_a(), "IfcIndexedPolyCurve")
                self.assertEqual(
                    [segment.is_a() for segment in curve.Segments],
                    ["IfcLineIndex", "IfcArcIndex"],
                )

            product = model.createIfcMember(
                self.ifcopenshell.guid.new(),
                None,
                "Direct Curved Member",
                None,
                None,
                placement,
                representation,
                None,
                None,
            )
            settings = ifcopenshell.geom.settings()
            settings.set("use-world-coords", True)
            shape = ifcopenshell.geom.create_shape(settings, product)
            vertices = list(zip(*(iter(shape.geometry.verts),) * 3))
            bounds = [(min(axis), max(axis)) for axis in zip(*vertices)]
            expected = [(0.1, 1.1), (-0.8, 1.2), (0.3, 0.8)]
            for actual, target in zip(bounds, expected):
                self.assertAlmostEqual(actual[0], target[0], delta=0.001)
                self.assertAlmostEqual(actual[1], target[1], delta=0.001)

            with tempfile.TemporaryDirectory() as directory:
                output = Path(directory) / "mixed-curve.ifc"
                model.write(output)
                reopened = self.ifcopenshell.open(output)
            reopened_profile = (
                reopened.by_guid(product.GlobalId)
                .Representation.Representations[0]
                .Items[0]
                .SweptArea
            )
            self.assertEqual(reopened_profile.OuterCurve.is_a(), "IfcIndexedPolyCurve")
        finally:
            FreeCAD.closeDocument(document.Name)

    def test_arbitrary_multisolid_uses_native_faceted_brep(self):
        import Part
        import ifcopenshell.geom

        from nativeifc import ifc_geometry_export
        from nativeifc import ifc_tools

        document = FreeCAD.newDocument("IfcNativeFacetedBrep")
        try:
            vector = FreeCAD.Vector
            curved_boolean = Part.makeSphere(500).cut(
                Part.makeBox(500, 500, 1000, vector(0, -250, -500))
            )
            second_solid = Part.makeSphere(250, vector(1500, 0, 0))
            feature = document.addObject("Part::Feature", "ArbitrarySolids")
            feature.addProperty("App::PropertyString", "IfcType")
            feature.addProperty("App::PropertyColor", "ShapeColor")
            feature.addProperty("App::PropertyColorList", "DiffuseColor")
            feature.IfcType = "BuildingElementProxy"
            feature.Shape = Part.makeCompound([curved_boolean, second_solid])
            feature.ShapeColor = (0.5, 0.5, 0.5)
            feature.DiffuseColor = [
                color
                for solid, color in zip(
                    feature.Shape.Solids,
                    ((0.8, 0.1, 0.1), (0.1, 0.8, 0.1)),
                )
                for _face in solid.Faces
            ]
            feature.Placement.Base = vector(100, 200, 300)
            document.recompute()

            model = ifc_tools.create_ifcfile()
            context = next(
                item
                for item in model.by_type("IfcGeometricRepresentationSubContext")
                if item.ContextIdentifier == "Body"
            )
            exporter = ifc_geometry_export.GeometryExporter(model)
            with patch(
                "importers.exportIFC.getRepresentation",
                side_effect=AssertionError("legacy geometry fallback was used"),
            ):
                representation, placement = exporter.create_representation(context, feature, {})

            shape_representation = representation.Representations[0]
            self.assertEqual(shape_representation.RepresentationType, "Brep")
            self.assertEqual(len(shape_representation.Items), 2)
            self.assertTrue(
                all(item.is_a() == "IfcFacetedBrep" for item in shape_representation.Items)
            )
            item_styles = [item.StyledByItem[0].Styles[0] for item in shape_representation.Items]
            self.assertNotEqual(item_styles[0], item_styles[1])

            product = model.createIfcBuildingElementProxy(
                self.ifcopenshell.guid.new(),
                None,
                "Native Faceted BRep",
                None,
                None,
                placement,
                representation,
                None,
                None,
            )
            settings = ifcopenshell.geom.settings()
            settings.set("use-world-coords", True)
            reconstructed = ifcopenshell.geom.create_shape(settings, product)
            vertices = list(zip(*(iter(reconstructed.geometry.verts),) * 3))
            actual = [(min(axis), max(axis)) for axis in zip(*vertices)]
            bounds = feature.Shape.BoundBox
            expected = [
                (bounds.XMin * 0.001, bounds.XMax * 0.001),
                (bounds.YMin * 0.001, bounds.YMax * 0.001),
                (bounds.ZMin * 0.001, bounds.ZMax * 0.001),
            ]
            for reconstructed_bounds, source_bounds in zip(actual, expected):
                self.assertAlmostEqual(reconstructed_bounds[0], source_bounds[0], delta=0.002)
                self.assertAlmostEqual(reconstructed_bounds[1], source_bounds[1], delta=0.002)

            with tempfile.TemporaryDirectory() as directory:
                output = Path(directory) / "native-faceted-brep.ifc"
                model.write(output)
                reopened = self.ifcopenshell.open(output)
            reopened_representation = reopened.by_guid(product.GlobalId).Representation
            self.assertEqual(len(reopened_representation.Representations[0].Items), 2)
        finally:
            FreeCAD.closeDocument(document.Name)

    def test_mixed_compound_and_mesh_use_native_tessellation(self):
        import Mesh
        import Part

        from nativeifc import ifc_geometry_export
        from nativeifc import ifc_tools

        document = FreeCAD.newDocument("IfcNativeTessellation")
        try:
            box = Part.makeBox(1000, 800, 600)
            wire = Part.makePolygon(
                [
                    FreeCAD.Vector(0, 0, 900),
                    FreeCAD.Vector(1000, 0, 900),
                    FreeCAD.Vector(0, 800, 900),
                    FreeCAD.Vector(0, 0, 900),
                ]
            )
            mixed = document.addObject("Part::Feature", "MixedCompound")
            mixed.Shape = Part.makeCompound([box, Part.Face(wire)])

            mesh = document.addObject("Mesh::Feature", "TriangleMesh")
            mesh.Mesh = Mesh.Mesh(
                [
                    (
                        FreeCAD.Vector(0, 0, 0),
                        FreeCAD.Vector(500, 0, 0),
                        FreeCAD.Vector(0, 500, 250),
                    )
                ]
            )
            mesh.Placement.Base = FreeCAD.Vector(2000, 0, 0)
            document.recompute()

            model = ifc_tools.create_ifcfile()
            context = next(
                item
                for item in model.by_type("IfcGeometricRepresentationSubContext")
                if item.ContextIdentifier == "Body"
            )
            exporter = ifc_geometry_export.GeometryExporter(model)
            mixed_representation, mixed_placement = exporter.create_representation(
                context, mixed, {}
            )
            mesh_representation, mesh_placement = exporter.create_representation(context, mesh, {})

            for representation in (mixed_representation, mesh_representation):
                shape_representation = representation.Representations[0]
                self.assertEqual(shape_representation.RepresentationType, "Tessellation")
                self.assertEqual(shape_representation.Items[0].is_a(), "IfcTriangulatedFaceSet")

            products = [
                model.createIfcBuildingElementProxy(
                    self.ifcopenshell.guid.new(),
                    None,
                    name,
                    None,
                    None,
                    placement,
                    representation,
                    None,
                    None,
                )
                for name, placement, representation in (
                    ("Mixed compound", mixed_placement, mixed_representation),
                    ("Triangle mesh", mesh_placement, mesh_representation),
                )
            ]
            with tempfile.TemporaryDirectory() as directory:
                output = Path(directory) / "native-tessellation.ifc"
                model.write(output)
                reopened = self.ifcopenshell.open(output)
            for product in products:
                item = reopened.by_guid(product.GlobalId).Representation.Representations[0].Items[0]
                self.assertEqual(item.is_a(), "IfcTriangulatedFaceSet")

            ifc2x3 = self.ifcopenshell.file(schema="IFC2X3")
            origin = ifc2x3.createIfcCartesianPoint((0.0, 0.0, 0.0))
            axis = ifc2x3.createIfcAxis2Placement3D(origin, None, None)
            ifc2x3_context = ifc2x3.createIfcGeometricRepresentationContext(
                None, "Model", 3, 1e-5, axis, None
            )
            ifc2x3_representation, _placement = ifc_geometry_export.GeometryExporter(
                ifc2x3
            ).create_representation(ifc2x3_context, mixed, {})
            ifc2x3_shape = ifc2x3_representation.Representations[0]
            self.assertEqual(ifc2x3_shape.RepresentationType, "SurfaceModel")
            self.assertEqual(ifc2x3_shape.Items[0].is_a(), "IfcFaceBasedSurfaceModel")
        finally:
            FreeCAD.closeDocument(document.Name)

    def test_direct_geometry_styles_are_reused_and_persisted(self):
        import Part

        from nativeifc import ifc_geometry_export
        from nativeifc import ifc_tools

        document = FreeCAD.newDocument("IfcDirectStyles")
        try:
            objects = []
            for index in range(2):
                slab = document.addObject("Part::Feature", f"StyledSlab{index}")
                slab.addProperty("App::PropertyString", "IfcType")
                slab.addProperty("App::PropertyColor", "ShapeColor")
                slab.addProperty("App::PropertyInteger", "Transparency")
                slab.IfcType = "Slab"
                slab.ShapeColor = (0.2, 0.4, 0.6)
                slab.Transparency = 25
                slab.Shape = Part.makeBox(1000, 1000, 200)
                slab.Placement.Base.x = index * 1500
                objects.append(slab)
            document.recompute()

            model = ifc_tools.create_ifcfile()
            context = next(
                item
                for item in model.by_type("IfcGeometricRepresentationSubContext")
                if item.ContextIdentifier == "Body"
            )
            exporter = ifc_geometry_export.GeometryExporter(model)
            representations = [
                exporter.create_representation(context, obj, {})[0] for obj in objects
            ]

            self.assertEqual(len(model.by_type("IfcSurfaceStyle")), 1)
            style = model.by_type("IfcSurfaceStyle")[0]
            shading = style.Styles[0]
            self.assertAlmostEqual(shading.SurfaceColour.Red, 0.2)
            self.assertAlmostEqual(shading.SurfaceColour.Green, 0.4)
            self.assertAlmostEqual(shading.SurfaceColour.Blue, 0.6)
            self.assertAlmostEqual(shading.Transparency, 0.25)
            for representation in representations:
                item = representation.Representations[0].Items[0]
                self.assertEqual(item.StyledByItem[0].Styles[0], style)

            with tempfile.TemporaryDirectory() as directory:
                output = Path(directory) / "direct-styles.ifc"
                model.write(output)
                reopened = self.ifcopenshell.open(output)
            reopened_style = reopened.by_type("IfcSurfaceStyle")[0]
            self.assertAlmostEqual(reopened_style.Styles[0].Transparency, 0.25)
        finally:
            FreeCAD.closeDocument(document.Name)


if __name__ == "__main__":
    unittest.main()
