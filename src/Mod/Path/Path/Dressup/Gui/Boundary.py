# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
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

from PySide import QtGui
from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD
import FreeCADGui
import Path
import Path.Dressup.Boundary as PathDressupPathBoundary
import PathGui

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


translate = FreeCAD.Qt.translate


class TaskPanel(object):
    def __init__(self, obj, viewProvider):
        self.obj = obj
        self.viewProvider = viewProvider
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/DressupPathBoundary.ui")
        if obj.Stock:
            self.visibilityBoundary = obj.Stock.ViewObject.Visibility
            obj.Stock.ViewObject.Visibility = True
        else:
            self.visibilityBoundary = False

        self.buttonBox = None
        self.isDirty = False

        self.stockFromBase = None
        self.stockFromExisting = None
        self.stockCreateBox = None
        self.stockCreateCylinder = None
        self.stockEdit = None

    def getStandardButtons(self):
        return int(
            QtGui.QDialogButtonBox.Ok
            | QtGui.QDialogButtonBox.Apply
            | QtGui.QDialogButtonBox.Cancel
        )

    def modifyStandardButtons(self, buttonBox):
        self.buttonBox = buttonBox

    def setDirty(self):
        self.isDirty = True
        self.buttonBox.button(QtGui.QDialogButtonBox.Apply).setEnabled(True)

    def setClean(self):
        self.isDirty = False
        self.buttonBox.button(QtGui.QDialogButtonBox.Apply).setEnabled(False)

    def clicked(self, button):
        # callback for standard buttons
        if button == QtGui.QDialogButtonBox.Apply:
            self.updateDressup()
            FreeCAD.ActiveDocument.recompute()

    def abort(self):
        FreeCAD.ActiveDocument.abortTransaction()
        self.cleanup(False)

    def reject(self):
        FreeCAD.ActiveDocument.abortTransaction()
        self.cleanup(True)

    def accept(self):
        if self.isDirty:
            self.updateDressup()
        FreeCAD.ActiveDocument.commitTransaction()
        self.cleanup(True)

    def cleanup(self, gui):
        self.viewProvider.clearTaskPanel()
        if gui:
            FreeCADGui.ActiveDocument.resetEdit()
            FreeCADGui.Control.closeDialog()
            FreeCAD.ActiveDocument.recompute()
            if self.obj.Stock:
                self.obj.Stock.ViewObject.Visibility = self.visibilityBoundary

    def updateDressup(self):
        if self.obj.Inside != self.form.stockInside.isChecked():
            self.obj.Inside = self.form.stockInside.isChecked()
        self.stockEdit.getFields(self.obj)
        self.setClean()

    def updateStockEditor(self, index, force=False):
        import Path.Main.Gui.Job as PathJobGui
        import Path.Main.Stock as PathStock

        def setupFromBaseEdit():
            Path.Log.track(index, force)
            if force or not self.stockFromBase:
                self.stockFromBase = PathJobGui.StockFromBaseBoundBoxEdit(
                    self.obj, self.form, force
                )
            self.stockEdit = self.stockFromBase

        def setupCreateBoxEdit():
            Path.Log.track(index, force)
            if force or not self.stockCreateBox:
                self.stockCreateBox = PathJobGui.StockCreateBoxEdit(
                    self.obj, self.form, force
                )
            self.stockEdit = self.stockCreateBox

        def setupCreateCylinderEdit():
            Path.Log.track(index, force)
            if force or not self.stockCreateCylinder:
                self.stockCreateCylinder = PathJobGui.StockCreateCylinderEdit(
                    self.obj, self.form, force
                )
            self.stockEdit = self.stockCreateCylinder

        def setupFromExisting():
            Path.Log.track(index, force)
            if force or not self.stockFromExisting:
                self.stockFromExisting = PathJobGui.StockFromExistingEdit(
                    self.obj, self.form, force
                )
            if self.stockFromExisting.candidates(self.obj):
                self.stockEdit = self.stockFromExisting
                return True
            return False

        if index == -1:
            if self.obj.Stock is None or PathJobGui.StockFromBaseBoundBoxEdit.IsStock(
                self.obj
            ):
                setupFromBaseEdit()
            elif PathJobGui.StockCreateBoxEdit.IsStock(self.obj):
                setupCreateBoxEdit()
            elif PathJobGui.StockCreateCylinderEdit.IsStock(self.obj):
                setupCreateCylinderEdit()
            elif PathJobGui.StockFromExistingEdit.IsStock(self.obj):
                setupFromExisting()
            else:
                Path.Log.error(
                    translate("PathJob", "Unsupported stock object %s")
                    % self.obj.Stock.Label
                )
        else:
            if index == PathJobGui.StockFromBaseBoundBoxEdit.Index:
                setupFromBaseEdit()
            elif index == PathJobGui.StockCreateBoxEdit.Index:
                setupCreateBoxEdit()
            elif index == PathJobGui.StockCreateCylinderEdit.Index:
                setupCreateCylinderEdit()
            elif index == PathJobGui.StockFromExistingEdit.Index:
                if not setupFromExisting():
                    setupFromBaseEdit()
                    index = -1
            else:
                Path.Log.error(
                    translate("PathJob", "Unsupported stock type %s (%d)")
                    % (self.form.stock.currentText(), index)
                )
        self.stockEdit.activate(self.obj, index == -1)

    def setupUi(self):
        self.updateStockEditor(-1, False)
        self.form.stockInside.setChecked(self.obj.Inside)

        self.form.stock.currentIndexChanged.connect(self.updateStockEditor)
        self.form.stockInside.stateChanged.connect(self.setDirty)
        self.form.stockExtXneg.textChanged.connect(self.setDirty)
        self.form.stockExtXpos.textChanged.connect(self.setDirty)
        self.form.stockExtYneg.textChanged.connect(self.setDirty)
        self.form.stockExtYpos.textChanged.connect(self.setDirty)
        self.form.stockExtZneg.textChanged.connect(self.setDirty)
        self.form.stockExtZpos.textChanged.connect(self.setDirty)
        self.form.stockBoxLength.textChanged.connect(self.setDirty)
        self.form.stockBoxWidth.textChanged.connect(self.setDirty)
        self.form.stockBoxHeight.textChanged.connect(self.setDirty)
        self.form.stockCylinderRadius.textChanged.connect(self.setDirty)
        self.form.stockCylinderHeight.textChanged.connect(self.setDirty)


