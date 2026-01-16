# test for FreeCAD issue https://github.com/freecad/freecad/issues/25893
# regression test for sketch placement updates during edit mode
# when sketcher workbench is active and has not be closed / exited

import unittest
import FreeCAD

# check if GUI is available
try:
    import FreeCADGui

    GUI_AVAILABLE = FreeCADGui.getMainWindow() is not None
except (ImportError, AttributeError):
    GUI_AVAILABLE = False

from FreeCAD import Base


class TestSketchPlacementUpdate(unittest.TestCase):
    """
    test that sketch placement/attachment changes update the 3D view
    when the sketch is in edit mode.

    regression test for issue #25893 caused by PR #25478

    NOTE: issue #25893 specifically affects ATTACHED sketches where
    AttachmentOffset changes don't update the visual position during
    edit mode. These tests use an attached sketch to properly test
    this scenario.

    These tests require GUI and will be skipped with freecadcmd.
    """

    def setUp(self):
        """create a document with a body, and within the body create a cylinder,
        then attach a sketch to the cylinder to the bottom round face of the
        cylinder.
        """
        if not GUI_AVAILABLE:
            self.skipTest("GUI not available")

        self.doc = FreeCAD.newDocument("TestPlacementUpdate")

        # create a body (typical partdesign workflow)
        self.body = self.doc.addObject("PartDesign::Body", "Body")

        # create a cylinder to attach the sketch to (10mm tall)
        self.cylinder = self.doc.addObject("PartDesign::AdditiveCylinder", "Cylinder")
        self.cylinder.Height = 10.0
        self.cylinder.Radius = 2.0
        self.body.addObject(self.cylinder)
        self.doc.recompute()

        # create a sketch and attach it to the bottom face of the cylinder
        self.sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        self.body.addObject(self.sketch)

        # attach the sketch to Face2, ie. bottom circle face of the cylinder
        self.sketch.AttachmentSupport = (self.cylinder, ["Face2"])
        self.sketch.MapMode = "FlatFace"
        self.sketch.MapReversed = True

        # add simple geometry so sketch is valid
        import Part

        # create 2 diagonal lines at the base of the cylinder
        self.sketch.addGeometry(
            Part.LineSegment(Base.Vector(-3, -3, 0), Base.Vector(3, 3, 0)), False
        )
        self.sketch.addGeometry(
            Part.LineSegment(Base.Vector(3, -3, 0), Base.Vector(-3, 3, 0)), False
        )

        self.doc.recompute()

    def tearDown(self):
        """clean up the test document"""
        if GUI_AVAILABLE:
            FreeCAD.closeDocument(self.doc.Name)

    @unittest.skipIf(not GUI_AVAILABLE, "GUI not available")
    def test_attachment_offset_updates_in_edit_mode(self):
        """
        test that changing AttachmentOffset while editing updates the transform.
        this is the main regression test for issue #25893.

        For attached sketches, AttachmentOffset controls the visual position
        relative to the attachment face.
        """
        # enter edit mode
        FreeCADGui.ActiveDocument.setEdit(self.sketch.Name)

        # get initial editing transform (it's a property, not a method)
        initial_transform = FreeCADGui.ActiveDocument.EditingTransform
        self.assertIsNotNone(initial_transform, "Editing transform should exist")

        # change the attachment offset (move 5mm in Z direction)
        # for attached sketches, this should update the visual position
        new_offset = Base.Placement(Base.Vector(0, 0, 5), Base.Rotation())
        self.sketch.AttachmentOffset = new_offset
        self.doc.recompute()

        # get updated editing transform
        updated_transform = FreeCADGui.ActiveDocument.EditingTransform

        # verify the transform was updated
        self.assertNotEqual(
            initial_transform,
            updated_transform,
            "Editing transform should update when AttachmentOffset changes",
        )

        # exit edit mode
        FreeCADGui.ActiveDocument.resetEdit()

    @unittest.skipIf(not GUI_AVAILABLE, "GUI not available")
    def test_multiple_attachment_offset_updates(self):
        """
        test that multiple AttachmentOffset changes in edit mode all update correctly.
        """
        # enter edit mode
        FreeCADGui.ActiveDocument.setEdit(self.sketch.Name)

        transforms = []

        # make several changes and verify each updates the transform
        for i in range(1, 4):
            z_offset = i * 2.0  # 2mm, 4mm, 6mm
            self.sketch.AttachmentOffset = Base.Placement(
                Base.Vector(0, 0, z_offset), Base.Rotation()
            )
            self.doc.recompute()

            transform = FreeCADGui.ActiveDocument.EditingTransform
            transforms.append(transform)

            # verify this transform is different from previous
            if i > 1:
                self.assertNotEqual(
                    transforms[-1],
                    transforms[-2],
                    f"Transform {i} should differ from transform {i-1}",
                )

        # exit edit mode
        FreeCADGui.ActiveDocument.resetEdit()

    @unittest.skipIf(not GUI_AVAILABLE, "GUI not available")
    def test_no_update_when_not_editing(self):
        """
        verify that attachment offset changes don't cause issues when sketch is not in edit mode.
        this ensures the fix only acts during edit mode.
        """
        # change attachment offset while NOT editing
        new_offset = Base.Placement(Base.Vector(0, 0, 8), Base.Rotation())
        self.sketch.AttachmentOffset = new_offset
        self.doc.recompute()

        # this should not crash or cause any issues
        # just verify the sketch is still valid and attached
        self.assertTrue(
            self.sketch.isDerivedFrom("Sketcher::SketchObject"), "Sketch should still be valid"
        )
        self.assertEqual(self.sketch.MapMode, "FlatFace", "Sketch should still be attached")

    @unittest.skipIf(not GUI_AVAILABLE, "GUI not available")
    def test_unattached_sketch_placement_updates(self):
        """
        test that unattached sketches also work correctly.
        for unattached sketches, the base Placement property controls position.
        """
        # create an unattached sketch for comparison
        unattached = self.doc.addObject("Sketcher::SketchObject", "UnattachedSketch")
        self.body.addObject(unattached)

        import Part

        unattached.addGeometry(Part.Circle(Base.Vector(0, 0, 0), Base.Vector(0, 0, 1), 2.0), False)
        self.doc.recompute()

        # enter edit mode
        FreeCADGui.ActiveDocument.setEdit(unattached.Name)

        initial_transform = FreeCADGui.ActiveDocument.EditingTransform

        # for unattached sketches, change Placement (not AttachmentOffset)
        unattached.Placement = Base.Placement(Base.Vector(5, 5, 5), Base.Rotation())
        self.doc.recompute()

        updated_transform = FreeCADGui.ActiveDocument.EditingTransform

        # verify transform updated
        self.assertNotEqual(
            initial_transform,
            updated_transform,
            "Editing transform should update when Placement changes for unattached sketch",
        )

        # exit edit mode
        FreeCADGui.ActiveDocument.resetEdit()
