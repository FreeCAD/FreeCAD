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


class TwoNodeViewProvider(PreviewViewProvider):
    """Reproduces the PartDesign FeatureAddSub arrangement: a primary preview
    node plus a second, more transparent 'tool' node sharing its colour."""

    def attachPreview(self, vext):
        from pivy import coin

        self.toolNode = coin.SoType.fromName("SoPreviewShape").createInstance()
        self.toolNode.color.connectFrom(vext.PreviewShapeNode.color)
        self.toolNode.transparency.setValue(0.95)
        vext.PreviewRootNode.addChild(self.toolNode)

    def updatePreview(self, vext):
        self.updatePreviewCalls = getattr(self, "updatePreviewCalls", 0) + 1
        vext.updatePreviewShape(Part.makeBox(3, 3, 3), self.toolNode)


class InstancingViewProvider(PreviewViewProvider):
    """Reproduces the PartDesign ViewProviderTransformed arrangement: the default
    node is discarded and one shared shape node is instanced under N transforms."""

    instanceCount = 3

    def attachPreview(self, vext):
        from pivy import coin

        self.sharedNode = coin.SoType.fromName("SoPreviewShape").createInstance()
        # removeAllChildren() below drops every scene graph parent, so without an
        # explicit ref Coin frees the node between updatePreview() calls.
        self.sharedNode.ref()

    def updatePreview(self, vext):
        from pivy import coin

        vext.updatePreviewShape(Part.makeBox(2, 2, 2), self.sharedNode)

        vext.PreviewRootNode.removeAllChildren()
        for index in range(self.instanceCount):
            transform = coin.SoTransform()
            transform.translation.setValue(index * 10.0, 0.0, 0.0)

            separator = coin.SoSeparator()
            separator.addChild(transform)
            separator.addChild(self.sharedNode)

            vext.PreviewRootNode.addChild(separator)


class ReplacingShapeViewProvider(PreviewViewProvider):
    """Overrides the shape the preview is built from."""

    def getPreviewShape(self, vext):
        return Part.makeSphere(4)


def _makePreviewObject(document, viewProviderClass):
    obj = document.addObject("Part::FeaturePython", "Feature")
    feature = PreviewFeature(obj)
    viewProviderClass(obj.ViewObject)
    document.recompute()
    return obj, feature


class TestPreviewProxyHooks(unittest.TestCase):
    def setUp(self):
        self.document = FreeCAD.newDocument("PreviewHookTest")

    def tearDown(self):
        FreeCAD.closeDocument(self.document.Name)

    def testAttachPreviewHookCanAddNodes(self):
        obj, _ = _makePreviewObject(self.document, TwoNodeViewProvider)

        # default node plus the tool node the hook added
        self.assertEqual(obj.ViewObject.PreviewRootNode.getNumChildren(), 2)

    def testTwoNodesCarryIndependentTransparency(self):
        obj, _ = _makePreviewObject(self.document, TwoNodeViewProvider)

        toolNode = obj.ViewObject.PreviewRootNode.getChild(1)

        self.assertAlmostEqual(toolNode.transparency.getValue(), 0.95, places=5)
        self.assertNotAlmostEqual(
            obj.ViewObject.PreviewShapeNode.transparency.getValue(), 0.95, places=5
        )

    def testTwoNodesShareColourThroughConnectFrom(self):
        obj, _ = _makePreviewObject(self.document, TwoNodeViewProvider)

        toolNode = obj.ViewObject.PreviewRootNode.getChild(1)
        obj.ViewObject.PreviewShapeNode.color.setValue(0.25, 0.5, 0.75)

        self.assertEqual(toolNode.color.getValue().getValue(), (0.25, 0.5, 0.75))

    def testUpdatePreviewHookIsCalledWhenPreviewShapeChanges(self):
        obj, _ = _makePreviewObject(self.document, TwoNodeViewProvider)
        callsBefore = getattr(obj.ViewObject.Proxy, "updatePreviewCalls", 0)

        obj.PreviewShape = Part.makeBox(7, 7, 7)

        self.assertGreater(obj.ViewObject.Proxy.updatePreviewCalls, callsBefore)

    def testInstancingArrangementReplacesDefaultNode(self):
        obj, _ = _makePreviewObject(self.document, InstancingViewProvider)

        obj.ViewObject.updatePreview()
        root = obj.ViewObject.PreviewRootNode

        self.assertEqual(root.getNumChildren(), InstancingViewProvider.instanceCount)
        # the shape node is genuinely shared, not copied
        self.assertEqual(root.getChild(0).getChild(1), root.getChild(1).getChild(1))

    def testInstanceTransformsAreDistinct(self):
        obj, _ = _makePreviewObject(self.document, InstancingViewProvider)

        obj.ViewObject.updatePreview()
        root = obj.ViewObject.PreviewRootNode

        translations = [
            root.getChild(index).getChild(0).translation.getValue().getValue()
            for index in range(InstancingViewProvider.instanceCount)
        ]

        self.assertEqual(translations[0], (0.0, 0.0, 0.0))
        self.assertEqual(translations[1], (10.0, 0.0, 0.0))
        self.assertEqual(translations[2], (20.0, 0.0, 0.0))

    def testRepeatedUpdatePreviewDoesNotAccumulateChildren(self):
        obj, _ = _makePreviewObject(self.document, InstancingViewProvider)

        obj.ViewObject.updatePreview()
        obj.ViewObject.updatePreview()
        obj.ViewObject.updatePreview()

        self.assertEqual(
            obj.ViewObject.PreviewRootNode.getNumChildren(), InstancingViewProvider.instanceCount
        )

    def testGetPreviewShapeHookReplacesPreviewShapeProperty(self):
        obj, _ = _makePreviewObject(self.document, ReplacingShapeViewProvider)

        obj.PreviewShape = Part.makeBox(1, 1, 1)
        obj.ViewObject.updatePreview()

        # a unit box tessellates to 32 points, a sphere of radius 4 to thousands
        self.assertGreater(_coordinateCount(obj.ViewObject.PreviewShapeNode), 100)

    def testSchedulerCoalescesBatchedUpdatesIntoOneRecompute(self):
        import FreeCADGui

        obj, feature = _makePreviewObject(self.document, TwoNodeViewProvider)
        obj.ViewObject.showPreview(True)

        callsBefore = feature.recomputePreviewCalls
        for size in (11.0, 12.0, 13.0, 14.0):
            obj.Size = size

        FreeCADGui.updateGui()  # drains the scheduler

        recomputes = feature.recomputePreviewCalls - callsBefore
        self.assertEqual(recomputes, 1)


class TestExtensionRegisteredNameUnchanged(unittest.TestCase):
    """Workbenches look the extension up by this exact literal string, so the
    registered type name must survive changes to the class hierarchy."""

    def testHasExtensionByLiteralName(self):
        document = FreeCAD.newDocument("ExtensionNameCheckTest")
        obj = document.addObject("Part::FeaturePython", "Feature")
        PreviewViewProvider(obj.ViewObject)
        document.recompute()

        self.assertTrue(obj.ViewObject.hasExtension("PartGui::ViewProviderPreviewExtensionPython"))

        FreeCAD.closeDocument(document.Name)
