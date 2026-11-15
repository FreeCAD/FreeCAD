# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

import FreeCAD
import FreeCADGui
from PySide import QtWidgets


class TestStartView(unittest.TestCase):
    def setUp(self):
        FreeCADGui.runCommand("Start_Start")
        FreeCADGui.updateGui()
        self.mdiArea = FreeCADGui.getMainWindow().findChild(QtWidgets.QMdiArea)
        self.subWindow = next(
            subWindow
            for subWindow in self.mdiArea.subWindowList()
            if subWindow.widget().objectName() == "StartView"
        )

    def tearDown(self):
        self.subWindow.close()
        FreeCADGui.updateGui()

    def cardsPainted(self):
        painted = [
            cardView.updatesEnabled() and cardView.viewport().updatesEnabled()
            for cardView in self.subWindow.widget().findChildren(QtWidgets.QListView)
            if cardView.metaObject().className() == "StartGui::FileCardView"
        ]
        self.assertTrue(painted)
        return painted

    def testCardsStayPaintedWhenNoWindowIsActive(self):
        self.mdiArea.subWindowActivated.emit(self.subWindow)
        self.mdiArea.subWindowActivated.emit(None)
        self.assertTrue(all(self.cardsPainted()))

    def testCardsStopPaintingBehindAnotherWindow(self):
        doc = FreeCAD.newDocument()
        FreeCADGui.updateGui()
        self.assertFalse(any(self.cardsPainted()))

        FreeCAD.closeDocument(doc.Name)
        FreeCADGui.updateGui()
        self.assertTrue(all(self.cardsPainted()))
