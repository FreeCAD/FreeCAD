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
