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
"""Provides the TechDraw ShareView Task Dialog."""

__title__ = "TechDrawTools.TaskShareView"
__author__ = "WandererFan"
__url__ = "https://www.freecad.org"
__version__ = "00.01"
__date__ = "2022/01/11"

from PySide import QtCore
import PySide.QtGui as QtGui

import FreeCAD as App
import FreeCADGui as Gui

from TechDrawTools import TDToolsMovers

import os

translate = App.Qt.translate

class TaskShareView:
    def __init__(self):
        self._uiPath = App.getHomePath()
        self._uiPath = os.path.join(self._uiPath, "Mod/TechDraw/TechDrawTools/Gui/TaskMoveView.ui")
        self.form = Gui.PySideUic.loadUi(self._uiPath)

        self.form.setWindowTitle(translate("TechDraw_ShareView", "Share View with another Page"))
        self.form.lViewName.setText(translate("TechDraw_ShareView", "View to share"))

        self.form.pbView.clicked.connect(self.pickView)
        self.form.pbFromPage.clicked.connect(self.pickFromPage)
        self.form.pbToPage.clicked.connect(self.pickToPage)

        self.viewName = ""
        self.fromPageName = ""
        self.toPageName   = ""

        self.dialogOpen = False

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
        if (self.dialogOpen) :
            return
        _dlgPath = App.getHomePath()
        _dlgPath = os.path.join(_dlgPath, "Mod/TechDraw/TechDrawTools/Gui/DlgPageChooser.ui")
        dlg = Gui.PySideUic.loadUi(_dlgPath)
        self.dialogOpen = True
        dlg.lPrompt.setText(translate("TechDraw_ShareView", "Select View to share from list."))
        dlg.setWindowTitle(translate("TechDraw_ShareView", "Select View"))

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
        self.dialogOpen = False

    def pickFromPage(self):
#        print("pickFromPage")
        if (self.dialogOpen) :
            return
        _dlgPath = App.getHomePath()
        _dlgPath = os.path.join(_dlgPath, "Mod/TechDraw/TechDrawTools/Gui/DlgPageChooser.ui")
        dlg = Gui.PySideUic.loadUi(_dlgPath)
        self.dialogOpen = True
        dlg.lPrompt.setText(translate("TechDraw_ShareView", "Select From Page."))
        dlg.setWindowTitle(translate("TechDraw_ShareView", "Select Page"))

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
        self.dialogOpen = False


    def pickToPage(self):
#        print("pickToPage")
        if (self.dialogOpen) :
            return
        _dlgPath = App.getHomePath()
        _dlgPath = os.path.join(_dlgPath, "Mod/TechDraw/TechDrawTools/Gui/DlgPageChooser.ui")
        dlg = Gui.PySideUic.loadUi(_dlgPath)
        self.dialogOpen = True
        dlg.lPrompt.setText(translate("TechDraw_ShareView", "Select To Page."))
        dlg.setWindowTitle(translate("TechDraw_ShareView", "Select Page"))

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
        self.dialogOpen = False

    def setValues(self, viewName, fromPageName, toPageName):
        self.viewName = viewName
        self.form.leView.setText(viewName)
        self.fromPageName = fromPageName
        self.form.leFromPage.setText(fromPageName)
        self.toPageName = toPageName
        self.form.leToPage.setText(toPageName)

