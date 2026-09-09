# SPDX-License-Identifier: LGPL-2.1-or-later
"""GUI command defaults; run with FreeCAD -t TestPartDesignGui.TestThinPadDefaults."""

import unittest
import FreeCAD as App
import FreeCADGui as Gui
import Part
import Sketcher
from PySide import QtCore, QtWidgets


class TestThinPadDefaults(unittest.TestCase):
    def setUp(self):
        Gui.activateWorkbench("PartDesignWorkbench")
        self.gizmoPrefs = App.ParamGet("User parameter:BaseApp/Preferences/Gui/Gizmos")
        self.gizmosEnabled = self.gizmoPrefs.GetBool("EnableGizmos", True)
        self.gizmoPrefs.SetBool("EnableGizmos", True)
        self.doc = App.newDocument("ThinPadCommand")
        self.body = self.doc.addObject("PartDesign::Body", "Body")
        Gui.activeDocument().activeView().setActiveObject("pdbody", self.body)

    def tearDown(self):
        Gui.Selection.clearSelection()
        if Gui.activeDocument().getInEdit():
            Gui.activeDocument().resetEdit()
        Gui.Control.closeDialog()
        App.closeDocument(self.doc.Name)
        self.gizmoPrefs.SetBool("EnableGizmos", self.gizmosEnabled)

    def sketch(self, closed=False):
        sketch = self.body.newObject("Sketcher::SketchObject", "Profile")
        points = [App.Vector(0, 0, 0), App.Vector(20, 0, 0), App.Vector(20, 20, 0)]
        if closed:
            points.extend([App.Vector(0, 20, 0), points[0]])
        sketch.addGeometry([Part.LineSegment(a, b) for a, b in zip(points, points[1:])], False)
        self.doc.recompute()
        return sketch

    def createPad(self, profile, thin, sub=""):
        return self.createExtrusion("Pad", profile, thin, [sub])

    def createExtrusion(self, kind, profile, thin, subs):
        Gui.Selection.clearSelection()
        for sub in subs:
            Gui.Selection.addSelection(self.doc.Name, profile.Name, sub)
        Gui.runCommand("PartDesign_" + kind, 0)
        # Check the command's first preview, without a second recompute masking errors.
        feature = self.doc.getObject(kind)
        self.assertIsNotNone(feature)
        self.assertEqual(feature.Thin, thin)
        self.assertEqual(feature.ThinExtension, "Off")
        self.assertTrue(feature.ThinExtendAll)
        self.assertNotIn("Invalid", feature.State, feature.getStatusString())
        self.assertTrue(feature.Shape.isValid())
        self.assertEqual(len(feature.Shape.Solids), 1)
        if kind == "Pocket":
            self.assertLess(feature.Shape.Volume, self.doc.Base.Shape.Volume)
        checkbox = Gui.getMainWindow().findChild(QtWidgets.QCheckBox, "thinMode")
        self.assertIsNotNone(checkbox)
        self.assertEqual(checkbox.isChecked(), thin)
        self.assertEqual(checkbox.text(), "Web Mode")
        return feature

    def testOpenSketch(self):
        self.createPad(self.sketch(), True)

    def pocketBase(self):
        base = self.body.newObject("PartDesign::Feature", "Base")
        base.Shape = Part.makeBox(30, 30, 10, App.Vector(-5, -5, 0))
        self.doc.recompute()
        return base

    def pocketSketch(self, closed=False):
        self.pocketBase()
        sketch = self.sketch(closed)
        sketch.Placement.Base.z = 10
        self.doc.recompute()
        return sketch

    def testPocketOpenSketch(self):
        self.createExtrusion("Pocket", self.pocketSketch(), True, [""])

    def testPocketSelectedSketchEdge(self):
        self.createExtrusion("Pocket", self.pocketSketch(True), True, ["Edge1"])

    def testPocketSelectedSketchEdges(self):
        self.createExtrusion("Pocket", self.pocketSketch(True), True, ["Edge1", "Edge2"])

    def testPocketClosedSketch(self):
        self.createExtrusion("Pocket", self.pocketSketch(True), False, [""])

    def testPocketWholeWire(self):
        self.pocketBase()
        wire = self.body.newObject("PartDesign::ShapeBinder", "Wire")
        wire.Shape = Part.Wire([Part.makeCircle(4, App.Vector(10, 10, 10))])
        self.doc.recompute()
        self.createExtrusion("Pocket", wire, True, [""])

    def testPocketWholeEdge(self):
        self.pocketBase()
        edge = self.body.newObject("PartDesign::ShapeBinder", "Edge")
        edge.Shape = Part.makeLine(App.Vector(0, 0, 10), App.Vector(20, 0, 10))
        self.doc.recompute()
        self.createExtrusion("Pocket", edge, True, [""])

    def testPocketSelectedFace(self):
        base = self.pocketBase()
        top = next(i + 1 for i, face in enumerate(base.Shape.Faces) if face.normalAt(0, 0).z > 0.99)
        self.createExtrusion("Pocket", base, False, ["Face%d" % top])

    def testPocketSelectedSolidEdge(self):
        base = self.pocketBase()
        topEdge = next(
            i + 1
            for i, edge in enumerate(base.Shape.Edges)
            if all(abs(vertex.Point.z - 10) < 1e-7 for vertex in edge.Vertexes)
        )
        self.createExtrusion("Pocket", base, True, ["Edge%d" % topEdge])

    def checkExtensionControls(self, feature):
        combo = Gui.getMainWindow().findChild(QtWidgets.QComboBox, "thinExtension")
        self.assertIsNotNone(combo)
        self.assertEqual([combo.itemText(i) for i in range(combo.count())], ["Off", "C1", "C2"])
        controls = Gui.getMainWindow().findChild(QtWidgets.QWidget, "thinExtensionControls")
        all_ends = Gui.getMainWindow().findChild(QtWidgets.QCheckBox, "thinExtendAll")
        self.assertTrue(all_ends.isChecked())
        button = Gui.getMainWindow().findChild(QtWidgets.QToolButton, "thinSelectExtensionEdges")
        self.assertEqual(button.text(), "+ Add edge")
        self.assertTrue(button.icon().isNull())
        for index in (1, 2, 0):
            combo.setCurrentIndex(index)
            self.assertEqual(feature.ThinExtension, ["Off", "Tangent", "Natural"][index])
            self.assertTrue(controls.isHidden())
            self.assertEqual(all_ends.isHidden(), index == 0)
            self.assertNotIn("Invalid", feature.State, feature.getStatusString())
            if index:
                all_ends.setChecked(False)
                self.assertFalse(feature.ThinExtendAll)
                self.assertFalse(controls.isHidden())
                self.assertNotIn("Invalid", feature.State, feature.getStatusString())
                all_ends.setChecked(True)
                self.assertTrue(feature.ThinExtendAll)
                self.assertTrue(controls.isHidden())
        combo.setCurrentIndex(1)
        all_ends.setChecked(False)
        button.click()
        profile = feature.Profile[0]
        Gui.Selection.addSelection(self.doc.Name, profile.Name, "Edge1")
        self.assertEqual(feature.ThinExtensionEdges[1], ["Edge1"])
        self.assertFalse(all_ends.isChecked())
        table = Gui.getMainWindow().findChild(QtWidgets.QTableWidget, "thinExtensionEdgesTable")
        table.selectRow(0)
        Gui.getMainWindow().findChild(QtWidgets.QToolButton, "thinRemoveExtensionEdges").click()
        self.assertFalse(feature.ThinExtendAll)
        self.assertFalse(all_ends.isChecked())
        self.assertFalse(controls.isHidden())
        self.assertEqual(table.rowCount(), 0)
        combo.setCurrentIndex(0)
        self.assertTrue(all_ends.isHidden())
        self.assertTrue(controls.isHidden())
        self.assertFalse(button.isChecked())

    def testWebExtensionPadControls(self):
        from PartDesignTests.TestThinExtrudeFeature import TestThinExtrudeFeature

        fixture = TestThinExtrudeFeature()
        fixture.doc = self.doc
        body, base, sketch, pad, inner = fixture.bowlWall(kind="PartDesign::Pad")
        pad.ThinExtension = "Off"
        Gui.activeDocument().activeView().setActiveObject("pdbody", body)
        Gui.activeDocument().setEdit(pad.Name)
        self.checkExtensionControls(pad)

    def testWebExtensionPocketControls(self):
        base = self.body.newObject("PartDesign::Feature", "Block")
        base.Shape = Part.makeBox(40, 30, 20, App.Vector(-20, -15, 0))
        sketch = self.body.newObject("Sketcher::SketchObject", "CutProfile")
        sketch.Placement.Base.z = 20
        sketch.addGeometry(Part.LineSegment(App.Vector(-8, 0, 0), App.Vector(8, 0, 0)), False)
        pocket = self.body.newObject("PartDesign::Pocket", "Pocket")
        pocket.Profile = sketch
        pocket.Thin = True
        pocket.Length = 5
        self.assertEqual(pocket.ThinExtension, "Off")
        self.doc.recompute()
        Gui.activeDocument().setEdit(pocket.Name)
        checkbox = Gui.getMainWindow().findChild(QtWidgets.QCheckBox, "thinMode")
        self.assertEqual(checkbox.text(), "Web Mode")
        self.checkExtensionControls(pocket)

    def testClosedSketch(self):
        self.createPad(self.sketch(True), False)

    def testSelectedEdgeOfClosedSketch(self):
        self.createPad(self.sketch(True), True, "Edge1")

    def testClosedSketchWithConstructionEdge(self):
        sketch = self.sketch(True)
        sketch.addGeometry(Part.LineSegment(App.Vector(2, 2, 0), App.Vector(3, 3, 0)), True)
        self.doc.recompute()
        self.createPad(sketch, False)

    def testClosedAndOpenSketch(self):
        sketch = self.sketch(True)
        sketch.addGeometry(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(10, 10, 0)), False)
        self.doc.recompute()
        self.createPad(sketch, True)

    def testWholeWire(self):
        wire = self.body.newObject("PartDesign::ShapeBinder", "Wire")
        wire.Shape = Part.Wire([Part.makeCircle(10)])
        self.doc.recompute()
        self.createPad(wire, True)

    def testWholeEdge(self):
        edge = self.body.newObject("PartDesign::ShapeBinder", "Edge")
        edge.Shape = Part.makeLine(App.Vector(), App.Vector(20, 0, 0))
        self.doc.recompute()
        self.createPad(edge, True)

    def testSelectedFace(self):
        base = self.body.newObject("PartDesign::Feature", "Base")
        base.Shape = Part.makeBox(20, 20, 5)
        self.doc.recompute()
        top = next(i + 1 for i, f in enumerate(base.Shape.Faces) if f.normalAt(0, 0).z > 0.99)
        self.createPad(base, False, "Face%d" % top)

    def gizmo(self, name):
        from pivy import coin

        coin.SoBaseKit.setSearchingChildren(True)
        search = coin.SoSearchAction()
        search.setName(name)
        search.setSearchingAll(True)
        search.apply(Gui.activeDocument().activeView().getViewer().getSceneGraph())
        self.assertIsNotNone(search.getPath(), name)
        return search.getPath().getTail()

    def testThinGizmos(self):
        pad = self.createPad(self.sketch(), True)
        first = self.gizmo("ThinThickness")
        second = self.gizmo("ThinSecondThickness")
        draft = self.gizmo("ThinDraft")
        self.assertTrue(first.getField("visible").getValue())
        self.assertFalse(second.getField("visible").getValue())
        self.assertTrue(draft.getField("visible").getValue())
        placement = Gui.getMainWindow().findChild(QtWidgets.QComboBox, "thinSide")
        placement.setCurrentIndex(3)
        self.assertTrue(second.getField("visible").getValue())
        thickness = Gui.getMainWindow().findChild(QtWidgets.QAbstractSpinBox, "thinThickness")
        thickness.setProperty("rawValue", 4.0)
        self.assertAlmostEqual(pad.ThinThickness.Value, 4)
        self.assertNotIn("Invalid", pad.State, pad.getStatusString())
        placement.setCurrentIndex(2)
        Gui.updateGui()
        # Centered placement displays half of the entered total width.
        dragger = first.getPart("dragger", False)
        self.assertAlmostEqual(dragger.getField("translation").getValue()[1], 2, places=6)

    def testSolidHidesThinGizmos(self):
        self.createPad(self.sketch(True), False)
        for name in ("ThinThickness", "ThinSecondThickness", "ThinDraft", "ThinSecondDraft"):
            self.assertFalse(self.gizmo(name).getField("visible").getValue(), name)

    def testCrossPocketTaperGizmosAndRecovery(self):
        from PartDesignTests.TestThinExtrudeFeature import TestThinExtrudeFeature

        fixture = TestThinExtrudeFeature()
        fixture.doc = self.doc
        fixture.testThinPocketExtensionModes()
        pocket = self.doc.Pocket
        pocket.ThinExtensionEdges = None
        pocket.ThinExtension = "Off"
        self.doc.recompute()
        Gui.activeDocument().activeView().setActiveObject("pdbody", self.doc.PocketBody)
        Gui.activeDocument().setEdit(pocket.Name)
        from pivy import coin

        container = coin.SoNode.getByName("ExtrudeGizmos")
        self.assertIsNotNone(container)
        taper = Gui.getMainWindow().findChild(QtWidgets.QAbstractSpinBox, "taperEdit")
        for angle in (2, -2, 0):
            taper.setProperty("rawValue", float(angle))
            self.assertNotIn("Invalid", pocket.State, pocket.getStatusString())
            self.assertTrue(container.getField("visible").getValue())
            self.assertTrue(self.gizmo("ThinThickness").getField("visible").getValue())
            self.assertTrue(self.gizmo("ThinDraft").getField("visible").getValue())
        # An excessive angle collapses the wall, but the last valid gizmo
        # placements must remain available to correct that setting.
        taper.setProperty("rawValue", 45.0)
        self.assertIn("Invalid", pocket.State)
        self.assertTrue(container.getField("visible").getValue())
        taper.setProperty("rawValue", 2.0)
        self.assertNotIn("Invalid", pocket.State, pocket.getStatusString())
        self.assertTrue(container.getField("visible").getValue())

    def testUpToFaceThinDraftGizmo(self):
        base = self.body.newObject("PartDesign::Feature", "Base")
        base.Shape = Part.makeBox(30, 20, 4, App.Vector(0, 0, -4))
        sketch = self.body.newObject("Sketcher::SketchObject", "Profile")
        sketch.addGeometry(Part.LineSegment(App.Vector(3, 10, 0), App.Vector(27, 10, 0)), False)
        sketch.Placement.Base.z = 10
        pad = self.body.newObject("PartDesign::Pad", "Pad")
        pad.Profile = sketch
        pad.Thin = True
        pad.ThinThickness = 2
        pad.Reversed = True
        pad.Type = "UpToFace"
        top = next(i + 1 for i, f in enumerate(base.Shape.Faces) if f.normalAt(0, 0).z > 0.99)
        pad.UpToFace = (base, ["Face%d" % top])
        self.doc.recompute()
        self.assertNotIn("Invalid", pad.State, pad.getStatusString())
        Gui.activeDocument().setEdit(pad.Name)
        self.assertTrue(self.gizmo("ThinDraft").getField("visible").getValue())
        thickness = self.gizmo("ThinThickness")
        self.assertAlmostEqual(thickness.getField("translation").getValue()[2], 0, places=5)
        draft = Gui.getMainWindow().findChild(QtWidgets.QAbstractSpinBox, "taperEdit")
        self.assertTrue(draft.isEnabled())
        volumes = []
        for angle in (-2, 2):
            draft.setProperty("rawValue", angle)
            self.assertNotIn("Invalid", pad.State, pad.getStatusString())
            self.assertAlmostEqual(pad.TaperAngle.Value, angle)
            volumes.append(pad.Shape.Volume)
        self.assertNotAlmostEqual(*volumes, places=4)
        neutral = Gui.getMainWindow().findChild(QtWidgets.QComboBox, "thinDraftReference")
        neutral.setCurrentIndex(1)
        self.assertAlmostEqual(thickness.getField("translation").getValue()[2], 10, places=5)


