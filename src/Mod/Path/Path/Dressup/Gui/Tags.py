# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
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

from PySide import QtCore, QtGui
from PySide.QtCore import QT_TRANSLATE_NOOP
from pivy import coin
import FreeCAD
import FreeCADGui
import Path
import Path.Base.Gui.GetPoint as PathGetPoint
import Path.Dressup.Tags as PathDressupTag
import PathGui
import PathScripts.PathUtils as PathUtils


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate


def addDebugDisplay():
    return Path.Log.getLevel(Path.Log.thisModule()) == Path.Log.Level.DEBUG


class PathDressupTagTaskPanel:
    DataX = QtCore.Qt.ItemDataRole.UserRole
    DataY = QtCore.Qt.ItemDataRole.UserRole + 1
    DataZ = QtCore.Qt.ItemDataRole.UserRole + 2
    DataID = QtCore.Qt.ItemDataRole.UserRole + 3

    def __init__(self, obj, viewProvider, jvoVisibility=None):
        self.obj = obj
        self.obj.Proxy.obj = obj
        self.viewProvider = viewProvider
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/HoldingTagsEdit.ui")
        self.getPoint = PathGetPoint.TaskPanel(self.form.removeEditAddGroup, True)
        self.jvo = PathUtils.findParentJob(obj).ViewObject
        if jvoVisibility is None:
            FreeCAD.ActiveDocument.openTransaction("Edit HoldingTags Dress-up")
            self.jvoVisible = self.jvo.isVisible()
            if self.jvoVisible:
                self.jvo.hide()
        else:
            self.jvoVisible = jvoVisibility
        self.pt = FreeCAD.Vector(0, 0, 0)

        self.isDirty = True
        self.buttonBox = None
        self.tags = None
        self.Positions = None
        self.Disabled = None
        self.editItem = None

    def getStandardButtons(self):
        return int(
            QtGui.QDialogButtonBox.Ok
            | QtGui.QDialogButtonBox.Apply
            | QtGui.QDialogButtonBox.Cancel
        )

    def clicked(self, button):
        if button == QtGui.QDialogButtonBox.Apply:
            self.getFields()
            self.obj.Proxy.execute(self.obj)
            self.isDirty = False

    def modifyStandardButtons(self, buttonBox):
        self.buttonBox = buttonBox
        self.getPoint.buttonBox = buttonBox

    def abort(self):
        FreeCAD.ActiveDocument.abortTransaction()
        self.cleanup(False)

    def reject(self):
        FreeCAD.ActiveDocument.abortTransaction()
        self.cleanup(True)

    def accept(self):
        FreeCAD.ActiveDocument.commitTransaction()
        self.cleanup(True)
        if self.isDirty:
            self.getFields()
            FreeCAD.ActiveDocument.recompute()

    def cleanup(self, gui):
        self.viewProvider.clearTaskPanel()
        if gui:
            FreeCADGui.ActiveDocument.resetEdit()
            FreeCADGui.Control.closeDialog()
            FreeCAD.ActiveDocument.recompute()
            if self.jvoVisible:
                self.jvo.show()

    def getTags(self, includeCurrent):
        tags = []
        index = self.form.lwTags.currentRow()
        for i in range(0, self.form.lwTags.count()):
            item = self.form.lwTags.item(i)
            enabled = item.checkState() == QtCore.Qt.CheckState.Checked
            x = item.data(self.DataX)
            y = item.data(self.DataY)
            # print("(%.2f, %.2f) i=%d/%s" % (x, y, i, index))
            if includeCurrent or i != index:
                tags.append((x, y, enabled))
        return tags

    def getFields(self):
        width = FreeCAD.Units.Quantity(self.form.ifWidth.text()).Value
        height = FreeCAD.Units.Quantity(self.form.ifHeight.text()).Value
        angle = self.form.dsbAngle.value()
        radius = FreeCAD.Units.Quantity(self.form.ifRadius.text()).Value

        tags = self.getTags(True)
        positions = []
        disabled = []
        for i, (x, y, enabled) in enumerate(tags):
            positions.append(FreeCAD.Vector(x, y, 0))
            if not enabled:
                disabled.append(i)

        if width != self.obj.Width:
            self.obj.Width = width
            self.isDirty = True
        if height != self.obj.Height:
            self.obj.Height = height
            self.isDirty = True
        if angle != self.obj.Angle:
            self.obj.Angle = angle
            self.isDirty = True
        if radius != self.obj.Radius:
            self.obj.Radius = radius
            self.isDirty = True
        if positions != self.obj.Positions:
            self.obj.Positions = positions
            self.isDirty = True
        if disabled != self.obj.Disabled:
            self.obj.Disabled = disabled
            self.isDirty = True

    def updateTagsView(self):
        Path.Log.track()
        self.form.lwTags.blockSignals(True)
        self.form.lwTags.clear()
        for i, pos in enumerate(self.Positions):
            lbl = "%d: (%.2f, %.2f)" % (i, pos.x, pos.y)
            item = QtGui.QListWidgetItem(lbl)
            item.setData(self.DataX, pos.x)
            item.setData(self.DataY, pos.y)
            item.setData(self.DataZ, pos.z)
            item.setData(self.DataID, i)
            if i in self.Disabled:
                item.setCheckState(QtCore.Qt.CheckState.Unchecked)
            else:
                item.setCheckState(QtCore.Qt.CheckState.Checked)
            flags = QtCore.Qt.ItemFlag.ItemIsSelectable
            flags |= QtCore.Qt.ItemFlag.ItemIsEnabled
            flags |= QtCore.Qt.ItemFlag.ItemIsUserCheckable
            item.setFlags(flags)
            self.form.lwTags.addItem(item)
        self.form.lwTags.blockSignals(False)
        self.whenTagSelectionChanged()
        self.viewProvider.updatePositions(self.Positions, self.Disabled)

    def generateNewTags(self):
        count = self.form.sbCount.value()
        Path.Log.track(count)
        if not self.obj.Proxy.generateTags(self.obj, count):
            self.obj.Proxy.execute(self.obj)
        self.Positions = self.obj.Positions
        self.Disabled = self.obj.Disabled
        self.updateTagsView()

    def copyNewTags(self):
        sel = self.form.cbSource.currentText()
        tags = [o for o in FreeCAD.ActiveDocument.Objects if sel == o.Label]
        if 1 == len(tags):
            if not self.obj.Proxy.copyTags(self.obj, tags[0]):
                self.obj.Proxy.execute(self.obj)
            self.Positions = self.obj.Positions
            self.Disabled = self.obj.Disabled
            self.updateTagsView()
        else:
            Path.Log.error("Cannot copy tags - internal error")

    def updateModel(self):
        self.getFields()
        self.updateTagsView()
        self.isDirty = True

    def whenCountChanged(self):
        count = self.form.sbCount.value()
        self.form.pbGenerate.setEnabled(count)

    def selectTagWithId(self, index):
        Path.Log.track(index)
        self.form.lwTags.setCurrentRow(index)

    def whenTagSelectionChanged(self):
        index = self.form.lwTags.currentRow()
        count = self.form.lwTags.count()
        self.form.pbDelete.setEnabled(index != -1 and count > 2)
        self.form.pbEdit.setEnabled(index != -1)
        self.viewProvider.selectTag(index)

    def whenTagsViewChanged(self):
        self.updateTagsViewWith(self.getTags(True))

    def updateTagsViewWith(self, tags):
        self.tags = tags
        self.Positions = [FreeCAD.Vector(t[0], t[1], 0) for t in tags]
        self.Disabled = [i for (i, t) in enumerate(self.tags) if not t[2]]
        self.updateTagsView()

    def deleteSelectedTag(self):
        self.updateTagsViewWith(self.getTags(False))

    def addNewTagAt(self, point, obj):
        if point and obj and self.obj.Proxy.pointIsOnPath(self.obj, point):
            Path.Log.info("addNewTagAt(%.2f, %.2f)" % (point.x, point.y))
            self.Positions.append(FreeCAD.Vector(point.x, point.y, 0))
            self.updateTagsView()
        else:
            Path.Log.notice(
                "ignore new tag at %s (obj=%s, on-path=%d" % (point, obj, 0)
            )

    def addNewTag(self):
        self.tags = self.getTags(True)
        self.getPoint.getPoint(self.addNewTagAt)

    def editTagAt(self, point, obj):
        Path.Log.track(point, obj)
        if point and self.obj.Proxy.pointIsOnPath(self.obj, point):
            tags = []
            for i, (x, y, enabled) in enumerate(self.tags):
                if i == self.editItem:
                    tags.append((point.x, point.y, enabled))
                else:
                    tags.append((x, y, enabled))
            self.updateTagsViewWith(tags)

    def editTag(self, item):
        if item:
            self.tags = self.getTags(True)
            self.editItem = item.data(self.DataID)
            x = item.data(self.DataX)
            y = item.data(self.DataY)
            z = item.data(self.DataZ)
            self.getPoint.getPoint(self.editTagAt, FreeCAD.Vector(x, y, z))

    def editSelectedTag(self):
        self.editTag(self.form.lwTags.currentItem())

    def setFields(self):
        self.updateTagsView()
        self.form.sbCount.setValue(len(self.Positions))
        self.form.ifHeight.setText(
            FreeCAD.Units.Quantity(self.obj.Height, FreeCAD.Units.Length).UserString
        )
        self.form.ifWidth.setText(
            FreeCAD.Units.Quantity(self.obj.Width, FreeCAD.Units.Length).UserString
        )
        self.form.dsbAngle.setValue(self.obj.Angle)
        self.form.ifRadius.setText(
            FreeCAD.Units.Quantity(self.obj.Radius, FreeCAD.Units.Length).UserString
        )

    def setupUi(self):
        self.Positions = self.obj.Positions
        self.Disabled = self.obj.Disabled

        self.setFields()
        self.whenCountChanged()

        if self.obj.Proxy.supportsTagGeneration(self.obj):
            self.form.sbCount.valueChanged.connect(self.whenCountChanged)
            self.form.pbGenerate.clicked.connect(self.generateNewTags)
        else:
            self.form.cbTagGeneration.setEnabled(False)

        enableCopy = False
        for tags in sorted(
            [o.Label for o in FreeCAD.ActiveDocument.Objects if "DressupTag" in o.Name]
        ):
            if tags != self.obj.Label:
                enableCopy = True
                self.form.cbSource.addItem(tags)
        if enableCopy:
            self.form.gbCopy.setEnabled(True)
            self.form.pbCopy.clicked.connect(self.copyNewTags)

        self.form.lwTags.itemChanged.connect(self.whenTagsViewChanged)
        self.form.lwTags.itemSelectionChanged.connect(self.whenTagSelectionChanged)
        self.form.lwTags.itemActivated.connect(self.editTag)

        self.form.pbDelete.clicked.connect(self.deleteSelectedTag)
        self.form.pbEdit.clicked.connect(self.editSelectedTag)
        self.form.pbAdd.clicked.connect(self.addNewTag)

        self.viewProvider.turnMarkerDisplayOn(True)


