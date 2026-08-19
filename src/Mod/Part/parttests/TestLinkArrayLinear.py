# SPDX-License-Identifier: LGPL-2.1-or-later

import os
import tempfile
import unittest
import xml.etree.ElementTree as ET
import zipfile

import FreeCAD as App
import Part


class TestLinkArrayLinear(unittest.TestCase):
    def setUp(self):
        self.doc = App.newDocument("TestLinkArrayLinear")
        source = self.doc.addObject("Part::Box", "Source")
        self.array = self.doc.addObject("Part::LinkArrayLinear", "Array")
        self.array.LinkedObject = source
        self.array.Occurrences = 3
        self.array.Occurrences2 = 3
        self.doc.recompute()

    def tearDown(self):
        App.closeDocument(self.doc.Name)

    def assertSuppressed(self, indices):
        self.assertEqual(self.array.getStatusString(), "Valid")
        self.assertEqual(
            [i for i, element in enumerate(self.array.ElementList) if element.Suppressed],
            indices,
        )

    def reload(self, legacy=False):
        with tempfile.TemporaryDirectory(prefix="freecad_linear_array_") as directory:
            path = os.path.join(directory, "array.FCStd")
            self.doc.saveAs(path)
            App.closeDocument(self.doc.Name)
            if legacy:
                # Reproduce a file written before coordinate-based suppression existed.
                with zipfile.ZipFile(path) as archive:
                    entries = {name: archive.read(name) for name in archive.namelist()}
                root = ET.fromstring(entries["Document.xml"])
                for properties in root.iter("Properties"):
                    for prop in list(properties):
                        if prop.get("name") in ("SuppressedPositions", "GeneratedOccurrences2"):
                            properties.remove(prop)
                    properties.set("Count", str(len(properties.findall("Property"))))
                entries["Document.xml"] = ET.tostring(root)
                with zipfile.ZipFile(path, "w") as archive:
                    for name, data in entries.items():
                        archive.writestr(name, data)
            self.doc = App.openDocument(path)
            self.array = self.doc.getObject("Array")

    def testSuppressionFollowsBothDirections(self):
        self.array.ElementList[0].Suppressed = True
        self.array.ElementList[5].Suppressed = True  # (1, 2)
        self.array.ElementList[8].Suppressed = True  # (2, 2)
        self.array.Occurrences2 = 5
        self.array.Occurrences = 4
        self.doc.recompute()
        self.assertSuppressed([0, 7, 12])

        self.array.Occurrences = 1
        self.array.Occurrences2 = 2
        self.doc.recompute()
        self.assertSuppressed([0])
        self.array.ElementList[0].Suppressed = False

        self.array.Occurrences = 3
        self.array.Occurrences2 = 3
        self.doc.recompute()
        self.assertSuppressed([5, 8])
        self.array.ElementList[5].Suppressed = False
        self.array.Occurrences2 = 4
        self.doc.recompute()
        self.assertSuppressed([10])

    def testSuppressionSurvivesReloadOutsideGrid(self):
        self.array.ElementList[5].Suppressed = True
        self.array.Occurrences2 = 1
        self.doc.recompute()
        self.reload()
        self.array.Occurrences2 = 4
        self.doc.recompute()
        self.assertSuppressed([6])

    def testLegacySuppressionIsMigrated(self):
        self.array.ElementList[5].Suppressed = True
        self.doc.recompute()
        self.reload(legacy=True)
        self.array.Occurrences2 = 4
        self.doc.recompute()
        self.assertSuppressed([6])

    def testPendingResizeUsesExistingElementCoordinates(self):
        self.array.Occurrences2 = 4
        self.array.ElementList[5].Suppressed = True  # Still the old (1, 2) element.
        self.reload()
        self.doc.recompute()
        self.assertSuppressed([6])

    def testSuppressionSurvivesCollapseAndExpand(self):
        self.array.ElementList[5].Suppressed = True
        self.array.ShowElement = False
        self.doc.recompute()
        self.array.Occurrences2 = 4
        self.doc.recompute()
        self.array.ShowElement = True
        self.doc.recompute()
        self.assertSuppressed([6])

    def testFailedResizeKeepsExistingElementCoordinates(self):
        self.array.Occurrences2 = 4
        self.array.Length2 = 0
        self.doc.recompute()
        self.array.ElementList[5].Suppressed = True
        self.array.Length2 = 100
        self.doc.recompute()
        self.assertSuppressed([6])

    def testResizeUndoRedo(self):
        self.doc.UndoMode = 1
        self.array.ElementList[5].Suppressed = True
        self.doc.recompute()
        self.doc.openTransaction("Resize array")
        self.array.Occurrences2 = 4
        self.doc.recompute()
        self.doc.commitTransaction()
        self.assertSuppressed([6])
        self.doc.undo()
        self.doc.recompute()
        self.assertSuppressed([5])
        self.doc.redo()
        self.doc.recompute()
        self.assertSuppressed([6])

    def testSuppressionUndoRedo(self):
        self.doc.UndoMode = 1
        self.doc.openTransaction("Suppress instance")
        self.array.ElementList[5].Suppressed = True
        self.doc.recompute()
        self.doc.commitTransaction()
        self.doc.undo()
        self.doc.recompute()
        self.assertSuppressed([])
        self.doc.redo()
        self.doc.recompute()
        self.assertSuppressed([5])

