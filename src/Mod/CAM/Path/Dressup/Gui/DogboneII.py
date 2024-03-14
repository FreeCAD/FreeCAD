# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2022 sliptonic <shopinthewoods@gmail.com>               *
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

from PySide import QtCore
from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD
import FreeCADGui
import Path
import Path.Dressup.DogboneII as DogboneII
import PathScripts.PathUtils as PathUtils

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate


class Marker(object):
    def __init__(self, pt, r, h, ena):
        if Path.Geom.isRoughly(h, 0):
            h = 0.1
        self.pt = pt
        self.r = r
        self.h = h
        self.ena = ena
        self.sep = coin.SoSeparator()
        self.pos = coin.SoTranslation()
        self.pos.translation = (pt.x, pt.y, pt.z + h / 2)
        self.rot = coin.SoRotationXYZ()
        self.rot.axis = self.rot.X
        self.rot.angle = DogboneII.PI / 2
        self.cyl = coin.SoCylinder()
        self.cyl.radius = r
        self.cyl.height = h
        # self.cyl.removePart(self.cyl.TOP)
        # self.cyl.removePart(self.cyl.BOTTOM)
        self.material = coin.SoMaterial()
        self.sep.addChild(self.pos)
        self.sep.addChild(self.rot)
        self.sep.addChild(self.material)
        self.sep.addChild(self.cyl)
        self.lowlight()

    def setSelected(self, selected):
        if selected:
            self.highlight()
        else:
            self.lowlight()

    def highlight(self):
        self.material.diffuseColor = self.color(1)
        self.material.transparency = 0.75

    def lowlight(self):
        self.material.diffuseColor = self.color(0)
        self.material.transparency = 0.90

    def _colorEnabled(self, id):
        if id == 1:
            return coin.SbColor(0.0, 0.9, 0.0)
        return coin.SbColor(0.0, 0.9, 0.0)

    def _colorDisabled(self, id):
        if id == 1:
            return coin.SbColor(0.9, 0.0, 0.0)
        return coin.SbColor(0.9, 0.0, 0.0)

    def color(self, id):
        if self.ena:
            return self._colorEnabled(id)
        return self._colorDisabled(id)


class TaskPanel(object):
    DataIds = QtCore.Qt.ItemDataRole.UserRole
    DataState = QtCore.Qt.ItemDataRole.UserRole + 1

    def __init__(self, viewProvider, obj):
        self.viewProvider = viewProvider
        self.obj = obj
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/DogboneEdit.ui")
        self.s = None
        FreeCAD.ActiveDocument.openTransaction("Edit Dogbone Dress-up")
        # self.height = 10 ???
        self.markers = []

    def reject(self):
        FreeCAD.ActiveDocument.abortTransaction()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.Selection.removeObserver(self.s)
        self.cleanup()

    def accept(self):
        self.getFields()
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.Selection.removeObserver(self.s)
        FreeCAD.ActiveDocument.recompute()
        self.cleanup()

    def cleanup(self):
        self.viewProvider.showMarkers(False)
        for m in self.markers:
            self.viewProvider.switch.removeChild(m.sep)
        self.markers = []

    def getFields(self):
        self.obj.Style = str(self.form.styleCombo.currentText())
        self.obj.Side = str(self.form.sideCombo.currentText())
        self.obj.Incision = str(self.form.incisionCombo.currentText())
        self.obj.Custom = self.form.custom.value()
        blacklist = []
        for i in range(0, self.form.bones.count()):
            item = self.form.bones.item(i)
            if item.checkState() == QtCore.Qt.CheckState.Unchecked:
                blacklist.extend(item.data(self.DataIds))
        self.obj.BoneBlacklist = sorted(blacklist)
        self.obj.Proxy.execute(self.obj)

    def updateBoneList(self):
        itemList = []
        for state in self.obj.Proxy.boneStates(self.obj):
            pos = state.position()
            ids = ",".join([str(nr) for nr in state.boneIDs()])
            lbl = f"({pos.x:.2f}, {pos.y:.2f}): {ids}"
            item = QtGui.QListWidgetItem(lbl)
            if state.isEnabled():
                item.setCheckState(QtCore.Qt.CheckState.Checked)
            else:
                item.setCheckState(QtCore.Qt.CheckState.Unchecked)
            flags = QtCore.Qt.ItemFlag.ItemIsSelectable
            flags |= QtCore.Qt.ItemFlag.ItemIsEnabled
            flags |= QtCore.Qt.ItemFlag.ItemIsUserCheckable
            item.setFlags(flags)
            item.setData(self.DataIds, state.boneIDs())
            item.setData(self.DataState, state)
            itemList.append(item)

        markers = []
        self.form.bones.clear()
        for item in sorted(
            itemList, key=lambda item: item.data(self.DataState).boneIDs()[0]
        ):
            self.form.bones.addItem(item)
            state = item.data(self.DataState)
            loc = state.boneTip()
            r = self.obj.Proxy.toolRadius(self.obj)
            zs = state.zLevels()
            markers.append(
                Marker(
                    FreeCAD.Vector(loc.x, loc.y, min(zs)),
                    r,
                    max(1, max(zs) - min(zs)),
                    state.isEnabled(),
                )
            )
        for m in self.markers:
            self.viewProvider.switch.removeChild(m.sep)
        for m in markers:
            self.viewProvider.switch.addChild(m.sep)
        self.markers = markers

    def updateUI(self):
        customSelected = self.obj.Incision == DogboneII.Incision.Custom
        self.form.custom.setEnabled(customSelected)
        self.form.customLabel.setEnabled(customSelected)
        self.updateBoneList()

    def updateModel(self):
        self.getFields()
        self.updateUI()
        FreeCAD.ActiveDocument.recompute()

    def setupCombo(self, combo, text, items):
        if items and len(items) > 0:
            for i in range(combo.count(), -1, -1):
                combo.removeItem(i)
            combo.addItems(items)
        index = combo.findText(text, QtCore.Qt.MatchFixedString)
        if index >= 0:
            combo.setCurrentIndex(index)

    def setFields(self):
        self.setupCombo(self.form.styleCombo, self.obj.Style, DogboneII.Style.All)
        self.setupCombo(self.form.sideCombo, self.obj.Side, DogboneII.Side.All)
        self.setupCombo(
            self.form.incisionCombo, self.obj.Incision, DogboneII.Incision.All
        )
        self.form.custom.setMinimum(0.0)
        self.form.custom.setDecimals(3)
        self.form.custom.setValue(self.obj.Custom)
        self.updateUI()

    def open(self):
        self.s = SelObserver()
        # install the function mode resident
        FreeCADGui.Selection.addObserver(self.s)

    def setupUi(self):
        self.setFields()
        # now that the form is filled, setup the signal handlers
        self.form.styleCombo.currentIndexChanged.connect(self.updateModel)
        self.form.sideCombo.currentIndexChanged.connect(self.updateModel)
        self.form.incisionCombo.currentIndexChanged.connect(self.updateModel)
        self.form.custom.valueChanged.connect(self.updateModel)
        self.form.bones.itemChanged.connect(self.updateModel)
        self.form.bones.itemSelectionChanged.connect(self.updateMarkers)

        self.viewProvider.showMarkers(True)

    def updateMarkers(self):
        index = self.form.bones.currentRow()
        for i, m in enumerate(self.markers):
            m.setSelected(i == index)