class DressupPathBoundaryViewProvider(object):
    def __init__(self, vobj):
        self.attach(vobj)

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def attach(self, vobj):
        self.vobj = vobj
        self.obj = vobj.Object
        self.panel = None

    def claimChildren(self):
        return [self.obj.Base, self.obj.Stock]

    def onDelete(self, vobj, args=None):
        if vobj.Object and vobj.Object.Proxy:
            vobj.Object.Proxy.onDelete(vobj.Object, args)
        return True

    def setEdit(self, vobj, mode=0):
        panel = TaskPanel(vobj.Object, self)
        self.setupTaskPanel(panel)
        return True

    def unsetEdit(self, vobj, mode=0):
        if self.panel:
            self.panel.abort()

    def setupTaskPanel(self, panel):
        self.panel = panel
        FreeCADGui.Control.closeDialog()
        FreeCADGui.Control.showDialog(panel)
        panel.setupUi()

    def clearTaskPanel(self):
        self.panel = None


def Create(base, name="DressupPathBoundary"):
    FreeCAD.ActiveDocument.openTransaction("Create a Boundary dressup")
    obj = PathDressupPathBoundary.Create(base, name)
    obj.ViewObject.Proxy = DressupPathBoundaryViewProvider(obj.ViewObject)
    obj.Base.ViewObject.Visibility = False
    obj.Stock.ViewObject.Visibility = False
    FreeCAD.ActiveDocument.commitTransaction()
    obj.ViewObject.Document.setEdit(obj.ViewObject, 0)
    return obj


class CommandPathDressupPathBoundary:
    def GetResources(self):
        return {
            "Pixmap": "Path_Dressup",
            "MenuText": QT_TRANSLATE_NOOP("Path_DressupPathBoundary", "Boundary"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "Path_DressupPathBoundary",
                "Creates a Path Boundary Dress-up from a selected path",
            ),
        }

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                    return True
        return False

    def Activated(self):
        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) != 1:
            Path.Log.error(
                translate("Path_DressupPathBoundary", "Please select one path object")
                + "\n"
            )
            return
        baseObject = selection[0]

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction("Create Path Boundary Dress-up")
        FreeCADGui.addModule("Path.Dressup.Gui.Boundary")
        FreeCADGui.doCommand(
            "Path.Dressup.Gui.Boundary.Create(App.ActiveDocument.%s)" % baseObject.Name
        )
        # FreeCAD.ActiveDocument.commitTransaction()  # Final `commitTransaction()` called via TaskPanel.accept()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("Path_DressupPathBoundary", CommandPathDressupPathBoundary())

Path.Log.notice("Loading PathDressupPathBoundaryGui... done\n")