class TestRibReferenceSelection(unittest.TestCase):
    def setUp(self):
        from PartDesignTests.TestRib import TestRib

        Gui.activateWorkbench("PartDesignWorkbench")
        self.fixture = TestRib("testSideProfileRib")
        self.fixture.setUp()
        self.fixture.testSideProfileRib()
        self.doc = self.fixture.doc
        self.rib = self.doc.Rib
        self.point = self.doc.Body.newObject("PartDesign::Point", "TargetPoint")
        self.point.Placement.Base = App.Vector(16, 12, 0)
        self.plane = self.doc.Body.newObject("PartDesign::Plane", "DirectionPlane")
        self.plane.Placement.Rotation = App.Rotation(App.Vector(1, 0, 0), 180)
        self.doc.Body.Tip = self.rib
        self.doc.recompute()
        Gui.activeDocument().activeView().setActiveObject("pdbody", self.doc.Body)
        Gui.activeDocument().setEdit(self.rib.Name)

    def tearDown(self):
        Gui.Selection.clearSelection()
        Gui.activeDocument().resetEdit()
        Gui.Control.closeDialog()
        self.fixture.tearDown()

    def widget(self, kind, name):
        widget = Gui.getMainWindow().findChild(kind, name)
        self.assertIsNotNone(widget, name)
        return widget

    def testTowardPointAndFace(self):
        self.widget(QtWidgets.QComboBox, "ribDirectionMode").setCurrentIndex(1)
        top = next(
            i + 1
            for i, f in enumerate(self.doc.Base.Shape.Faces)
            if f.normalAt(0, 0).z > 0.99 and abs(f.CenterOfMass.z) < 1e-7
        )
        for obj, sub in ((self.point, ""), (self.doc.Base, "Face%d" % top)):
            button = self.widget(QtWidgets.QToolButton, "ribSelectDirection")
            button.click()
            Gui.Selection.addSelection(self.doc.Name, obj.Name, sub)
            self.assertFalse(button.isChecked())
            self.assertEqual(self.rib.ReferenceAxis[0], obj)
            self.assertIn(
                obj.Label, self.widget(QtWidgets.QLineEdit, "ribDirectionReference").text()
            )
            self.doc.recompute()
            self.fixture.valid(self.rib)

    def testAdvancedCollapsedByDefault(self):
        advanced = self.widget(QtWidgets.QWidget, "ribAdvancedParameters")
        contents = self.widget(QtWidgets.QWidget, "ribAdvancedPanel")
        self.assertTrue(advanced.isVisible())
        self.assertFalse(contents.isVisible())
        self.assertTrue(
            QtCore.QMetaObject.invokeMethod(advanced, "showHide", QtCore.Qt.DirectConnection)
        )
        loop = QtCore.QEventLoop()
        QtCore.QTimer.singleShot(500, loop.quit)
        loop.exec()
        self.assertTrue(contents.isVisible())

    def testParallelDatumPlane(self):
        self.widget(QtWidgets.QComboBox, "ribDirectionMode").setCurrentIndex(2)
        button = self.widget(QtWidgets.QToolButton, "ribSelectDirection")
        button.click()
        # A point has no intrinsic axis and must not be accepted in this mode.
        Gui.Selection.addSelection(self.doc.Name, self.point.Name)
        self.assertTrue(button.isChecked())
        Gui.Selection.addSelection(self.doc.Name, self.plane.Name)
        self.assertFalse(button.isChecked())
        self.assertEqual(self.rib.ReferenceAxis[0], self.plane)
        self.doc.recompute()
        self.fixture.valid(self.rib)

    def testDraftTowardDatumPoint(self):
        self.widget(QtWidgets.QComboBox, "ribPullMode").setCurrentIndex(2)
        button = self.widget(QtWidgets.QToolButton, "ribSelectPullDirection")
        button.click()
        Gui.Selection.addSelection(self.doc.Name, self.point.Name)
        self.assertFalse(button.isChecked())
        self.assertEqual(self.rib.DraftPullDirection[0], self.point)
        self.rib.TaperAngle = 0.5
        self.doc.recompute()
        self.fixture.valid(self.rib)

    def testDesignerVisibilityAndExtentMapping(self):
        extent = self.widget(QtWidgets.QComboBox, "ribExtent")
        length = self.widget(QtWidgets.QAbstractSpinBox, "ribLength")
        reversedBox = self.widget(QtWidgets.QCheckBox, "ribReversed")
        second = self.widget(QtWidgets.QAbstractSpinBox, "ribThickness2")
        self.assertEqual(
            [extent.itemData(i) for i in range(extent.count())], ["UpToShape", "Length"]
        )
        self.assertTrue(length.isHidden())
        self.assertTrue(reversedBox.isHidden())
        extent.setCurrentIndex(1)
        self.assertEqual(self.rib.Type, "Length")
        self.assertFalse(length.isHidden())
        self.assertFalse(reversedBox.isHidden())
        self.widget(QtWidgets.QComboBox, "ribPlacement").setCurrentIndex(3)
        self.assertFalse(second.isHidden())
        self.widget(QtWidgets.QComboBox, "ribPlacement").setCurrentIndex(2)
        self.assertTrue(second.isHidden())
        extent.setCurrentIndex(0)
        self.assertEqual(self.rib.Type, "UpToShape")
        self.assertTrue(length.isHidden())

    def testDesignerQuantityBindings(self):
        thickness = self.widget(QtWidgets.QAbstractSpinBox, "ribThickness")
        draft = self.widget(QtWidgets.QAbstractSpinBox, "ribDraftAngle")
        thickness.setProperty("rawValue", 3.0)
        self.assertAlmostEqual(self.rib.ThinThickness.Value, 3.0)
        for angle in (-0.5, 0.5, 0.0):
            draft.setProperty("rawValue", angle)
            self.assertAlmostEqual(self.rib.TaperAngle.Value, angle)
            self.fixture.valid(self.rib)

    def testDesignerReferenceClear(self):
        mode = self.widget(QtWidgets.QComboBox, "ribDirectionMode")
        mode.setCurrentIndex(1)
        self.widget(QtWidgets.QToolButton, "ribSelectDirection").click()
        Gui.Selection.addSelection(self.doc.Name, self.point.Name)
        self.assertEqual(self.rib.ReferenceAxis[0], self.point)
        self.widget(QtWidgets.QToolButton, "ribClearDirection").click()
        self.assertEqual(mode.currentIndex(), 0)
        self.assertTrue(self.rib.AutoDirection)
        self.assertFalse(self.rib.ReferenceAxis)
        self.assertEqual(self.widget(QtWidgets.QLineEdit, "ribDirectionReference").text(), "")
        self.fixture.valid(self.rib)

    def testDesignerRetranslationPreservesValues(self):
        panel = self.widget(QtWidgets.QWidget, "ribParametersPanel")
        while panel and panel.metaObject().className() != "PartDesignGui::TaskRibParameters":
            panel = panel.parentWidget()
        self.assertIsNotNone(panel)
        extent = self.widget(QtWidgets.QComboBox, "ribExtent")
        extent.setCurrentIndex(0)
        thickness = self.rib.ThinThickness.Value
        volume = self.rib.Shape.Volume
        QtCore.QCoreApplication.sendEvent(panel, QtCore.QEvent(QtCore.QEvent.LanguageChange))
        self.assertEqual(extent.currentData(), "UpToShape")
        self.assertEqual(self.rib.Type, "UpToShape")
        self.assertEqual(self.rib.ThinThickness.Value, thickness)
        self.assertAlmostEqual(self.rib.Shape.Volume, volume)
        self.assertEqual(self.widget(QtWidgets.QComboBox, "ribDirectionMode").count(), 5)
        self.assertEqual(self.widget(QtWidgets.QComboBox, "ribPullMode").count(), 6)
