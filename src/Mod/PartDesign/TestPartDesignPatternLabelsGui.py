# SPDX-License-Identifier: LGPL-2.1-or-later
"""Regression coverage for pattern labels in the edited occurrence's frame."""

import math
import unittest

import FreeCAD as App
import FreeCADGui as Gui
import PartDesign
from pivy import coin
from PySide import QtGui


def worldPoint(matrix, point):
    rows = matrix
    values = tuple(point.getValue())
    return App.Vector(*(sum(values[index] * rows[index][component] for index in range(3))
                        + rows[3][component] for component in range(3)))


def datumLabels(datum_type):
    root = Gui.activeDocument().activeView().getViewer().getSoRenderManager().getSceneGraph()
    search = coin.SoSearchAction()
    search.setType(coin.SoType.fromName("SoDatumLabel"))
    search.setInterest(coin.SoSearchAction.ALL)
    search.setSearchingAll(True)
    search.apply(root)
    found = []
    for path in search.getPaths():
        label = path.getTail()
        if label.getField("datumtype").getValue() != datum_type:
            continue
        action = coin.SoGetMatrixAction(coin.SbViewportRegion(800, 600))
        action.apply(path)
        # Copy the values while the action that owns the matrix is still alive.
        rows = tuple(tuple(row) for row in action.getMatrix().getValue())
        found.append((label, rows))
    return found


