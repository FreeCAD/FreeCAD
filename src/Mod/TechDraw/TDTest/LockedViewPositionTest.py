# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

import FreeCAD
import TechDrawGui
from PySide import QtCore

from .TechDrawTestUtilities import createPageWithSVGTemplate


class LockedViewPositionTest(unittest.TestCase):
    def setUp(self):
        FreeCAD.newDocument("TDLockedPosition")
        self.doc = FreeCAD.ActiveDocument
        self.doc.addObject("Part::Box", "Box")
        self.page = createPageWithSVGTemplate()
        self.page.ViewObject.show()

    def tearDown(self):
        FreeCAD.closeDocument("TDLockedPosition")

    def graphicsItem(self, view):
        scene = TechDrawGui.getSceneForPage(self.page)
        for item in scene.items():
            if item.data(1) == view.Name:
                return item
        self.fail(f"Graphics item for {view.Name} was not found")

    def testLockedViewTracksFeaturePosition(self):
        unlocked = self.doc.addObject("TechDraw::DrawViewPart", "UnlockedView")
        locked = self.doc.addObject("TechDraw::DrawViewPart", "LockedView")
        self.page.addView(unlocked)
        self.page.addView(locked)
        unlocked.Source = [self.doc.Box]
        locked.Source = [self.doc.Box]
        locked.LockPosition = True

        for view in (unlocked, locked):
            view.X = 42
            view.Y = 73

        self.doc.recompute()
        QtCore.QCoreApplication.processEvents()

        self.assertEqual(self.graphicsItem(locked).pos(), self.graphicsItem(unlocked).pos())


if __name__ == "__main__":
    unittest.main()