class HoldingTagMarker:
    def __init__(self, point, colors):
        self.point = point
        self.color = colors
        self.sep = coin.SoSeparator()
        self.pos = coin.SoTranslation()
        self.pos.translation = (point.x, point.y, point.z)
        self.sphere = coin.SoSphere()
        self.scale = coin.SoType.fromName("SoShapeScale").createInstance()
        self.scale.setPart("shape", self.sphere)
        self.scale.scaleFactor.setValue(7)
        self.material = coin.SoMaterial()
        self.sep.addChild(self.pos)
        self.sep.addChild(self.material)
        self.sep.addChild(self.scale)
        self.enabled = True
        self.selected = False

    def setSelected(self, select):
        self.selected = select
        self.sphere.radius = 1.5 if select else 1.0
        self.setEnabled(self.enabled)

    def setEnabled(self, enabled):
        self.enabled = enabled
        if enabled:
            self.material.diffuseColor = (
                self.color[0] if not self.selected else self.color[2]
            )
            self.material.transparency = 0.0
        else:
            self.material.diffuseColor = (
                self.color[1] if not self.selected else self.color[2]
            )
            self.material.transparency = 0.6


class PathDressupTagViewProvider:
    def __init__(self, vobj):
        Path.Log.track()
        self.vobj = vobj
        self.panel = None

        self.debugDisplay()

        # initialized later
        self.obj = None
        self.tags = None
        self.switch = None
        self.colors = None

    def debugDisplay(self):
        # if False and addDebugDisplay():
        #    if not hasattr(self.vobj, 'Debug'):
        #        self.vobj.addProperty('App::PropertyLink', 'Debug', 'Debug', QT_TRANSLATE_NOOP('Path_DressupTag', 'Some elements for debugging'))
        #        dbg = self.vobj.Object.Document.addObject('App::DocumentObjectGroup', 'TagDebug')
        #        self.vobj.Debug = dbg
        #    return True
        return False

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def setupColors(self):
        def colorForColorValue(val):
            v = [((val >> n) & 0xFF) / 255.0 for n in [24, 16, 8, 0]]
            return coin.SbColor(v[0], v[1], v[2])

        pref = Path.Preferences.preferences()
        #                                                      R         G          B          A
        npc = pref.GetUnsigned(
            "DefaultPathMarkerColor", ((85 * 256 + 255) * 256 + 0) * 256 + 255
        )
        hpc = pref.GetUnsigned(
            "DefaultHighlightPathColor", ((255 * 256 + 125) * 256 + 0) * 256 + 255
        )
        dpc = pref.GetUnsigned(
            "DefaultDisabledPathColor", ((205 * 256 + 205) * 256 + 205) * 256 + 154
        )
        self.colors = [
            colorForColorValue(npc),
            colorForColorValue(dpc),
            colorForColorValue(hpc),
        ]

    def attach(self, vobj):
        Path.Log.track()
        self.setupColors()
        self.vobj = vobj
        self.obj = vobj.Object
        self.tags = []
        self.switch = coin.SoSwitch()
        vobj.RootNode.addChild(self.switch)
        self.turnMarkerDisplayOn(False)

        if self.obj and self.obj.Base:
            for i in self.obj.Base.InList:
                if hasattr(i, "Group") and self.obj.Base.Name in [
                    o.Name for o in i.Group
                ]:
                    i.Group = [o for o in i.Group if o.Name != self.obj.Base.Name]
            if self.obj.Base.ViewObject:
                self.obj.Base.ViewObject.Visibility = False
            # if self.debugDisplay() and self.vobj.Debug.ViewObject:
            #    self.vobj.Debug.ViewObject.Visibility = False

    def turnMarkerDisplayOn(self, display):
        sw = coin.SO_SWITCH_ALL if display else coin.SO_SWITCH_NONE
        self.switch.whichChild = sw

    def claimChildren(self):
        Path.Log.track()
        # if self.debugDisplay():
        #    return [self.obj.Base, self.vobj.Debug]
        return [self.obj.Base]

    def onDelete(self, arg1=None, arg2=None):
        """this makes sure that the base operation is added back to the job and visible"""
        Path.Log.track()
        if self.obj.Base and self.obj.Base.ViewObject:
            self.obj.Base.ViewObject.Visibility = True
        job = PathUtils.findParentJob(self.obj)
        if arg1.Object and arg1.Object.Base and job:
            job.Proxy.addOperation(arg1.Object.Base, arg1.Object)
            arg1.Object.Base = None
        # if self.debugDisplay():
        #    self.vobj.Debug.removeObjectsFromDocument()
        #    self.vobj.Debug.Document.removeObject(self.vobj.Debug.Name)
        #    self.vobj.Debug = None
        return True

    def updatePositions(self, positions, disabled):
        for tag in self.tags:
            self.switch.removeChild(tag.sep)
        tags = []
        for i, p in enumerate(positions):
            tag = HoldingTagMarker(
                self.obj.Proxy.pointAtBottom(self.obj, p), self.colors
            )
            tag.setEnabled(not i in disabled)
            tags.append(tag)
            self.switch.addChild(tag.sep)
        self.tags = tags

    def updateData(self, obj, propName):
        Path.Log.track(propName)
        if "Disabled" == propName:
            self.updatePositions(obj.Positions, obj.Disabled)

    def onModelChanged(self):
        Path.Log.track()
        # if self.debugDisplay():
        #    self.vobj.Debug.removeObjectsFromDocument()
        #    for solid in self.obj.Proxy.solids:
        #        tag = self.obj.Document.addObject('Part::Feature', 'tag')
        #        tag.Shape = solid
        #        if tag.ViewObject and self.vobj.Debug.ViewObject:
        #            tag.ViewObject.Visibility = self.vobj.Debug.ViewObject.Visibility
        #            tag.ViewObject.Transparency = 80
        #        self.vobj.Debug.addObject(tag)
        #    tag.purgeTouched()

    def setEdit(self, vobj, mode=0):
        panel = PathDressupTagTaskPanel(vobj.Object, self)
        self.setupTaskPanel(panel)
        return True

    def unsetEdit(self, vobj, mode):
        if hasattr(self, "panel") and self.panel:
            self.panel.abort()

    def setupTaskPanel(self, panel):
        self.panel = panel
        FreeCADGui.Control.closeDialog()
        FreeCADGui.Control.showDialog(panel)
        panel.setupUi()
        FreeCADGui.Selection.addSelectionGate(self)
        FreeCADGui.Selection.addObserver(self)

    def clearTaskPanel(self):
        self.panel = None
        FreeCADGui.Selection.removeSelectionGate()
        FreeCADGui.Selection.removeObserver(self)
        self.turnMarkerDisplayOn(False)

    # SelectionObserver interface

    def selectTag(self, index):
        Path.Log.track(index)
        for i, tag in enumerate(self.tags):
            tag.setSelected(i == index)

    def tagAtPoint(self, point, matchZ):
        x = point[0]
        y = point[1]
        z = point[2]
        if self.tags and not matchZ:
            z = self.tags[0].point.z
        p = FreeCAD.Vector(x, y, z)
        for i, tag in enumerate(self.tags):
            if Path.Geom.pointsCoincide(
                p, tag.point, tag.sphere.radius.getValue() * 1.3
            ):
                return i
        return -1

    def allow(self, doc, obj, sub):
        if obj == self.obj:
            return True
        return False

    def addSelection(self, doc, obj, sub, point):
        Path.Log.track(doc, obj, sub, point)
        if self.panel:
            i = self.tagAtPoint(point, sub is None)
            self.panel.selectTagWithId(i)
        FreeCADGui.updateGui()


