# SPDX-License-Identifier: LGPL-2.1-or-later
import os
import tempfile
import unittest

import FreeCAD as App
import Part
import Sketcher


def isolatedLayerDefaults(test):
    preferences = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Sketcher/LayerDefaults")
    types = {"String": "String", "Integer": "Int", "Float": "Float", "Boolean": "Bool"}
    keys = {"Color": "String", "Pattern": "Int", "LineWidth": "Float", "Locked": "Bool",
            "UseConstraints": "Bool", "UseSolvedStateColors": "Bool"}
    previous = [entry for entry in preferences.GetContents() or [] if entry[1] in keys]
    def clear():
        for key, kind in keys.items():
            getattr(preferences, "Rem" + kind)(key)
    def restore():
        clear()
        for kind, key, value in previous:
            getattr(preferences, "Set" + types[kind])(key, value)
    test.addCleanup(restore)
    clear()
    return preferences


class TestSketchLayers(unittest.TestCase):
    def setUp(self):
        self.defaults = isolatedLayerDefaults(self)
        self.doc = App.newDocument("SketchLayers")
        self.doc.UndoMode = 1
        self.sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")

    def tearDown(self):
        App.closeDocument(self.doc.Name)

    def line(self, x=0, construction=False):
        return self.sketch.addGeometry(
            Part.LineSegment(App.Vector(x, 0, 0), App.Vector(x + 10, 0, 0)), construction
        )

    def testLayerPolicyDefaultsOnlyApplyAtCreation(self):
        s = self.sketch
        self.defaults.SetBool("Locked", True)
        self.defaults.SetBool("UseConstraints", False)
        self.doc.openTransaction("Layer defaults")
        layer = s.addLayer("Defaults")
        self.doc.commitTransaction()
        self.assertEqual(s.LockedLayers, [])
        self.assertEqual(s.UnconstrainedLayers, [layer])
        other = self.doc.addObject("Sketcher::SketchObject", "Other")
        self.assertEqual(other.LockedLayers, [])
        self.assertEqual(other.UnconstrainedLayers, [0])
        self.doc.removeObject(other.Name)
        self.doc.undo()
        self.assertNotIn(str(layer), s.Layers)
        self.defaults.SetBool("Locked", False)
        self.defaults.SetBool("UseConstraints", True)
        self.doc.redo()
        self.assertEqual(s.LockedLayers, [])
        self.assertEqual(s.UnconstrainedLayers, [layer])
        next_layer = s.addLayer("Next")
        self.assertNotIn(next_layer, s.LockedLayers)
        self.assertNotIn(next_layer, s.UnconstrainedLayers)

    def testRegistryAndActiveLayer(self):
        s = self.sketch
        self.assertEqual(s.Layers, {"0": "Default"})
        self.assertEqual(s.ActiveLayer, 0)
        first = s.addLayer("Profiles")
        second = s.addLayer("Construction")
        s.renameLayer(first, "Profilé 'α'")
        self.assertEqual(s.Layers[str(first)], "Profilé 'α'")
        s.setActiveLayer(first)
        self.assertEqual(s.getGeometryLayer(self.line()), first)
        self.assertEqual(s.getGeometryLayer(self.line(20, True)), first)
        s.removeLayer(first)
        self.assertEqual(s.ActiveLayer, 0)
        self.assertEqual([s.getGeometryLayer(i) for i in range(2)], [0, 0])
        self.assertGreater(s.addLayer("New layer"), second)
        for name in ("", "  ", "Construction"):
            with self.assertRaises(ValueError):
                s.addLayer(name)
        for call in (lambda: s.removeLayer(0), lambda: s.setActiveLayer(999),
                     lambda: s.renameLayer(999, "Invalid")):
            with self.assertRaises(ValueError):
                call()

    def testAssignmentIsAtomicAndPreservesSketch(self):
        s = self.sketch
        a, b = self.line(), self.line(20, True)
        s.addConstraint(Sketcher.Constraint("Horizontal", a))
        self.doc.recompute()
        shape = s.Shape.Length
        dof = s.DoF
        constraint = s.Constraints[0].Content
        layer = s.addLayer("Other")
        for invalid in (-1, -2, -3, 50, -2147483648):
            with self.assertRaises(ValueError):
                s.setGeometryLayer([a, invalid], layer)
            self.assertEqual(s.getGeometryLayer(a), 0)
        s.setGeometryLayer([a, b, a], layer)
        self.doc.recompute()
        self.assertEqual(s.getGeometryLayer(a), layer)
        self.assertTrue(s.getConstruction(b))
        self.assertEqual(s.Constraints[0].Content, constraint)
        self.assertAlmostEqual(s.Shape.Length, shape)
        self.assertEqual(s.DoF, dof)

    def testUndoRedoAndSave(self):
        s = self.sketch
        geo = self.line()
        self.doc.openTransaction("Create and assign layer")
        layer = s.addLayer("Saved")
        s.setActiveLayer(layer)
        s.setGeometryLayer([geo], layer)
        self.doc.commitTransaction()
        self.doc.undo()
        self.assertNotIn(str(layer), s.Layers)
        self.assertEqual(s.getGeometryLayer(geo), 0)
        self.doc.redo()
        self.assertEqual(s.getGeometryLayer(geo), layer)
        self.assertEqual(s.ActiveLayer, layer)
        with tempfile.TemporaryDirectory() as folder:
            path = os.path.join(folder, "layers.FCStd")
            self.doc.recompute()
            self.doc.saveAs(path)
            App.closeDocument(self.doc.Name)
            self.doc = App.openDocument(path)
            s = self.doc.getObject("Sketch")
            self.assertEqual(s.Layers[str(layer)], "Saved")
            self.assertEqual(s.ActiveLayer, layer)
            self.assertEqual(s.getGeometryLayer(geo), layer)

    def testInternalGeometryFollowsParent(self):
        s = self.sketch
        parent = s.addGeometry(Part.Ellipse(App.Vector(0, 0, 0), 10, 5))
        layer = s.addLayer("Ellipse")
        s.setGeometryLayer([parent], layer)
        s.exposeInternalGeometry(parent)
        self.assertGreater(s.GeometryCount, 1)
        self.assertTrue(all(s.getGeometryLayer(i) == layer for i in range(s.GeometryCount)))
        s.setGeometryLayer([1], 0)
        self.assertTrue(all(s.getGeometryLayer(i) == 0 for i in range(s.GeometryCount)))

    def testExternalGeometrySurvivesRebuild(self):
        source = self.doc.addObject("PartDesign::Feature", "Source")
        source.Shape = Part.makeLine(App.Vector(0, 0, 0), App.Vector(20, 0, 0))
        self.doc.recompute()
        s = self.sketch
        layer = s.addLayer("References")
        s.setActiveLayer(layer)
        s.addExternal(source.Name, "Edge1")
        self.doc.recompute()
        self.assertEqual(s.getGeometryLayer(-3), layer)
        s.setGeometryLayer([-3], 0)
        source.Shape = Part.makeLine(App.Vector(0, 0, 0), App.Vector(30, 0, 0))
        self.doc.recompute()
        self.assertEqual(s.getGeometryLayer(-3), 0)
        s.setGeometryLayer([-3], layer)
        s.removeLayer(layer)
        self.assertEqual(s.getGeometryLayer(-3), 0)

    def testSplitPreservesLayer(self):
        s = self.sketch
        geo = self.line()
        layer = s.addLayer("Profile")
        s.setGeometryLayer([geo], layer)
        s.split(geo, App.Vector(5, 0, 0))
        self.assertEqual(s.GeometryCount, 2)
        self.assertEqual([s.getGeometryLayer(i) for i in range(2)], [layer, layer])

    def testGroupMovesTogether(self):
        s = self.sketch
        handle, member = self.line(), self.line(20)
        s.addConstraint(Sketcher.Constraint("Group", [handle, 0, member, 0]))
        layer = s.addLayer("Group")
        s.setGeometryLayer([member], layer)
        self.assertEqual([s.getGeometryLayer(i) for i in (handle, member)], [layer, layer])

    def testCarbonCopyUsesDestinationLayer(self):
        source = self.doc.addObject("Sketcher::SketchObject", "Source")
        sourceLayer = source.addLayer("Source layer")
        source.setActiveLayer(sourceLayer)
        source.addGeometry(Part.Circle(App.Vector(), App.Vector(0, 0, 1), 5))
        self.doc.recompute()
        s = self.sketch
        s.addLayer("Unrelated layer with same ID")
        targetLayer = s.addLayer("Imported")
        s.setActiveLayer(targetLayer)
        s.carbonCopy(source.Name, False)
        self.assertEqual(s.getGeometryLayer(0), targetLayer)

    def testLegacyGeometryRestoresToDefaultLayer(self):
        import re
        import zipfile
        s = self.sketch
        self.line()
        layer = s.addLayer("Active")
        s.setActiveLayer(layer)
        self.doc.recompute()
        with tempfile.TemporaryDirectory() as folder:
            path = os.path.join(folder, "original.FCStd")
            legacy = os.path.join(folder, "legacy.FCStd")
            self.doc.saveAs(path)
            with zipfile.ZipFile(path) as source, zipfile.ZipFile(legacy, "w") as target:
                replacements = 0
                for entry in source.infolist():
                    content = source.read(entry.filename)
                    if entry.filename == "Document.xml":
                        content, replacements = re.subn(rb' geometryLayer="-?\d+"', b'', content)
                    target.writestr(entry, content)
            self.assertGreater(replacements, 0)
            App.closeDocument(self.doc.Name)
            self.doc = App.openDocument(legacy)
            s = self.doc.getObject("Sketch")
            self.assertEqual(s.ActiveLayer, layer)
            self.assertEqual(s.getGeometryLayer(0), 0)
            copy = s.addGeometry(s.Geometry[0])
            self.assertEqual(s.getGeometryLayer(copy), 0)
            fresh = s.addGeometry(Part.Circle(App.Vector(), App.Vector(0, 0, 1), 5))
            self.assertEqual(s.getGeometryLayer(fresh), layer)

    def testPreparedGeometryUsesActiveLayer(self):
        s = self.sketch
        layer = s.addLayer("Active")
        s.setActiveLayer(layer)
        # Offset prepares geometry this way before passing it to addGeometry.
        for construction in (False, True):
            facade = Sketcher.GeometryFacade(
                Part.LineSegment(App.Vector(0, 0, 0), App.Vector(10, 0, 0)))
            facade.Construction = construction
            geo = s.addGeometry(facade.Geometry)
            self.assertEqual(s.getGeometryLayer(geo), layer)
            self.assertEqual(s.getConstruction(geo), construction)
        # Explicit default-layer membership is distinct from an unassigned layer.
        facade.GeometryLayerId = 0
        geo = s.addGeometry(facade.Geometry)
        self.assertEqual(s.getGeometryLayer(geo), 0)
        copy = s.addGeometry(s.Geometry[geo])
        self.assertEqual(s.getGeometryLayer(copy), 0)

    def testTrimCirclePreservesLayerAndUndo(self):
        s = self.sketch
        layer = s.addLayer("Profile")
        circle = s.addGeometry(Part.Circle(App.Vector(), App.Vector(0, 0, 1), 5))
        s.addGeometry(Part.LineSegment(App.Vector(-10, 0, 0), App.Vector(10, 0, 0)))
        s.setGeometryLayer([circle], layer)
        self.doc.recompute()
        self.doc.openTransaction("Trim layered circle")
        s.trim(circle, App.Vector(0, 5, 0))
        self.doc.commitTransaction()
        self.assertIsInstance(s.Geometry[circle], Part.ArcOfCircle)
        self.assertEqual(s.getGeometryLayer(circle), layer)
        self.doc.undo()
        self.assertIsInstance(s.Geometry[circle], Part.Circle)
        self.assertEqual(s.getGeometryLayer(circle), layer)
        self.doc.redo()
        self.assertEqual(s.getGeometryLayer(circle), layer)

    def testTrimAdditionalPiecesPreserveLayer(self):
        s = self.sketch
        line = self.line()
        layer = s.addLayer("Profile")
        s.setGeometryLayer([line], layer)
        for x in (3, 7):
            s.addGeometry(Part.LineSegment(App.Vector(x, -5, 0), App.Vector(x, 5, 0)))
        self.doc.recompute()
        s.trim(line, App.Vector(5, 0, 0))
        self.assertEqual(s.GeometryCount, 4)
        self.assertEqual([s.getGeometryLayer(i) for i in range(4)], [layer, 0, 0, layer])

    def testJoinPreservesSourceLayer(self):
        s = self.sketch
        layer = s.addLayer("Profile")
        self.line()
        s.addGeometry(Part.LineSegment(App.Vector(10, 0, 0), App.Vector(20, 5, 0)))
        s.setGeometryLayer([0, 1], layer)
        self.doc.recompute()
        s.join(0, 2, 1, 1)
        self.assertIsInstance(s.Geometry[0], Part.BSplineCurve)
        self.assertTrue(all(s.getGeometryLayer(i) == layer for i in range(s.GeometryCount)))

    def testGroupMovesFromInternalGeometry(self):
        s = self.sketch
        handle = self.line(20)
        ellipse = s.addGeometry(Part.Ellipse(App.Vector(), 10, 5))
        s.exposeInternalGeometry(ellipse)
        second = s.addGeometry(Part.Ellipse(App.Vector(40, 0, 0), 10, 5))
        s.exposeInternalGeometry(second)
        s.addConstraint(Sketcher.Constraint("Group", [handle, 0, ellipse, 0, second, 0]))
        layer = s.addLayer("Group")
        self.doc.openTransaction("Move from internal geometry")
        s.setGeometryLayer([ellipse + 1], layer)
        self.doc.commitTransaction()
        self.assertTrue(all(s.getGeometryLayer(i) == layer for i in range(s.GeometryCount)))
        self.doc.undo()
        self.assertTrue(all(s.getGeometryLayer(i) == 0 for i in range(s.GeometryCount)))
        self.doc.redo()
        self.assertTrue(all(s.getGeometryLayer(i) == layer for i in range(s.GeometryCount)))

    def testArrayConstructionFollowsSourceLayer(self):
        s = self.sketch
        line = self.line()
        layer = s.addLayer("Profile")
        s.setGeometryLayer([line], layer)
        s.addRectangularArray([line], App.Vector(0, 10, 0), False, 1, 2, True)
        self.assertEqual(s.GeometryCount, 3)
        self.assertTrue(s.getConstruction(2))
        self.assertEqual([s.getGeometryLayer(i) for i in range(3)], [layer] * 3)

    def testSplineConversionPreservesLayer(self):
        s = self.sketch
        geo = s.addGeometry(Part.Circle(App.Vector(), App.Vector(0, 0, 1), 5))
        layer = s.addLayer("Curve")
        s.setGeometryLayer([geo], layer)
        s.convertToNURBS(geo)
        self.assertEqual(s.getGeometryLayer(geo), layer)

    def testUnconstrainedLayerExcludesGeometryAndConstraints(self):
        s = self.sketch
        a, b = self.line(), self.line(20)
        layer = s.addLayer("Free drawing")
        s.setGeometryLayer([a], layer)
        s.addConstraint(Sketcher.Constraint("Horizontal", a))
        s.addConstraint(Sketcher.Constraint("Horizontal", b))
        s.solve()
        self.assertEqual(s.DoF, 6)
        saved = [c.Content for c in s.Constraints]
        s.UnconstrainedLayers = [layer]
        self.assertEqual(s.solve(), 0)
        self.assertEqual(s.DoF, 3)
        self.assertFalse(s.getActive(0))
        self.assertTrue(s.getActive(1))
        self.assertEqual([c.Content for c in s.Constraints], saved)
        self.assertEqual(s.addConstraint(Sketcher.Constraint("Distance", a, 12.0)), -1)
        self.assertEqual(len(s.Constraints), 2)
        ids = s.addConstraint([Sketcher.Constraint("Distance", a, 12.0),
                               Sketcher.Constraint("Distance", b, 15.0)])
        self.assertEqual(ids, (-1, 2))
        s.UnconstrainedLayers = []
        self.assertEqual(s.solve(), 0)
        self.assertEqual(s.DoF, 5)
        self.assertTrue(s.getActive(0))

    def testUnconstrainedLayerStillBuildsAndMoves(self):
        s = self.sketch
        layer = s.addLayer("Free drawing")
        s.setActiveLayer(layer)
        s.UnconstrainedLayers = [layer]
        geo = self.line()
        self.doc.recompute()
        self.assertEqual(s.DoF, 0)
        self.assertAlmostEqual(s.Shape.Length, 10)
        s.moveGeometry(geo, 0, App.Vector(3, 4, 0), True)
        self.assertAlmostEqual(s.Geometry[geo].StartPoint.x, 3)
        self.assertAlmostEqual(s.Geometry[geo].StartPoint.y, 4)
        s.moveGeometry(geo, 2, App.Vector(20, 8, 0), False)
        self.assertAlmostEqual(s.Geometry[geo].EndPoint.x, 20)
        self.assertAlmostEqual(s.Geometry[geo].EndPoint.y, 8)
        s.solve()
        self.assertAlmostEqual(s.Geometry[geo].EndPoint.y, 8)

    def testLayerConstraintSuppressionPreservesManualState(self):
        s = self.sketch
        a, b = self.line(), self.line(20)
        layer = s.addLayer("Free drawing")
        s.setGeometryLayer([a], layer)
        manual, cross = s.addConstraint([Sketcher.Constraint("Horizontal", a),
                                         Sketcher.Constraint("Equal", a, b)])
        s.setActive(manual, False)
        self.doc.openTransaction("Disable layer constraints")
        s.UnconstrainedLayers = [layer]
        self.doc.commitTransaction()
        self.assertFalse(s.getActive(manual))
        self.assertFalse(s.getActive(cross))
        self.doc.undo()
        self.assertFalse(s.getActive(manual))
        self.assertTrue(s.getActive(cross))
        self.doc.redo()
        self.assertFalse(s.getActive(cross))
        s.setGeometryLayer([a], 0)
        self.assertFalse(s.getActive(manual))
        self.assertTrue(s.getActive(cross))

    def testLockedLayerProtectsGeometryAndAllowsOtherLayers(self):
        s = self.sketch
        a, b = self.line(), self.line(20)
        layer = s.addLayer("Locked")
        s.setGeometryLayer([a], layer)
        s.addConstraint(Sketcher.Constraint("Horizontal", a))
        s.LockedLayers = [layer]
        self.assertEqual(s.solve(), 0)
        self.assertEqual(s.DoF, 4)
        original = s.Geometry[a].StartPoint
        for operation in [lambda: s.delGeometry(a), lambda: s.delGeometries([b, a]),
                          lambda: s.toggleConstruction(a),
                          lambda: s.setGeometryLayer([a], 0),
                          lambda: s.setGeometryLayer([b], layer),
                          lambda: s.moveGeometry(a, 1, App.Vector(10, 10, 0), False),
                          lambda: s.removeLayer(layer)]:
            with self.assertRaises((ValueError, RuntimeError)):
                operation()
            self.assertEqual(s.GeometryCount, 2)
            self.assertEqual(s.Geometry[a].StartPoint, original)
        s.moveGeometry(b, 1, App.Vector(25, 5, 0), False)
        self.assertEqual(s.Geometry[a].StartPoint, original)
        s.delGeometry(b)
        self.assertEqual(s.GeometryCount, 1)

    def testLockedLayerRejectsDirectPropertyMutationAndDrawing(self):
        s = self.sketch
        layer = s.addLayer("Locked")
        s.setActiveLayer(layer)
        a = self.line()
        s.LockedLayers = [layer]
        with self.assertRaises((ValueError, RuntimeError)):
            s.Geometry = []
        self.assertEqual(s.GeometryCount, 1)
        geometry = s.Geometry
        geometry[a].StartPoint = App.Vector(8, 2, 0)
        with self.assertRaises((ValueError, RuntimeError)):
            s.Geometry = geometry
        with self.assertRaises((ValueError, RuntimeError)):
            self.line(20)
        self.assertEqual(s.GeometryCount, 1)

    def testLayerFlagsUndoRedoAndRestore(self):
        s = self.sketch
        self.line()
        self.doc.openTransaction("Layer policy")
        s.LockedLayers = [0]
        s.UnconstrainedLayers = [0]
        self.doc.commitTransaction()
        self.doc.undo()
        self.assertEqual(s.LockedLayers, [])
        self.assertEqual(s.UnconstrainedLayers, [])
        self.doc.redo()
        self.assertEqual(s.LockedLayers, [0])
        self.assertEqual(s.UnconstrainedLayers, [0])
        with tempfile.TemporaryDirectory() as folder:
            path = os.path.join(folder, "policies.FCStd")
            self.doc.recompute()
            self.doc.saveAs(path)
            name = self.doc.Name
            App.closeDocument(name)
            self.doc = App.openDocument(path)
            self.sketch = self.doc.getObject("Sketch")
            self.assertEqual(self.sketch.LockedLayers, [0])
            self.assertEqual(self.sketch.UnconstrainedLayers, [0])
            self.assertEqual(self.sketch.solve(), 0)
            self.assertEqual(self.sketch.DoF, 0)
            with self.assertRaises((ValueError, RuntimeError)):
                self.sketch.delGeometry(0)

    def testLockedCompoundOperationsAreAtomic(self):
        s = self.sketch
        a, b = self.line(), self.line(20)
        s.addConstraint(Sketcher.Constraint("Horizontal", a))
        s.LockedLayers = [0]
        constraints = [c.Content for c in s.Constraints]
        for operation in [lambda: s.trim(a, App.Vector(5, 0, 0)),
                          lambda: s.split(a, App.Vector(5, 0, 0)),
                          lambda: s.extend(a, 3.0, 2)]:
            with self.assertRaises((ValueError, RuntimeError)):
                operation()
            self.assertEqual([c.Content for c in s.Constraints], constraints)
            self.assertEqual(s.GeometryCount, 2)
            self.assertAlmostEqual(s.Geometry[a].EndPoint.x, 10)

    def testUnconstrainedGroupStillTransformsWithoutDegreesOfFreedom(self):
        s = self.sketch
        handle, member = self.line(), self.line(20)
        s.addConstraint(Sketcher.Constraint("Group", [handle, 0, member, 0]))
        s.UnconstrainedLayers = [0]
        self.assertEqual(s.solve(), 0)
        self.assertEqual(s.DoF, 0)
        s.moveGeometry(handle, 0, App.Vector(3, 4, 0), True)
        self.assertAlmostEqual(s.Geometry[member].StartPoint.x, 23)
        self.assertAlmostEqual(s.Geometry[member].StartPoint.y, 4)
        self.assertEqual(s.DoF, 0)

    def testMovingIntoUnconstrainedLayerUpdatesSolver(self):
        s = self.sketch
        a = self.line()
        s.solve()
        self.assertEqual(s.DoF, 4)
        layer = s.addLayer("Free drawing")
        s.UnconstrainedLayers = [layer]
        s.setGeometryLayer([a], layer)
        self.assertEqual(s.DoF, 0)
        s.setGeometryLayer([a], 0)
        self.assertEqual(s.DoF, 4)
