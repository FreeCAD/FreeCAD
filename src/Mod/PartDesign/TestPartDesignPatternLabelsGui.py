# SPDX-License-Identifier: LGPL-2.1-or-later
"""Regression coverage for pattern labels in the edited occurrence's frame."""

import math
import unittest

import FreeCAD as App
import FreeCADGui as Gui
from pivy import coin
from PySide import QtGui


def worldPoint(matrix, point):
    rows = matrix
    values = tuple(point.getValue())
    return App.Vector(
        *(
            sum(values[index] * rows[index][component] for index in range(3)) + rows[3][component]
            for component in range(3)
        )
    )


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
            part.Placement = App.Placement(
                App.Vector(20, -30, 40), App.Rotation(App.Vector(0, 1, 0), 25)
            )
            root = part
        if body_placed:
            body.Placement = App.Placement(
                App.Vector(100, 200, 30), App.Rotation(App.Vector(0, 0, 1), 90)
            )
        box = self.makeBox(body)
        if feature_placed:
            # X rotation leaves the pattern's X direction and box length aligned.
            box.Placement = App.Placement(
                App.Vector(5, 7, 11), App.Rotation(App.Vector(1, 0, 0), 30)
            )
        pattern = self.makeLinearPattern(body, box)
        path = pattern.Name + "."
        if nested or linked:
            path = body.Name + "." + path
        if linked:
            link = self.doc.addObject("App::Link", "Instance")
            link.LinkedObject = root
            link.LinkTransform = True
            link.LinkPlacement = App.Placement(
                App.Vector(-50, 60, 80), App.Rotation(App.Vector(1, 0, 0), 50)
            )
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
            self.assertLess(
                (model_matrix.multVec(basis) - edit_matrix.multVec(basis)).Length,
                1e-6,
                "Editing transform must match the selected model occurrence",
            )
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

    def checkPolarFrame(
        self,
        body_placed=False,
        feature_placed=False,
        nested=False,
        linked=False,
        tilted=False,
        reversed=False,
        spacing=False,
        multi=False,
    ):
        body = self.doc.addObject("PartDesign::Body", "Body")
        root = body
        if nested or linked:
            part = self.doc.addObject("App::Part", "Part")
            part.addObject(body)
            part.Placement = App.Placement(
                App.Vector(20, -30, 40), App.Rotation(App.Vector(0, 1, 0), 25)
            )
            root = part
        if body_placed:
            rotation_axis = App.Vector(1, 0, 0) if tilted else App.Vector(0, 0, 1)
            body.Placement = App.Placement(
                App.Vector(100, 200, 30), App.Rotation(rotation_axis, 90)
            )
        box = self.makeBox(body)
        if feature_placed:
            box.Placement = App.Placement(
                App.Vector(5, 7, 11), App.Rotation(App.Vector(1, 0, 0), 30)
            )
        pattern = body.newObject("PartDesign::PolarPattern", "PolarPattern")
        pattern.Originals = [box]
        pattern.Axis = (self.doc.Z_Axis, [""])
        pattern.Angle = 90
        pattern.Occurrences = 2
        pattern.Reversed = reversed
        if spacing:
            pattern.Mode = "Spacing"
            pattern.Occurrences = 3
            pattern.Offset = 30
            pattern.Spacings = [20, -1]
        self.doc.recompute()
        self.assertTrue(pattern.Shape.isValid(), pattern.State)
        edited = pattern
        if multi:
            edited = body.newObject("PartDesign::MultiTransform", "MultiTransform")
            edited.Originals = [box]
            edited.Transformations = [pattern]
            self.doc.recompute()
            self.assertTrue(edited.Shape.isValid(), edited.State)
        path = edited.Name + "."
        if nested or linked:
            path = body.Name + "." + path
        if linked:
            link = self.doc.addObject("App::Link", "Instance")
            link.LinkedObject = root
            link.LinkTransform = True
            link.LinkPlacement = App.Placement(
                App.Vector(-50, 60, 80), App.Rotation(App.Vector(1, 0, 0), 50)
            )
            root = link
            self.doc.recompute()
        # Obtain the occurrence's frame independently of the label's scene graph.
        model_matrix = root.getSubObject(path, retType=4)
        self.assertIsNotNone(model_matrix)
        Gui.activeDocument().activeView().setActiveObject("pdbody", body)
        self.assertTrue(Gui.getDocument(self.doc.Name).setEdit(root.Name, 0, path))
        Gui.updateGui()
        if multi:
            transformations = Gui.getMainWindow().findChild(
                QtGui.QListWidget, "listTransformFeatures"
            )
            self.assertIsNotNone(transformations)
            transformations.setCurrentRow(0)
            transformations.activated.emit(transformations.model().index(0, 0))
            Gui.updateGui()

        # The axis reference and source center are in Body coordinates. The
        # occurrence matrix includes the pattern's own placement, so remove it.
        frame = model_matrix * pattern.Placement.inverse().toMatrix()
        if multi:
            # MultiTransform subtasks retain their existing Body-coordinate path.
            frame = App.Matrix()
        expected_center = frame.multVec(App.Vector())
        expected_start = frame.multVec(box.Shape.CenterOfMass)
        expected_axis = frame.multVec(App.Vector(0, 0, -1 if reversed else 1)) - expected_center
        expected_axis.normalize()
        radius_vector = expected_start - expected_center
        expected_ray = radius_vector - expected_axis * radius_vector.dot(expected_axis)
        expected_ray.normalize()
        labels = datumLabels(0)  # SoDatumLabel::ANGLE
        angles = [20, 30] if spacing else [90]
        self.assertEqual(len(labels), len(angles))
        cumulative_angle = 0
        for (label, matrix), expected_angle in zip(labels, angles):
            # The angle range is not an exposed Coin field. Check the displayed
            # quantity, then reconstruct the starting ray from the label frame.
            angle = App.Units.parseQuantity(str(label.getField("string")[0])).getValueAs("deg")
            self.assertAlmostEqual(angle.Value, expected_angle)
            origin = worldPoint(matrix, coin.SbVec3f(0, 0, 0))
            self.assertLess((origin - expected_center).Length, 1e-5)
            axis = worldPoint(matrix, coin.SbVec3f(0, 0, 1)) - origin
            self.assertLess((axis - expected_axis).Length, 1e-5)
            start_angle = label.getField("param2").getValue()
            ray = worldPoint(matrix, coin.SbVec3f(math.cos(start_angle), math.sin(start_angle), 0))
            expected = App.Rotation(expected_axis, cumulative_angle).multVec(expected_ray)
            self.assertLess((ray - origin - expected).Length, 1e-5)
            radius = label.getField("param1").getValue()
            self.assertTrue(math.isfinite(radius))
            self.assertGreater(radius, 0)
            cumulative_angle += expected_angle

    def testPolarPatternAngleLabelControl(self):
        self.checkPolarFrame()

    def testPolarPatternNonidentityFeaturePlacementControl(self):
        self.checkPolarFrame(feature_placed=True)

    def testPolarPatternRotatedAndTranslatedBody(self):
        # Z rotation leaves the axis unchanged but must rotate the starting ray.
        self.checkPolarFrame(body_placed=True)

    def testPolarPatternTiltedBody(self):
        self.checkPolarFrame(body_placed=True, tilted=True)

    def testPolarPatternNestedPartBodyAndFeature(self):
        self.checkPolarFrame(body_placed=True, feature_placed=True, nested=True)

    def testPolarPatternEditedLinkOccurrence(self):
        self.checkPolarFrame(body_placed=True, feature_placed=True, linked=True)

    def testPolarPatternReversed(self):
        self.checkPolarFrame(body_placed=True, tilted=True, reversed=True)

    def testPolarPatternCustomSpacings(self):
        self.checkPolarFrame(body_placed=True, tilted=True, spacing=True)

    def testPolarPatternInsideMultiTransformControl(self):
        self.checkPolarFrame(multi=True)

    def testPolarPatternPlacedMultiTransformControl(self):
        self.checkPolarFrame(body_placed=True, feature_placed=True, multi=True)

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
