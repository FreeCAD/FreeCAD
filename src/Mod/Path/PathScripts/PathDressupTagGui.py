# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
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
import Draft
import FreeCAD
import FreeCADGui
import Path
import PathScripts
import PathScripts.PathDressupTag as PathDressupTag
import PathScripts.PathLog as PathLog
import PathScripts.PathUtils as PathUtils

from PathScripts.PathGeom import PathGeom
from PathScripts.PathPreferences import PathPreferences
from PySide import QtCore, QtGui
from pivy import coin

PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
PathLog.trackModule()

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class PathDressupTagTaskPanel:
    DataX  = QtCore.Qt.ItemDataRole.UserRole
    DataY  = QtCore.Qt.ItemDataRole.UserRole + 1
    DataZ  = QtCore.Qt.ItemDataRole.UserRole + 2
    DataID = QtCore.Qt.ItemDataRole.UserRole + 3

    def __init__(self, obj, viewProvider, jvoVisibility=None):
        self.obj = obj
        self.obj.Proxy.obj = obj
        self.viewProvider = viewProvider
        self.form = QtGui.QWidget()
        self.formTags  = FreeCADGui.PySideUic.loadUi(":/panels/HoldingTagsEdit.ui")
        self.formPoint = FreeCADGui.PySideUic.loadUi(":/panels/PointEdit.ui")
        self.layout = QtGui.QVBoxLayout(self.form)
        #self.form.setGeometry(self.formTags.geometry())
        self.form.setWindowTitle(self.formTags.windowTitle())
        self.form.setSizePolicy(self.formTags.sizePolicy())
        self.formTags.setParent(self.form)
        self.formPoint.setParent(self.form)
        self.layout.addWidget(self.formTags)
        self.layout.addWidget(self.formPoint)
        self.formPoint.hide()
        self.jvo = PathUtils.findParentJob(obj).ViewObject
        if jvoVisibility is None:
            FreeCAD.ActiveDocument.openTransaction(translate("PathDressup_HoldingTags", "Edit HoldingTags Dress-up"))
            self.jvoVisible = self.jvo.isVisible()
            if self.jvoVisible:
                self.jvo.hide()
        else:
            self.jvoVisible = jvoVisibility
        self.pt = FreeCAD.Vector(0, 0, 0)

        closeButton = self.formPoint.buttonBox.button(QtGui.QDialogButtonBox.StandardButton.Close)
        closeButton.setText(translate("PathDressup_HoldingTags", "Done"))

        self.isDirty = True

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Apply | QtGui.QDialogButtonBox.Cancel)

    def clicked(self, button):
        if button == QtGui.QDialogButtonBox.Apply:
            self.getFields()
            self.obj.Proxy.execute(self.obj)
            self.isDirty = False

    def addEscapeShortcut(self):
        # The only way I could get to intercept the escape key, or really any key was
        # by creating an action with a shortcut .....
        self.escape = QtGui.QAction(self.formPoint)
        self.escape.setText('Done')
        self.escape.setShortcut(QtGui.QKeySequence.fromString('Esc'))
        QtCore.QObject.connect(self.escape, QtCore.SIGNAL('triggered()'), self.pointDone)
        self.formPoint.addAction(self.escape)

    def removeEscapeShortcut(self):
        if self.escape:
            self.formPoint.removeAction(self.escape)
            self.escape = None

    def modifyStandardButtons(self, buttonBox):
        self.buttonBox = buttonBox

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
        self.removeGlobalCallbacks()
        self.viewProvider.clearTaskPanel()
        if gui:
            FreeCADGui.ActiveDocument.resetEdit()
            FreeCADGui.Control.closeDialog()
            FreeCAD.ActiveDocument.recompute()
            if self.jvoVisible:
                self.jvo.show()

    def getTags(self, includeCurrent):
        tags = []
        index = self.formTags.lwTags.currentRow()
        for i in range(0, self.formTags.lwTags.count()):
            item = self.formTags.lwTags.item(i)
            enabled = item.checkState() == QtCore.Qt.CheckState.Checked
            x = item.data(self.DataX)
            y = item.data(self.DataY)
            #print("(%.2f, %.2f) i=%d/%s" % (x, y, i, index))
            if includeCurrent or i != index:
                tags.append((x, y, enabled))
        return tags

    def getFields(self):
        width  = FreeCAD.Units.Quantity(self.formTags.ifWidth.text()).Value
        height = FreeCAD.Units.Quantity(self.formTags.ifHeight.text()).Value
        angle  = self.formTags.dsbAngle.value()
        radius = FreeCAD.Units.Quantity(self.formTags.ifRadius.text()).Value

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
        PathLog.track()
        self.formTags.lwTags.blockSignals(True)
        self.formTags.lwTags.clear()
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
            self.formTags.lwTags.addItem(item)
        self.formTags.lwTags.blockSignals(False)
        self.whenTagSelectionChanged()
        self.viewProvider.updatePositions(self.Positions, self.Disabled)

    def generateNewTags(self):
        count = self.formTags.sbCount.value()
        if not self.obj.Proxy.generateTags(self.obj, count):
            self.obj.Proxy.execute(self.obj)

        self.updateTagsView()
        #if PathLog.getLevel(LOG_MODULE) == PathLog.Level.DEBUG:
        #    # this causes a big of an echo and a double click on the spin buttons, don't know why though
        #    FreeCAD.ActiveDocument.recompute()


    def updateModel(self):
        self.getFields()
        self.updateTagsView()
        self.isDirty = True
        #FreeCAD.ActiveDocument.recompute()

    def whenCountChanged(self):
        count = self.formTags.sbCount.value()
        self.formTags.pbGenerate.setEnabled(count)

    def selectTagWithId(self, index):
        PathLog.track(index)
        self.formTags.lwTags.setCurrentRow(index)

    def whenTagSelectionChanged(self):
        index = self.formTags.lwTags.currentRow()
        count = self.formTags.lwTags.count()
        self.formTags.pbDelete.setEnabled(index != -1 and count > 2)
        self.formTags.pbEdit.setEnabled(index != -1)
        self.viewProvider.selectTag(index)

    def whenTagsViewChanged(self):
        self.updateTagsViewWith(self.getTags(True))

    def updateTagsViewWith(self, tags):
        self.tags = tags
        self.Positions = [FreeCAD.Vector(t[0], t[1], 0) for t in self.tags]
        self.Disabled = [i for (i,t) in enumerate(self.tags) if not t[2]]
        self.updateTagsView()

    def deleteSelectedTag(self):
        self.updateTagsViewWith(self.getTags(False))

    def addNewTagAt(self, point, obj):
        if point and obj and self.obj.Proxy.pointIsOnPath(self.obj, point):
            PathLog.info("addNewTagAt(%.2f, %.2f)" % (point.x, point.y))
            self.Positions.append(FreeCAD.Vector(point.x, point.y, 0))
            self.updateTagsView()
        else:
            print("ignore new tag at %s" % (point))

    def addNewTag(self):
        self.tags = self.getTags(True)
        self.getPoint(self.addNewTagAt)

    def editTagAt(self, point, obj):
        if point and obj and (obj or point != FreeCAD.Vector()) and self.obj.Proxy.pointIsOnPath(self.obj, point):
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
            self.getPoint(self.editTagAt, FreeCAD.Vector(x, y, z))

    def editSelectedTag(self):
        self.editTag(self.formTags.lwTags.currentItem())

    def removeGlobalCallbacks(self):
        if hasattr(self, 'view') and self.view:
            if self.pointCbClick:
                self.view.removeEventCallbackPivy(coin.SoMouseButtonEvent.getClassTypeId(), self.pointCbClick)
                self.pointCbClick = None
            if self.pointCbMove:
                self.view.removeEventCallbackPivy(coin.SoLocation2Event.getClassTypeId(), self.pointCbMove)
                self.pointCbMove = None
            self.view = None

    def getPoint(self, whenDone, start=None):

        def displayPoint(p):
            self.formPoint.ifValueX.setText(FreeCAD.Units.Quantity(p.x, FreeCAD.Units.Length).UserString)
            self.formPoint.ifValueY.setText(FreeCAD.Units.Quantity(p.y, FreeCAD.Units.Length).UserString)
            self.formPoint.ifValueZ.setText(FreeCAD.Units.Quantity(p.z, FreeCAD.Units.Length).UserString)
            self.formPoint.ifValueX.setFocus()
            self.formPoint.ifValueX.selectAll()

        def mouseMove(cb):
            event = cb.getEvent()
            pos = event.getPosition()
            cntrl = event.wasCtrlDown()
            shift = event.wasShiftDown()
            self.pt = FreeCADGui.Snapper.snap(pos, lastpoint=start, active=cntrl, constrain=shift)
            plane = FreeCAD.DraftWorkingPlane
            p = plane.getLocalCoords(self.pt)
            displayPoint(p)

        def click(cb):
            event = cb.getEvent()
            if event.getButton() == 1 and event.getState() == coin.SoMouseButtonEvent.DOWN:
                accept()

        def accept():
            if start:
                self.pointAccept()
            else:
                self.pointAcceptAndContinue()

        def cancel():
            self.pointCancel()

        self.pointWhenDone = whenDone
        self.formTags.hide()
        self.formPoint.show()
        self.addEscapeShortcut()
        if start:
            displayPoint(start)
        else:
            displayPoint(FreeCAD.Vector(0,0,0))

        self.view = Draft.get3DView()
        self.pointCbClick = self.view.addEventCallbackPivy(coin.SoMouseButtonEvent.getClassTypeId(), click)
        self.pointCbMove = self.view.addEventCallbackPivy(coin.SoLocation2Event.getClassTypeId(), mouseMove)

        self.buttonBox.setEnabled(False)

    def setupSpinBox(self, widget, val, decimals = 2):
        if decimals:
            widget.setDecimals(decimals)
        widget.setValue(val)

    def setFields(self):
        self.updateTagsView()
        self.formTags.sbCount.setValue(len(self.Positions))
        self.formTags.ifHeight.setText(FreeCAD.Units.Quantity(self.obj.Height, FreeCAD.Units.Length).UserString)
        self.formTags.ifWidth.setText(FreeCAD.Units.Quantity(self.obj.Width, FreeCAD.Units.Length).UserString)
        self.formTags.dsbAngle.setValue(self.obj.Angle)
        self.formTags.ifRadius.setText(FreeCAD.Units.Quantity(self.obj.Radius, FreeCAD.Units.Length).UserString)

    def setupUi(self):
        self.Positions = self.obj.Positions
        self.Disabled  = self.obj.Disabled

        self.setFields()
        self.whenCountChanged()

        if self.obj.Proxy.supportsTagGeneration(self.obj):
            self.formTags.sbCount.valueChanged.connect(self.whenCountChanged)
            self.formTags.pbGenerate.clicked.connect(self.generateNewTags)
        else:
            self.formTags.cbTagGeneration.setEnabled(False)

        self.formTags.lwTags.itemChanged.connect(self.whenTagsViewChanged)
        self.formTags.lwTags.itemSelectionChanged.connect(self.whenTagSelectionChanged)
        self.formTags.lwTags.itemActivated.connect(self.editTag)

        self.formTags.pbDelete.clicked.connect(self.deleteSelectedTag)
        self.formTags.pbEdit.clicked.connect(self.editSelectedTag)
        self.formTags.pbAdd.clicked.connect(self.addNewTag)

        self.formPoint.buttonBox.accepted.connect(self.pointAccept)
        self.formPoint.buttonBox.rejected.connect(self.pointReject)

        self.formPoint.ifValueX.editingFinished.connect(self.updatePoint)
        self.formPoint.ifValueY.editingFinished.connect(self.updatePoint)
        self.formPoint.ifValueZ.editingFinished.connect(self.updatePoint)

        self.viewProvider.turnMarkerDisplayOn(True)

    def pointFinish(self, ok, cleanup = True):
        obj = FreeCADGui.Snapper.lastSnappedObject

        if cleanup:
            self.removeGlobalCallbacks();
            FreeCADGui.Snapper.off()
            self.buttonBox.setEnabled(True)
            self.removeEscapeShortcut()
            self.formPoint.hide()
            self.formTags.show()
            self.formTags.setFocus()

        if ok:
            self.pointWhenDone(self.pt, obj)
        else:
            self.pointWhenDone(None, None)

    def pointDone(self):
        self.pointFinish(False)

    def pointReject(self):
        self.pointFinish(False)

    def pointAccept(self):
        self.pointFinish(True)

    def pointAcceptAndContinue(self):
        self.pointFinish(True, False)

    def updatePoint(self):
        x = FreeCAD.Units.Quantity(self.formPoint.ifValueX.text()).Value
        y = FreeCAD.Units.Quantity(self.formPoint.ifValueY.text()).Value
        z = FreeCAD.Units.Quantity(self.formPoint.ifValueZ.text()).Value
        self.pt = FreeCAD.Vector(x, y, z)

