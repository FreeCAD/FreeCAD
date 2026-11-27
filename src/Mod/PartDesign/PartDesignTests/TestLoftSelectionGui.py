# SPDX-License-Identifier: LGPL-2.1-or-later
"""Regression coverage for loft preselection grouped by owning object."""

import unittest
import FreeCAD as App
import FreeCADGui as Gui
import Part
import PartDesign
import Sketcher


class TestLoftSelectionGui(unittest.TestCase):
    def setUp(self):
        Gui.activateWorkbench("PartDesignWorkbench")
        self.doc = App.newDocument("LoftSelectionGui")
        self.body = self.doc.addObject("PartDesign::Body", "Body")
        Gui.activeDocument().activeView().setActiveObject("pdbody", self.body)
        Gui.Selection.clearSelection()

    def tearDown(self):
        Gui.Selection.clearSelection()
        if Gui.getDocument(self.doc.Name).getInEdit():
            Gui.getDocument(self.doc.Name).resetEdit()
        if Gui.Control.activeDialog():
            Gui.Control.closeDialog()
        App.closeDocument(self.doc.Name)

    def makeBox(self, z=0, height=10):
        box = self.body.newObject("PartDesign::Feature", "Box")
        box.Shape = Part.makeBox(2, 3, height, App.Vector(0, 0, z))
        self.doc.recompute()
        faces = sorted(
            (face.CenterOfMass.z, "Face%d" % (index + 1))
            for index, face in enumerate(box.Shape.Faces)
            if abs(face.normalAt(0, 0).z) > 0.99
        )
        self.assertEqual(len(faces), 2)
        return box, [name for z, name in faces]

    def selectedLoft(self, selections):
        for obj, subname in selections:
            Gui.Selection.addSelection(self.doc.Name, obj.Name, subname)
        Gui.runCommand("PartDesign_AdditiveLoft", 0)
        Gui.updateGui()
        self.doc.recompute()
        lofts = [obj for obj in self.body.Group if obj.TypeId == "PartDesign::AdditiveLoft"]
        self.assertEqual(len(lofts), 1)
        return lofts[0]

    def assertValidLoft(self, loft, volume):
        self.assertFalse(loft.Shape.isNull(), loft.State)
        self.assertTrue(loft.Shape.isValid())
        self.assertAlmostEqual(loft.Shape.Volume, volume)

    def testExplicitFaceProfileAndSectionControl(self):
        box, faces = self.makeBox()
        loft = self.body.newObject("PartDesign::AdditiveLoft", "Loft")
        loft.Profile = (box, [faces[0]])
        loft.Sections = [(box, [faces[1]])]
        self.doc.recompute()
        self.assertValidLoft(loft, 60)

    def checkFacesOfSameObject(self, reverse):
        box, faces = self.makeBox()
        if reverse:
            faces.reverse()
        loft = self.selectedLoft([(box, name) for name in faces])
        # The object-grouped selection must become separate loft sections.
        self.assertEqual(loft.Profile, (box, [faces[0]]))
        self.assertEqual(len(loft.Sections), 1)
        self.assertEqual(loft.Sections[0][0], box)
        self.assertEqual(tuple(loft.Sections[0][1]), (faces[1],))
        self.assertValidLoft(loft, 60)

    def testFacesOfSameObjectBecomeProfileAndSection(self):
        # https://github.com/FreeCAD/FreeCAD/issues/32016
        self.checkFacesOfSameObject(reverse=False)

    def testFacesOfSameObjectKeepSelectionOrder(self):
        self.checkFacesOfSameObject(reverse=True)

    def testCoplanarFacesOfSameObjectStayOneProfile(self):
        # A slot splits the top of the box into two coplanar faces: a multi-face profile.
        slotted = self.body.newObject("PartDesign::Feature", "Slotted")
        slotted.Shape = Part.makeBox(4, 3, 10).cut(Part.makeBox(2, 3, 2, App.Vector(1, 0, 8)))
        self.doc.recompute()
        faces = [
            "Face%d" % (index + 1)
            for index, face in enumerate(slotted.Shape.Faces)
            if face.normalAt(0, 0).z > 0.99 and abs(face.CenterOfMass.z - 10) < 1e-6
        ]
        self.assertEqual(len(faces), 2)
        loft = self.selectedLoft([(slotted, name) for name in faces])
        self.assertEqual(loft.Profile, (slotted, faces))
        self.assertEqual(len(loft.Sections), 0)

    def testFacesOfDifferentObjectsKeepExistingSections(self):
        bottom, bottom_faces = self.makeBox(z=0, height=1)
        top, top_faces = self.makeBox(z=9, height=1)
        loft = self.selectedLoft([(bottom, bottom_faces[0]), (top, top_faces[1])])
        self.assertEqual(loft.Profile, (bottom, [bottom_faces[0]]))
        self.assertEqual(len(loft.Sections), 1)
        self.assertEqual(loft.Sections[0][0], top)
        self.assertEqual(tuple(loft.Sections[0][1]), (top_faces[1],))
        self.assertValidLoft(loft, 60)

    def testWholeSketchSelectionKeepsWholeProfiles(self):
        sketches = []
        for z in (0, 10):
            sketch = self.body.newObject("Sketcher::SketchObject", "Sketch")
            sketch.Placement = App.Placement(App.Vector(0, 0, z), App.Rotation())
            points = [App.Vector(x, y, 0) for x, y in ((0, 0), (2, 0), (2, 3), (0, 3))]
            sketch.addGeometry(
                [Part.LineSegment(points[index], points[(index + 1) % 4]) for index in range(4)],
                False,
            )
            sketches.append(sketch)
        self.doc.recompute()
        loft = self.selectedLoft([(sketch, "") for sketch in sketches])
        self.assertEqual(loft.Profile, (sketches[0], []))
        self.assertEqual(len(loft.Sections), 1)
        self.assertEqual(loft.Sections[0][0], sketches[1])
        self.assertEqual(tuple(loft.Sections[0][1]), ("",))
        self.assertValidLoft(loft, 60)
