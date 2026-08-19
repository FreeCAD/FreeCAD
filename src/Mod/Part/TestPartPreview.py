# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

import FreeCAD
import Part


class PreviewFeature:
    """Minimal Python feature carrying the preview extension."""

    def __init__(self, obj):
        obj.Proxy = self
        obj.addProperty("App::PropertyLength", "Size", "Test", "Drives the preview")
        obj.Size = 10.0
        if not obj.hasExtension("Part::PreviewExtensionPython"):
            obj.addExtension("Part::PreviewExtensionPython")
        self.recomputePreviewCalls = 0

    def recomputePreview(self, ext):
        self.recomputePreviewCalls += 1
        obj = ext.ExtendedObject
        obj.PreviewShape = Part.makeBox(obj.Size, obj.Size, obj.Size)

    def execute(self, obj):
        obj.Shape = Part.makeBox(obj.Size, obj.Size, obj.Size)


class TestPreviewExtensionPython(unittest.TestCase):
    def setUp(self):
        self.document = FreeCAD.newDocument("PreviewExtensionTest")
        self.object = self.document.addObject("Part::FeaturePython", "Feature")
        self.feature = PreviewFeature(self.object)
        self.document.recompute()

    def tearDown(self):
        FreeCAD.closeDocument(self.document.Name)

    def testRecomputePreviewProxyIsCalled(self):
        self.object.invalidatePreview()
        self.object.updatePreview()

        self.assertGreaterEqual(self.feature.recomputePreviewCalls, 1)
        self.assertFalse(self.object.PreviewShape.isNull())

    def testUpdatePreviewIsNoOpWhileFresh(self):
        self.object.invalidatePreview()
        self.object.updatePreview()
        callsAfterFirst = self.feature.recomputePreviewCalls

        self.object.updatePreview()

        self.assertEqual(self.feature.recomputePreviewCalls, callsAfterFirst)

    def testInvalidatePreviewForcesRecompute(self):
        self.object.updatePreview()
        callsBefore = self.feature.recomputePreviewCalls

        self.object.invalidatePreview()
        self.object.updatePreview()

        self.assertEqual(self.feature.recomputePreviewCalls, callsBefore + 1)

    def testIsPreviewFreshTracksInvalidation(self):
        self.object.updatePreview()
        self.assertTrue(self.object.isPreviewFresh())

        self.object.Size = 20.0

        self.assertFalse(self.object.isPreviewFresh())


class FreshPreviewFeature(PreviewFeature):
    """Feature that declares its preview never needs recomputing."""

    def mustRecomputePreview(self, ext):
        return False


class TestMustRecomputePreviewRouting(unittest.TestCase):
    def setUp(self):
        self.document = FreeCAD.newDocument("MustRecomputePreviewTest")
        self.object = self.document.addObject("Part::FeaturePython", "Feature")
        self.feature = FreshPreviewFeature(self.object)
        self.document.recompute()

    def tearDown(self):
        FreeCAD.closeDocument(self.document.Name)

    def testReturningFalseKeepsPreviewFresh(self):
        self.object.updatePreview()
        self.assertTrue(self.object.isPreviewFresh())

        # touch() alone never fires extensionOnChanged; assigning PreviewShape
        # is the only property write that skips the isInputProp branch, so
        # mustRecomputePreview() alone decides the outcome below.
        self.object.touch()
        self.object.PreviewShape = Part.makeBox(1, 1, 1)

        self.assertTrue(self.object.isPreviewFresh())


class TestMustRecomputePreviewDefault(unittest.TestCase):
    def setUp(self):
        self.document = FreeCAD.newDocument("MustRecomputePreviewDefaultTest")
        self.object = self.document.addObject("Part::FeaturePython", "Feature")
        self.feature = PreviewFeature(self.object)
        self.document.recompute()

    def tearDown(self):
        FreeCAD.closeDocument(self.document.Name)

    def testTouchThenPreviewShapeChangeInvalidatesByDefault(self):
        # Baseline for testReturningFalseKeepsPreviewFresh above: with no
        # Python override, the same trigger genuinely invalidates, so that
        # test is not vacuous.
        self.object.updatePreview()
        self.assertTrue(self.object.isPreviewFresh())

        self.object.touch()
        self.object.PreviewShape = Part.makeBox(1, 1, 1)

        self.assertFalse(self.object.isPreviewFresh())


class PreviewViewProvider:
    """Minimal Python view provider carrying the preview view extension."""

    def __init__(self, vobj):
        vobj.Proxy = self

    def attach(self, vobj):
        self.ViewObject = vobj
        if not vobj.hasExtension("PartGui::ViewProviderPreviewExtensionPython"):
            vobj.addExtension("PartGui::ViewProviderPreviewExtensionPython")


