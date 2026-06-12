# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

import FreeCAD
import TechDrawGui
from PySide import QtCore

from .TechDrawTestUtilities import createPageWithSVGTemplate


class BalloonPositionTest(unittest.TestCase):
    def setUp(self):
        FreeCAD.newDocument("TDBalloonPosition")
        self.doc = FreeCAD.ActiveDocument
        self.doc.addObject("Part::Box", "Box")
        self.page = createPageWithSVGTemplate()
        self.page.ViewObject.show()

        self.view = self.doc.addObject("TechDraw::DrawViewPart", "View")
        self.page.addView(self.view)
        self.view.Source = [self.doc.Box]
        self.view.X = 80
        self.view.Y = 70
        self.doc.recompute()
        QtCore.QCoreApplication.processEvents()

    def tearDown(self):
        FreeCAD.closeDocument("TDBalloonPosition")

    def graphicsItem(self, view):
        scene = TechDrawGui.getSceneForPage(self.page)
        for item in scene.items():
            if item.data(1) == view.Name:
                return item
        self.fail(f"Graphics item for {view.Name} was not found")

    def testBalloonOffsetAppliedOnce(self):
        balloon = self.doc.addObject("TechDraw::DrawViewBalloon", "Balloon")
        balloon.SourceView = self.view
        balloon.OriginX = 10
        balloon.OriginY = 15
        balloon.X = 30
        balloon.Y = 40
        balloon.Text = "1"
        self.page.addView(balloon)

        self.doc.recompute()
        QtCore.QCoreApplication.processEvents()

        item = self.graphicsItem(balloon)
        self.assertIsNotNone(item.parentItem())
        self.assertEqual(item.pos(), QtCore.QPointF(0, 0))


if __name__ == "__main__":
    unittest.main()