class TestPartDesignPatternLabelsGui(unittest.TestCase):
    def setUp(self):
        Gui.activateWorkbench("PartDesignWorkbench")
        self.doc = App.newDocument("PatternLabelFramesGui")
        Gui.Selection.clearSelection()

    def tearDown(self):
        Gui.Selection.clearSelection()
        if Gui.getDocument(self.doc.Name).getInEdit():
            Gui.getDocument(self.doc.Name).resetEdit()
        if Gui.Control.activeDialog():
            Gui.Control.closeDialog()
        App.closeDocument(self.doc.Name)

    def makeBox(self, body):
        box = body.newObject("PartDesign::AdditiveBox", "Box")
        box.Length, box.Width, box.Height = 2, 3, 10
        self.doc.recompute()
        return box

    def makeLinearPattern(self, body, box):
        pattern = body.newObject("PartDesign::LinearPattern", "Pattern")
        pattern.Originals = [box]
        pattern.Direction = (self.doc.X_Axis, [""])
        pattern.Occurrences = 3
        pattern.Length = 4
        pattern.Occurrences2 = 1
        self.doc.recompute()
        self.assertTrue(pattern.Shape.isValid(), pattern.State)
        self.assertAlmostEqual(pattern.Shape.Volume, 180)
        return pattern

    def assertDistanceLabel(self, expected_start, expected_vector):
        labels = datumLabels(1)  # SoDatumLabel::DISTANCE
        self.assertEqual(len(labels), 1)
        label, matrix = labels[0]
        points = label.getField("pnts")
        self.assertEqual(points.getNum(), 2)
        start = worldPoint(matrix, points[0])
        end = worldPoint(matrix, points[1])
        self.assertLess((end - start - expected_vector).Length, 1e-5)
        self.assertLess((start - expected_start).Length, 1e-5)

    def checkLinearFrame(self, body_placed=False, feature_placed=False, nested=False, linked=False):
        body = self.doc.addObject("PartDesign::Body", "Body")
        root = body
        if nested or linked:
            part = self.doc.addObject("App::Part", "Part")
            part.addObject(body)
            part.Placement = App.Placement(App.Vector(20, -30, 40),
                                           App.Rotation(App.Vector(0, 1, 0), 25))
            root = part
        if body_placed:
            body.Placement = App.Placement(App.Vector(100, 200, 30),
                                           App.Rotation(App.Vector(0, 0, 1), 90))
        box = self.makeBox(body)
        if feature_placed:
            # X rotation leaves the pattern's X direction and box length aligned.
            box.Placement = App.Placement(App.Vector(5, 7, 11),
                                          App.Rotation(App.Vector(1, 0, 0), 30))
        pattern = self.makeLinearPattern(body, box)
        path = pattern.Name + "."
        if nested or linked:
            path = body.Name + "." + path
        if linked:
            link = self.doc.addObject("App::Link", "Instance")
            link.LinkedObject = root
            link.LinkTransform = True
            link.LinkPlacement = App.Placement(App.Vector(-50, 60, 80),
                                               App.Rotation(App.Vector(1, 0, 0), 50))
            root = link
            self.doc.recompute()
        # Obtain the selected model occurrence independently of the GUI labels.
        model_matrix = root.getSubObject(path, retType=4)
        self.assertIsNotNone(model_matrix)
        Gui.activeDocument().activeView().setActiveObject("pdbody", body)
        self.assertTrue(Gui.getDocument(self.doc.Name).setEdit(root.Name, 0, path))
        Gui.updateGui()
        edit_matrix = Gui.getDocument(self.doc.Name).EditingTransform
        for basis in (App.Vector(), App.Vector(1, 0, 0), App.Vector(0, 1, 0), App.Vector(0, 0, 1)):
            self.assertLess((model_matrix.multVec(basis) - edit_matrix.multVec(basis)).Length, 1e-6,
                            "Editing transform must match the selected model occurrence")
        # The source center is in Body coordinates. Remove the pattern's local
        # placement before applying its full selected-occurrence matrix.
        local_start = pattern.Placement.inverse().multVec(box.Shape.CenterOfMass)
        local_vector = pattern.Placement.Rotation.inverted().multVec(App.Vector(4, 0, 0))
        expected_start = model_matrix.multVec(local_start)
        expected_vector = model_matrix.multVec(local_vector) - model_matrix.multVec(App.Vector())
        self.assertDistanceLabel(expected_start, expected_vector)

    def testIdentityPlacementControl(self):
        self.checkLinearFrame()

    def testNonidentityFeaturePlacementControl(self):
        self.checkLinearFrame(feature_placed=True)

    def testRotatedAndTranslatedBody(self):
        # https://github.com/FreeCAD/FreeCAD/issues/33563
        self.checkLinearFrame(body_placed=True)

    def testNestedPartBodyAndFeature(self):
        self.checkLinearFrame(body_placed=True, feature_placed=True, nested=True)

    def testEditedLinkOccurrence(self):
        self.checkLinearFrame(body_placed=True, feature_placed=True, linked=True)

    def testPolarPatternAngleLabelControl(self):
        body = self.doc.addObject("PartDesign::Body", "Body")
        box = self.makeBox(body)
        pattern = body.newObject("PartDesign::PolarPattern", "PolarPattern")
        pattern.Originals = [box]
        pattern.Axis = (self.doc.Z_Axis, [""])
        pattern.Angle = 90
        pattern.Occurrences = 2
        self.doc.recompute()
        self.assertTrue(pattern.Shape.isValid(), pattern.State)
        self.assertAlmostEqual(pattern.Shape.Volume, 120)
        Gui.activeDocument().activeView().setActiveObject("pdbody", body)
        self.assertTrue(Gui.getDocument(self.doc.Name).setEdit(pattern.Name))
        Gui.updateGui()
        labels = datumLabels(0)  # SoDatumLabel::ANGLE
        self.assertEqual(len(labels), 1)
        label, matrix = labels[0]
        # The angle range is not an exposed Coin field. Read the displayed
        # quantity and the exposed initial angle instead.
        angle = App.Units.parseQuantity(str(label.getField("string")[0])).getValueAs("deg")
        self.assertAlmostEqual(angle.Value, 90)
        center = box.Shape.CenterOfMass
        self.assertAlmostEqual(label.getField("param2").getValue(),
                               math.atan2(center.y, center.x), places=6)
        origin = worldPoint(matrix, coin.SbVec3f(0, 0, 0))
        self.assertLess(origin.Length, 1e-5)
        axis = worldPoint(matrix, coin.SbVec3f(0, 0, 1)) - origin
        self.assertLess((axis - App.Vector(0, 0, 1)).Length, 1e-5)

    def testLinearPatternInsideMultiTransformControl(self):
        body = self.doc.addObject("PartDesign::Body", "Body")
        box = self.makeBox(body)
        pattern = self.makeLinearPattern(body, box)
        multi = body.newObject("PartDesign::MultiTransform", "MultiTransform")
        multi.Originals = [box]
        multi.Transformations = [pattern]
        self.doc.recompute()
        self.assertTrue(multi.Shape.isValid(), multi.State)
        self.assertAlmostEqual(multi.Shape.Volume, 180)
        Gui.activeDocument().activeView().setActiveObject("pdbody", body)
        self.assertTrue(Gui.getDocument(self.doc.Name).setEdit(multi.Name))
        Gui.updateGui()
        transformations = Gui.getMainWindow().findChild(QtGui.QListWidget, "listTransformFeatures")
        self.assertIsNotNone(transformations)
        self.assertEqual(transformations.count(), 1)
        transformations.setCurrentRow(0)
        transformations.activated.emit(transformations.model().index(0, 0))
        Gui.updateGui()
        self.assertDistanceLabel(box.Shape.CenterOfMass, App.Vector(4, 0, 0))