class SelObserver(object):
    def __init__(self):
        import Path.Op.Gui.Selection as PST

        PST.eselect()

    def __del__(self):
        import Path.Op.Gui.Selection as PST

        PST.clear()

    def addSelection(self, doc, obj, sub, pnt):
        FreeCADGui.doCommand(
            "Gui.Selection.addSelection(FreeCAD.ActiveDocument." + obj + ")"
        )
        FreeCADGui.updateGui()


class ViewProviderDressup(object):
    def __init__(self, vobj):
        self.vobj = vobj
        self.obj = None

    def attach(self, vobj):
        self.obj = vobj.Object
        if self.obj and self.obj.Base:
            for i in self.obj.Base.InList:
                if hasattr(i, "Group"):
                    group = i.Group
                    for g in group:
                        if g.Name == self.obj.Base.Name:
                            group.remove(g)
                    i.Group = group
            # FreeCADGui.ActiveDocument.getObject(obj.Base.Name).Visibility = False
        self.switch = coin.SoSwitch()
        vobj.RootNode.addChild(self.switch)

    def showMarkers(self, on):
        sw = coin.SO_SWITCH_ALL if on else coin.SO_SWITCH_NONE
        self.switch.whichChild = sw

    def claimChildren(self):
        return [self.obj.Base]

    def setEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        panel = TaskPanel(self, vobj.Object)
        FreeCADGui.Control.showDialog(panel)
        panel.setupUi()
        return True

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onDelete(self, arg1=None, arg2=None):
        """this makes sure that the base operation is added back to the project and visible"""
        if arg1.Object and arg1.Object.Base:
            FreeCADGui.ActiveDocument.getObject(arg1.Object.Base.Name).Visibility = True
            job = PathUtils.findParentJob(arg1.Object)
            if job:
                job.Proxy.addOperation(arg1.Object.Base, arg1.Object)
            arg1.Object.Base = None
        return True


def Create(base, name="DressupDogbone"):
    """
    Create(obj, name='DressupDogbone') ... dresses the given Path.Op.Profile object with dogbones.
    """
    obj = DogboneII.Create(base, name)
    job = PathUtils.findParentJob(base)
    job.Proxy.addOperation(obj, base)

    if FreeCAD.GuiUp:
        obj.ViewObject.Proxy = ViewProviderDressup(obj.ViewObject)
        obj.Base.ViewObject.Visibility = False

    return obj


class CommandDressupDogboneII(object):
    def GetResources(self):
        return {
            "Pixmap": "CAM_Dressup",
            "MenuText": QT_TRANSLATE_NOOP("CAM_DressupDogbone", "Dogbone"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_DressupDogbone",
                "Creates a Dogbone Dress-up object from a selected toolpath",
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
            FreeCAD.Console.PrintError(
                translate("CAM_DressupDogbone", "Please select one toolpath object") + "\n"
            )
            return
        baseObject = selection[0]
        if not baseObject.isDerivedFrom("Path::Feature"):
            FreeCAD.Console.PrintError(
                translate("CAM_DressupDogbone", "The selected object is not a toolpath")
                + "\n"
            )
            return

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction("Create Dogbone Dress-up")
        FreeCADGui.addModule("Path.Dressup.Gui.DogboneII")
        FreeCADGui.doCommand(
            "Path.Dressup.Gui.DogboneII.Create(FreeCAD.ActiveDocument.%s)"
            % baseObject.Name
        )
        # FreeCAD.ActiveDocument.commitTransaction()  # Final `commitTransaction()` called via TaskPanel.accept()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtGui
    from pivy import coin

    FreeCADGui.addCommand("CAM_DressupDogbone", CommandDressupDogboneII())

FreeCAD.Console.PrintLog("Loading DressupDogboneII ... done\n")