class TestPreviewNodeAccess(unittest.TestCase):
    def setUp(self):
        self.document = FreeCAD.newDocument("PreviewNodeTest")
        self.object = self.document.addObject("Part::FeaturePython", "Feature")
        self.feature = PreviewFeature(self.object)
        PreviewViewProvider(self.object.ViewObject)
        self.document.recompute()

    def tearDown(self):
        FreeCAD.closeDocument(self.document.Name)

    def testPreviewRootNodeIsASeparator(self):
        from pivy import coin

        root = self.object.ViewObject.PreviewRootNode

        self.assertIsNotNone(root)
        self.assertTrue(root.isOfType(coin.SoSeparator.getClassTypeId()))

    def testPreviewShapeNodeIsAPreviewShape(self):
        from pivy import coin

        node = self.object.ViewObject.PreviewShapeNode

        self.assertIsNotNone(node)
        self.assertTrue(node.isOfType(coin.SoType.fromName("SoPreviewShape")))

    def testDefaultShapeNodeIsChildOfRoot(self):
        root = self.object.ViewObject.PreviewRootNode

        self.assertEqual(root.getNumChildren(), 1)
        self.assertEqual(root.getChild(0), self.object.ViewObject.PreviewShapeNode)

    def testUpdatePreviewShapePopulatesGeometry(self):
        from pivy import coin

        node = coin.SoType.fromName("SoPreviewShape").createInstance()

        self.object.ViewObject.updatePreviewShape(Part.makeBox(5, 5, 5), node)

        # coords is a plain child, not a registered field, so it is not
        # reachable as node.coords; search the scene graph for it instead.
        search = coin.SoSearchAction()
        search.setType(coin.SoCoordinate3.getClassTypeId())
        search.apply(node)
        coordinates = search.getPath().getTail()

        # A freshly-constructed SoCoordinate3 already reports getNum() == 1
        # (Coin's default single-value state), so > 0 would pass even with no
        # tessellation performed; a tessellated box has at least its 8 corners.
        self.assertGreaterEqual(coordinates.point.getNum(), 8)

    def testUpdatePreviewShapeRejectsWrongNodeType(self):
        from pivy import coin

        with self.assertRaises(TypeError):
            self.object.ViewObject.updatePreviewShape(Part.makeBox(5, 5, 5), coin.SoSeparator())

    def testUpdatePreviewShapeRejectsNonCoinArgument(self):
        with self.assertRaises(TypeError):
            self.object.ViewObject.updatePreviewShape(Part.makeBox(5, 5, 5), object())

    def testShowPreviewAttachesAndDetachesRoot(self):
        annotation = self.object.ViewObject.Annotation
        root = self.object.ViewObject.PreviewRootNode

        self.object.ViewObject.showPreview(True)
        self.assertGreaterEqual(annotation.findChild(root), 0)

        self.object.ViewObject.showPreview(False)
        self.assertLess(annotation.findChild(root), 0)


class BarePreviewViewProvider:
    """View provider that attaches without adding the preview extension."""

    def __init__(self, vobj):
        vobj.Proxy = self

    def attach(self, vobj):
        self.ViewObject = vobj


class TestPreviewNodeAccessBeforeAttach(unittest.TestCase):
    """Covers addExtension() called after the view provider has already
    attached, per api-corrections.md: registerExtension() alone does not run
    extensionAttach(), so the preview nodes can legitimately still be null.
    """

    def setUp(self):
        self.document = FreeCAD.newDocument("PreviewNodeBeforeAttachTest")
        self.object = self.document.addObject("Part::FeaturePython", "Feature")
        self.feature = PreviewFeature(self.object)
        BarePreviewViewProvider(self.object.ViewObject)
        self.document.recompute()
        # addExtension() from outside attach() - the pattern api-corrections.md
        # shows for the App-side extension - leaves extensionAttach() unrun.
        self.object.ViewObject.addExtension("PartGui::ViewProviderPreviewExtensionPython")

    def tearDown(self):
        FreeCAD.closeDocument(self.document.Name)

    def testPreviewRootNodeRaisesInsteadOfCrashing(self):
        # The getter raises Py::RuntimeError, but vobj.PreviewRootNode reaches
        # it through ExtensionContainerPy::getCustomAttributes(), which clears
        # any exception from a failed lookup and reports AttributeError instead
        # (shared framework behaviour, not specific to this extension). What
        # matters here is that it raises at all instead of segfaulting.
        with self.assertRaises(AttributeError):
            self.object.ViewObject.PreviewRootNode