class HoldingTagMarker:
    def __init__(self, point, colors):
        self.point = point
        self.color = colors
        self.sep = coin.SoSeparator()
        self.pos = coin.SoTranslation()
        self.pos.translation = (point.x, point.y, point.z)
        self.sphere = coin.SoSphere()
        self.scale = coin.SoType.fromName('SoShapeScale').createInstance()
        self.scale.setPart('shape', self.sphere)
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
            self.material.diffuseColor = self.color[0] if not self.selected else self.color[2]
            self.material.transparency = 0.0
        else:
            self.material.diffuseColor = self.color[1] if not self.selected else self.color[2]
            self.material.transparency = 0.6

class PathDressupTagViewProvider:

    def __init__(self, vobj):
        PathLog.track()
        vobj.Proxy = self
        self.vobj = vobj
        self.panel = None

    def __getstate__(self):
        return None
    def __setstate__(self, state):
        return None

    def setupColors(self):
        def colorForColorValue(val):
            v = [((val >> n) & 0xff) / 255. for n in [24, 16, 8, 0]]
            return coin.SbColor(v[0], v[1], v[2])

        pref = PathPreferences.preferences()
        #                                                      R         G          B          A
        npc = pref.GetUnsigned('DefaultPathMarkerColor',    (( 85*256 + 255)*256 +   0)*256 + 255)
        hpc = pref.GetUnsigned('DefaultHighlightPathColor', ((255*256 + 125)*256 +   0)*256 + 255)
        dpc = pref.GetUnsigned('DefaultDisabledPathColor',  ((205*256 + 205)*256 + 205)*256 + 154)
        self.colors = [colorForColorValue(npc), colorForColorValue(dpc), colorForColorValue(hpc)]

    def attach(self, vobj):
        PathLog.track()
        self.setupColors()
        self.obj = vobj.Object
        self.tags = []
        self.switch = coin.SoSwitch()
        vobj.RootNode.addChild(self.switch)
        self.turnMarkerDisplayOn(False)

        if self.obj and self.obj.Base:
            for i in self.obj.Base.InList:
                if hasattr(i, 'Group') and self.obj.Base.Name in [o.Name for o in i.Group]:
                    i.Group = [o for o in i.Group if o.Name != self.obj.Base.Name]
            if self.obj.Base.ViewObject:
                self.obj.Base.ViewObject.Visibility = False
            if PathLog.getLevel(PathLog.thisModule()) != PathLog.Level.DEBUG and self.obj.Debug.ViewObject:
                self.obj.Debug.ViewObject.Visibility = False

        self.obj.Proxy.changed.connect(self.onModelChanged)

    def turnMarkerDisplayOn(self, display):
        sw = coin.SO_SWITCH_ALL if display else coin.SO_SWITCH_NONE
        self.switch.whichChild = sw

    def claimChildren(self):
        PathLog.track()
        return [self.obj.Base, self.obj.Debug]

    def onDelete(self, arg1=None, arg2=None):
        PathLog.track()
        '''this makes sure that the base operation is added back to the project and visible'''
        if self.obj.Base.ViewObject:
            self.obj.Base.ViewObject.Visibility = True
        PathUtils.addToJob(arg1.Object.Base)
        self.obj.Debug.removeObjectsFromDocument()
        self.obj.Debug.Document.removeObject(self.obj.Debug.Name)
        self.obj.Debug = None
        return True

    def updatePositions(self, positions, disabled):
        for tag in self.tags:
            self.switch.removeChild(tag.sep)
        tags = []
        for i, p in enumerate(positions):
            tag = HoldingTagMarker(p, self.colors)
            tag.setEnabled(not i in disabled)
            tags.append(tag)
            self.switch.addChild(tag.sep)
        self.tags = tags

    def updateData(self, obj, propName):
        PathLog.track(propName)
        if 'Disabled' == propName:
            self.updatePositions(obj.Positions, obj.Disabled)

    def onModelChanged(self):
        PathLog.track()
        self.obj.Debug.removeObjectsFromDocument()
        for solid in self.obj.Proxy.solids:
            tag = self.obj.Document.addObject('Part::Feature', 'tag')
            tag.Shape = solid
            if tag.ViewObject and self.obj.Debug.ViewObject:
                tag.ViewObject.Visibility = self.obj.Debug.ViewObject.Visibility
                tag.ViewObject.Transparency = 80
            self.obj.Debug.addObject(tag)
            tag.purgeTouched()

    def setEdit(self, vobj, mode=0):
        panel = PathDressupTagTaskPanel(vobj.Object, self)
        self.setupTaskPanel(panel)
        return True

    def unsetEdit(self, vobj, mode):
        if hasattr(self, 'panel') and self.panel:
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
        PathLog.track(index)
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
            if PathGeom.pointsCoincide(p, tag.point, tag.sphere.radius.getValue() * 1.3):
                return i
        return -1

    def allow(self, doc, obj, sub):
        if obj == self.obj:
            return True
        return False

    def addSelection(self, doc, obj, sub, point):
        PathLog.track(doc, obj, sub, point)
        if self.panel:
            i = self.tagAtPoint(point, sub is None)
            self.panel.selectTagWithId(i)
        FreeCADGui.updateGui()