def Create(baseObject, name="DressupTag"):
    """
    Create(basePath, name = 'DressupTag') ... create tag dressup object for the given base path.
    Use this command only iff the UI is up - for batch processing see PathDressupTag.Create
    """
    FreeCAD.ActiveDocument.openTransaction("Create a Tag dressup")
    obj = PathDressupTag.Create(baseObject, name)
    obj.ViewObject.Proxy = PathDressupTagViewProvider(obj.ViewObject)
    FreeCAD.ActiveDocument.commitTransaction()
    obj.ViewObject.Document.setEdit(obj.ViewObject, 0)
    return obj


class CommandPathDressupTag:
    def GetResources(self):
        return {
            "Pixmap": "Path_Dressup",
            "MenuText": QT_TRANSLATE_NOOP("Path_DressupTag", "Tag"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "Path_DressupTag", "Creates a Tag Dress-up object from a selected path"
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
                translate("Path_DressupTag", "Please select one path object") + "\n"
            )
            return
        baseObject = selection[0]

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction("Create Tag Dress-up")
        FreeCADGui.addModule("Path.Dressup.Gui.Tags")
        FreeCADGui.doCommand(
            "Path.Dressup.Gui.Tags.Create(App.ActiveDocument.%s)" % baseObject.Name
        )
        # FreeCAD.ActiveDocument.commitTransaction()  # Final `commitTransaction()` called via TaskPanel.accept()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("Path_DressupTag", CommandPathDressupTag())

Path.Log.notice("Loading PathDressupTagGui... done\n")
