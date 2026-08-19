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
        """Counterpart of testIsPreviewFreshTracksInvalidation, which makes the same
        change without the override and does go stale."""
        self.object.updatePreview()
        self.assertTrue(self.object.isPreviewFresh())

        self.object.Size = 20.0

        self.assertTrue(self.object.isPreviewFresh())


def _coordinateCount(node):
    """Number of points in the node's SoCoordinate3.

    coords is a plain child rather than a registered field, so it is reached by
    searching the scene graph. A freshly-constructed SoCoordinate3 already
    reports 1, so an untessellated node counts 1 rather than 0.
    """
    from pivy import coin

    search = coin.SoSearchAction()
    search.setType(coin.SoCoordinate3.getClassTypeId())
    search.apply(node)
    return search.getPath().getTail().point.getNum()


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

        self.assertGreaterEqual(_coordinateCount(node), 8)

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
    """Covers addExtension() called after the view provider has already attached,
    which leaves extensionAttach() unrun and the preview nodes legitimately null.
    """

    def setUp(self):
        self.document = FreeCAD.newDocument("PreviewNodeBeforeAttachTest")
        self.object = self.document.addObject("Part::FeaturePython", "Feature")
        self.feature = PreviewFeature(self.object)
        BarePreviewViewProvider(self.object.ViewObject)
        self.document.recompute()
        self.object.ViewObject.addExtension("PartGui::ViewProviderPreviewExtensionPython")

    def tearDown(self):
        FreeCAD.closeDocument(self.document.Name)

    def testPreviewRootNodeRaisesInsteadOfCrashing(self):
        # ExtensionContainerPy::getCustomAttributes() replaces the getter's
        # RuntimeError with AttributeError; what matters is that accessing the
        # node raises at all rather than segfaulting.
        with self.assertRaises(AttributeError):
            self.object.ViewObject.PreviewRootNode