def Create(baseObject, name='DressupTag'):
    '''
    Create(basePath, name = 'DressupTag') ... create tag dressup object for the given base path.
    Use this command only iff the UI is up - for batch processing see PathDressupTag.Create
    '''
    FreeCAD.ActiveDocument.openTransaction(translate("PathDressup_Tag", "Create a Tag dressup"))
    obj = PathDressupTag.Create(baseObject, name)
    vp = PathDressupTagViewProvider(obj.ViewObject)
    FreeCAD.ActiveDocument.commitTransaction()
    obj.ViewObject.startEditing()
    return obj

class CommandPathDressupTag:

    def GetResources(self):
        return {'Pixmap': 'Path-Dressup',
                'MenuText': QtCore.QT_TRANSLATE_NOOP('PathDressup_Tag', 'Tag Dress-up'),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP('PathDressup_Tag', 'Creates a Tag Dress-up object from a selected path')}

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == 'Job':
                        return True
        return False

    def Activated(self):
        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) != 1:
            PathLog.error(translate('PathDressup_Tag', 'Please select one path object\n'))
            return
        baseObject = selection[0]

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction(translate('PathDressup_Tag', 'Create Tag Dress-up'))
        FreeCADGui.addModule('PathScripts.PathDressupTagGui')
        FreeCADGui.doCommand("PathScripts.PathDressupTagGui.Create(App.ActiveDocument.%s)" % baseObject.Name)
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('PathDressup_Tag', CommandPathDressupTag())

PathLog.notice('Loading PathDressupTagGui... done\n')
