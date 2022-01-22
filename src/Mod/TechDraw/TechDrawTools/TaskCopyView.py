# ***************************************************************************
# *   Copyright (c) 2022 Wanderer Fan <wandererfan@gmail.com>               *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides the TechDraw CopyView Task Dialog."""

__title__ = "TechDrawTools.TaskCopyView"
__author__ = "WandererFan"
__url__ = "https://www.freecadweb.org"
__version__ = "00.01"
__date__ = "2022/01/11"

from PySide.QtCore import QT_TRANSLATE_NOOP
from PySide import QtCore
import PySide.QtGui as QtGui

import FreeCAD as App
import FreeCADGui as Gui

from TechDrawTools import TDToolsMovers

import os

class TaskCopyView:
    def __init__(self):
        self._uiPath = App.getHomePath()
        self._uiPath = os.path.join(self._uiPath, "Mod/TechDraw/Gui/TaskMoveView.ui")
        self.form = Gui.PySideUic.loadUi(self._uiPath)

        self.form.setWindowTitle(QT_TRANSLATE_NOOP("CopyView", "Copy View to a second Page"))

        self.form.pbView.clicked.connect(self.pickView)
        self.form.pbFromPage.clicked.connect(self.pickFromPage)
        self.form.pbToPage.clicked.connect(self.pickToPage)

        self.viewName = ""
        self.fromPageName = ""
        self.toPageName   = ""

    def accept(self):
#        print ("Accept")
        view = App.ActiveDocument.getObject(self.viewName)
        fromPage = App.ActiveDocument.getObject(self.fromPageName)
        toPage = App.ActiveDocument.getObject(self.toPageName)
        TDToolsMovers.moveView(view, fromPage, toPage, True)
        return True

    def reject(self):
#        print ("Reject")
        return True

    def pickView(self):
#        print("pickView")
        _dlgPath = App.getHomePath()
        _dlgPath = os.path.join(_dlgPath, "Mod/TechDraw/Gui/DlgPageChooser.ui")
        dlg = Gui.PySideUic.loadUi(_dlgPath)
        dlg.lPrompt.setText(QT_TRANSLATE_NOOP("CopyView", "Select View to copy from list."))
        dlg.setWindowTitle(QT_TRANSLATE_NOOP("CopyView", "Select View"))

        views = [x for x in App.ActiveDocument.Objects if x.isDerivedFrom("TechDraw::DrawView")]
        for v in views:
            s = v.Label + " / " + v.Name
            item = QtGui.QListWidgetItem(s, dlg.lwPages)
            item.setData(QtCore.Qt.UserRole, v.Name)
        if (dlg.exec() == QtGui.QDialog.Accepted) :
            if dlg.lwPages.selectedItems():
                selItem = dlg.lwPages.selectedItems()[0]
                self.viewName = selItem.data(QtCore.Qt.UserRole)
                self.form.leView.setText(self.viewName)

    def pickFromPage(self):
#        print("pickFromPage")
        _dlgPath = App.getHomePath()
        _dlgPath = os.path.join(_dlgPath, "Mod/TechDraw/Gui/DlgPageChooser.ui")
        dlg = Gui.PySideUic.loadUi(_dlgPath)
        dlg.lPrompt.setText(QT_TRANSLATE_NOOP("CopyView", "Select From Page."))
        dlg.setWindowTitle(QT_TRANSLATE_NOOP("CopyView", "Select Page"))

        pages = [x for x in App.ActiveDocument.Objects if x.isDerivedFrom("TechDraw::DrawPage")]
        for p in pages:
            s = p.Label + " / " + p.Name
            item = QtGui.QListWidgetItem(s, dlg.lwPages)
            item.setData(QtCore.Qt.UserRole, p.Name)
        if (dlg.exec() == QtGui.QDialog.Accepted) :
            if dlg.lwPages.selectedItems():
                selItem = dlg.lwPages.selectedItems()[0]
                self.fromPageName = selItem.data(QtCore.Qt.UserRole)
                self.form.leFromPage.setText(self.fromPageName)

    def pickToPage(self):
#        print("pickToPage")
        _dlgPath = App.getHomePath()
        _dlgPath = os.path.join(_dlgPath, "Mod/TechDraw/Gui/DlgPageChooser.ui")
        dlg = Gui.PySideUic.loadUi(_dlgPath)
        dlg.lPrompt.setText(QT_TRANSLATE_NOOP("CopyView", "Select To Page."))
        dlg.setWindowTitle(QT_TRANSLATE_NOOP("CopyView", "Select Page"))

        pages = [x for x in App.ActiveDocument.Objects if x.isDerivedFrom("TechDraw::DrawPage")]
        for p in pages:
            s = p.Label + " / " + p.Name
            item = QtGui.QListWidgetItem(s, dlg.lwPages)
            item.setData(QtCore.Qt.UserRole, p.Name)
        if (dlg.exec() == QtGui.QDialog.Accepted) :
            if dlg.lwPages.selectedItems():
                selItem = dlg.lwPages.selectedItems()[0]
                self.toPageName = selItem.data(QtCore.Qt.UserRole)
                self.form.leToPage.setText(self.toPageName)

    def setValues(self, viewName, fromPageName, toPageName):
        self.viewName = viewName
        self.form.leView.setText(viewName)
        self.fromPageName = fromPageName
        self.form.leFromPage.setText(fromPageName)
        self.toPageName = toPageName
        self.form.leToPage.setText(toPageName)

