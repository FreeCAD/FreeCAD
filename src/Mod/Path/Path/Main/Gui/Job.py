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
from collections import Counter
from contextlib import contextmanager
from pivy import coin
import FreeCAD
import FreeCADGui
import Path
import Path.Base.Gui.SetupSheet as PathSetupSheetGui
import Path.Base.Util as PathUtil
import Path.GuiInit as PathGuiInit
import Path.Main.Gui.JobCmd as PathJobCmd
import Path.Main.Gui.JobDlg as PathJobDlg
import Path.Main.Job as PathJob
import Path.Main.Stock as PathStock
import Path.Tool.Gui.Bit as PathToolBitGui
import Path.Tool.Gui.Controller as PathToolControllerGui
import PathScripts.PathUtils as PathUtils
import json
import math
import traceback

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Draft = LazyLoader("Draft", globals(), "Draft")
Part = LazyLoader("Part", globals(), "Part")
DraftVecUtils = LazyLoader("DraftVecUtils", globals(), "DraftVecUtils")

translate = FreeCAD.Qt.translate

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


def _OpenCloseResourceEditor(obj, vobj, edit):
    job = PathUtils.findParentJob(obj)
    if job and job.ViewObject and job.ViewObject.Proxy:
        if edit:
            job.ViewObject.Proxy.editObject(obj)
        else:
            job.ViewObject.Proxy.uneditObject(obj)
    else:
        missing = "Job"
        if job:
            missing = "ViewObject"
            if job.ViewObject:
                missing = "Proxy"
        Path.Log.warning("Cannot edit %s - no %s" % (obj.Label, missing))


@contextmanager
def selectionEx():
    sel = FreeCADGui.Selection.getSelectionEx()
    try:
        yield sel
    finally:
        FreeCADGui.Selection.clearSelection()
        for s in sel:
            if s.SubElementNames:
                FreeCADGui.Selection.addSelection(s.Object, s.SubElementNames)
            else:
                FreeCADGui.Selection.addSelection(s.Object)


class ViewProvider:
    def __init__(self, vobj):
        mode = 2
        vobj.setEditorMode("BoundingBox", mode)
        vobj.setEditorMode("DisplayMode", mode)
        vobj.setEditorMode("Selectable", mode)
        vobj.setEditorMode("ShapeColor", mode)
        vobj.setEditorMode("Transparency", mode)
        self.deleteOnReject = True

        # initialized later
        self.axs = None
        self.mat = None
        self.obj = None
        self.sca = None
        self.scs = None
        self.sep = None
        self.sph = None
        self.switch = None
        self.taskPanel = None
        self.vobj = None
        self.baseVisibility = {}
        self.stockVisibility = False

    def attach(self, vobj):
        self.vobj = vobj
        self.obj = vobj.Object
        self.taskPanel = None
        if not hasattr(self, "baseVisibility"):
            self.baseVisibility = {}
        if not hasattr(self, "stockVisibility"):
            self.stockVisibility = False

        # setup the axis display at the origin
        self.switch = coin.SoSwitch()
        self.sep = coin.SoSeparator()
        self.axs = coin.SoType.fromName("SoAxisCrossKit").createInstance()
        self.axs.set("xHead.transform", "scaleFactor 2 3 2")
        self.axs.set("yHead.transform", "scaleFactor 2 3 2")
        self.axs.set("zHead.transform", "scaleFactor 2 3 2")
        self.sca = coin.SoType.fromName("SoShapeScale").createInstance()
        self.sca.setPart("shape", self.axs)
        self.sca.scaleFactor.setValue(0.5)
        self.mat = coin.SoMaterial()
        self.mat.diffuseColor = coin.SbColor(0.9, 0, 0.9)
        self.mat.transparency = 0.85
        self.sph = coin.SoSphere()
        self.scs = coin.SoType.fromName("SoShapeScale").createInstance()
        self.scs.setPart("shape", self.sph)
        self.scs.scaleFactor.setValue(10)
        self.sep.addChild(self.sca)
        self.sep.addChild(self.mat)
        self.sep.addChild(self.scs)
        self.switch.addChild(self.sep)
        vobj.RootNode.addChild(self.switch)
        self.showOriginAxis(False)

    def showOriginAxis(self, yes):
        sw = coin.SO_SWITCH_ALL if yes else coin.SO_SWITCH_NONE
        self.switch.whichChild = sw

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def deleteObjectsOnReject(self):
        return hasattr(self, "deleteOnReject") and self.deleteOnReject

    def setEdit(self, vobj=None, mode=0):
        Path.Log.track(mode)
        if 0 == mode:
            job = self.vobj.Object
            if not job.Proxy.integrityCheck(job):
                return False
            self.openTaskPanel()
        return True

    def openTaskPanel(self, activate=None):
        self.taskPanel = TaskPanel(self.vobj, self.deleteObjectsOnReject())
        FreeCADGui.Control.closeDialog()
        FreeCADGui.Control.showDialog(self.taskPanel)
        self.taskPanel.setupUi(activate)
        self.showOriginAxis(True)
        self.deleteOnReject = False

    def resetTaskPanel(self):
        self.showOriginAxis(False)
        self.taskPanel = None

    def unsetEdit(self, arg1, arg2):
        if self.taskPanel:
            self.taskPanel.reject(False)

    def editObject(self, obj):
        if obj:
            if obj in self.obj.Model.Group:
                return self.openTaskPanel("Model")
            if obj == self.obj.Stock:
                return self.openTaskPanel("Stock")
            Path.Log.info(
                "Expected a specific object to edit - %s not recognized" % obj.Label
            )
        return self.openTaskPanel()

    def uneditObject(self, obj=None):
        self.unsetEdit(None, None)

    def getIcon(self):
        return ":/icons/Path_Job.svg"

    def claimChildren(self):
        children = []
        children.append(self.obj.Operations)
        if hasattr(self.obj, "Model"):
            # unfortunately this function is called before the object has been fully loaded
            # which means we could be dealing with an old job which doesn't have the new Model
            # yet.
            children.append(self.obj.Model)
        if self.obj.Stock:
            children.append(self.obj.Stock)
        if hasattr(self.obj, "SetupSheet"):
            # when loading a job that didn't have a setup sheet they might not've been created yet
            children.append(self.obj.SetupSheet)
        if hasattr(self.obj, "Tools"):
            children.append(self.obj.Tools)
        return children

    def onDelete(self, vobj, arg2=None):
        Path.Log.track(vobj.Object.Label, arg2)
        self.obj.Proxy.onDelete(self.obj, arg2)
        return True

    def updateData(self, obj, prop):
        Path.Log.track(obj.Label, prop)
        # make sure the resource view providers are setup properly
        if prop == "Model" and self.obj.Model:
            for base in self.obj.Model.Group:
                if base.ViewObject and base.ViewObject.Proxy:
                    base.ViewObject.Proxy.onEdit(_OpenCloseResourceEditor)
        if (
            prop == "Stock"
            and self.obj.Stock
            and self.obj.Stock.ViewObject
            and self.obj.Stock.ViewObject.Proxy
        ):
            self.obj.Stock.ViewObject.Proxy.onEdit(_OpenCloseResourceEditor)

    def rememberBaseVisibility(self, obj, base):
        Path.Log.track()
        if base.ViewObject:
            orig = PathUtil.getPublicObject(obj.Proxy.baseObject(obj, base))
            self.baseVisibility[base.Name] = (
                base,
                base.ViewObject.Visibility,
                orig,
                orig.ViewObject.Visibility,
            )
            orig.ViewObject.Visibility = False
            base.ViewObject.Visibility = True

    def forgetBaseVisibility(self, obj, base):
        Path.Log.track()
        if self.baseVisibility.get(base.Name):
            visibility = self.baseVisibility[base.Name]
            visibility[0].ViewObject.Visibility = visibility[1]
            visibility[2].ViewObject.Visibility = visibility[3]
            del self.baseVisibility[base.Name]

    def setupEditVisibility(self, obj):
        Path.Log.track()
        self.baseVisibility = {}
        for base in obj.Model.Group:
            self.rememberBaseVisibility(obj, base)

        self.stockVisibility = False
        if obj.Stock and obj.Stock.ViewObject:
            self.stockVisibility = obj.Stock.ViewObject.Visibility
            self.obj.Stock.ViewObject.Visibility = True

    def resetEditVisibility(self, obj):
        Path.Log.track()
        for base in obj.Model.Group:
            self.forgetBaseVisibility(obj, base)
        if obj.Stock and obj.Stock.ViewObject:
            obj.Stock.ViewObject.Visibility = self.stockVisibility

    def setupContextMenu(self, vobj, menu):
        Path.Log.track()
        for action in menu.actions():
            menu.removeAction(action)
        action = QtGui.QAction(translate("Path_Job", "Edit"), menu)
        action.triggered.connect(self.setEdit)
        menu.addAction(action)


class StockEdit(object):
    Index = -1
    StockType = PathStock.StockType.Unknown

    def __init__(self, obj, form, force):
        Path.Log.track(obj.Label, force)
        self.obj = obj
        self.form = form
        self.force = force
        self.setupUi(obj)

    @classmethod
    def IsStock(cls, obj):
        return PathStock.StockType.FromStock(obj.Stock) == cls.StockType

    def activate(self, obj, select=False):
        Path.Log.track(obj.Label, select)

        def showHide(widget, activeWidget):
            if widget == activeWidget:
                widget.show()
            else:
                widget.hide()

        if select:
            self.form.stock.setCurrentIndex(self.Index)
        editor = self.editorFrame()
        showHide(self.form.stockFromExisting, editor)
        showHide(self.form.stockFromBase, editor)
        showHide(self.form.stockCreateBox, editor)
        showHide(self.form.stockCreateCylinder, editor)
        self.setFields(obj)

    def setStock(self, obj, stock):
        Path.Log.track(obj.Label, stock)
        if obj.Stock:
            Path.Log.track(obj.Stock.Name)
            obj.Document.removeObject(obj.Stock.Name)
        Path.Log.track(stock.Name)
        obj.Stock = stock
        if stock.ViewObject and stock.ViewObject.Proxy:
            stock.ViewObject.Proxy.onEdit(_OpenCloseResourceEditor)

    def setLengthField(self, widget, prop):
        widget.setText(
            FreeCAD.Units.Quantity(prop.Value, FreeCAD.Units.Length).UserString
        )

    # the following members must be overwritten by subclasses
    def editorFrame(self):
        return None

    def setFields(self, obj):
        pass

    def setupUi(self, obj):
        pass


class StockFromBaseBoundBoxEdit(StockEdit):
    Index = 2
    StockType = PathStock.StockType.FromBase

    def __init__(self, obj, form, force):
        super(StockFromBaseBoundBoxEdit, self).__init__(obj, form, force)

        self.trackXpos = None
        self.trackYpos = None
        self.trackZpos = None

    def editorFrame(self):
        Path.Log.track()
        return self.form.stockFromBase

    def getFieldsStock(self, stock, fields=None):
        if fields is None:
            fields = ["xneg", "xpos", "yneg", "ypos", "zneg", "zpos"]
        try:
            if "xneg" in fields:
                stock.ExtXneg = FreeCAD.Units.Quantity(self.form.stockExtXneg.text())
            if "xpos" in fields:
                stock.ExtXpos = FreeCAD.Units.Quantity(self.form.stockExtXpos.text())
            if "yneg" in fields:
                stock.ExtYneg = FreeCAD.Units.Quantity(self.form.stockExtYneg.text())
            if "ypos" in fields:
                stock.ExtYpos = FreeCAD.Units.Quantity(self.form.stockExtYpos.text())
            if "zneg" in fields:
                stock.ExtZneg = FreeCAD.Units.Quantity(self.form.stockExtZneg.text())
            if "zpos" in fields:
                stock.ExtZpos = FreeCAD.Units.Quantity(self.form.stockExtZpos.text())
        except Exception:
            pass

    def getFields(self, obj, fields=None):
        if fields is None:
            fields = ["xneg", "xpos", "yneg", "ypos", "zneg", "zpos"]
        Path.Log.track(obj.Label, fields)
        if self.IsStock(obj):
            self.getFieldsStock(obj.Stock, fields)
        else:
            Path.Log.error("Stock not from Base bound box!")

    def setFields(self, obj):
        Path.Log.track()
        if self.force or not self.IsStock(obj):
            Path.Log.track()
            stock = PathStock.CreateFromBase(obj)
            if self.force and self.editorFrame().isVisible():
                self.getFieldsStock(stock)
            self.setStock(obj, stock)
            self.force = False
        self.setLengthField(self.form.stockExtXneg, obj.Stock.ExtXneg)
        self.setLengthField(self.form.stockExtXpos, obj.Stock.ExtXpos)
        self.setLengthField(self.form.stockExtYneg, obj.Stock.ExtYneg)
        self.setLengthField(self.form.stockExtYpos, obj.Stock.ExtYpos)
        self.setLengthField(self.form.stockExtZneg, obj.Stock.ExtZneg)
        self.setLengthField(self.form.stockExtZpos, obj.Stock.ExtZpos)

    def setupUi(self, obj):
        Path.Log.track()
        self.setFields(obj)
        self.checkXpos()
        self.checkYpos()
        self.checkZpos()
        self.form.stockExtXneg.textChanged.connect(self.updateXpos)
        self.form.stockExtYneg.textChanged.connect(self.updateYpos)
        self.form.stockExtZneg.textChanged.connect(self.updateZpos)
        self.form.stockExtXpos.textChanged.connect(self.checkXpos)
        self.form.stockExtYpos.textChanged.connect(self.checkYpos)
        self.form.stockExtZpos.textChanged.connect(self.checkZpos)
        if hasattr(self.form, "linkStockAndModel"):
            self.form.linkStockAndModel.setChecked(False)

    def checkXpos(self):
        self.trackXpos = self.form.stockExtXneg.text() == self.form.stockExtXpos.text()
        self.getFields(self.obj, ["xpos"])

    def checkYpos(self):
        self.trackYpos = self.form.stockExtYneg.text() == self.form.stockExtYpos.text()
        self.getFields(self.obj, ["ypos"])

    def checkZpos(self):
        self.trackZpos = self.form.stockExtZneg.text() == self.form.stockExtZpos.text()
        self.getFields(self.obj, ["zpos"])

    def updateXpos(self):
        fields = ["xneg"]
        if self.trackXpos:
            self.form.stockExtXpos.setText(self.form.stockExtXneg.text())
            fields.append("xpos")
        self.getFields(self.obj, fields)

    def updateYpos(self):
        fields = ["yneg"]
        if self.trackYpos:
            self.form.stockExtYpos.setText(self.form.stockExtYneg.text())
            fields.append("ypos")
        self.getFields(self.obj, fields)

    def updateZpos(self):
        fields = ["zneg"]
        if self.trackZpos:
            self.form.stockExtZpos.setText(self.form.stockExtZneg.text())
            fields.append("zpos")
        self.getFields(self.obj, fields)


class StockCreateBoxEdit(StockEdit):
    Index = 0
    StockType = PathStock.StockType.CreateBox

    def editorFrame(self):
        return self.form.stockCreateBox

    def getFields(self, obj, fields=None):
        if fields is None:
            fields = ["length", "width", "height"]
        try:
            if self.IsStock(obj):
                if "length" in fields:
                    obj.Stock.Length = FreeCAD.Units.Quantity(
                        self.form.stockBoxLength.text()
                    )
                if "width" in fields:
                    obj.Stock.Width = FreeCAD.Units.Quantity(
                        self.form.stockBoxWidth.text()
                    )
                if "height" in fields:
                    obj.Stock.Height = FreeCAD.Units.Quantity(
                        self.form.stockBoxHeight.text()
                    )
            else:
                Path.Log.error("Stock not a box!")
        except Exception:
            pass

    def setFields(self, obj):
        if self.force or not self.IsStock(obj):
            self.setStock(obj, PathStock.CreateBox(obj))
            self.force = False
        self.setLengthField(self.form.stockBoxLength, obj.Stock.Length)
        self.setLengthField(self.form.stockBoxWidth, obj.Stock.Width)
        self.setLengthField(self.form.stockBoxHeight, obj.Stock.Height)

    def setupUi(self, obj):
        self.setFields(obj)
        self.form.stockBoxLength.textChanged.connect(
            lambda: self.getFields(obj, ["length"])
        )
        self.form.stockBoxWidth.textChanged.connect(
            lambda: self.getFields(obj, ["width"])
        )
        self.form.stockBoxHeight.textChanged.connect(
            lambda: self.getFields(obj, ["height"])
        )


class StockCreateCylinderEdit(StockEdit):
    Index = 1
    StockType = PathStock.StockType.CreateCylinder

    def editorFrame(self):
        return self.form.stockCreateCylinder

    def getFields(self, obj, fields=None):
        if fields is None:
            fields = ["radius", "height"]
        try:
            if self.IsStock(obj):
                if "radius" in fields:
                    obj.Stock.Radius = FreeCAD.Units.Quantity(
                        self.form.stockCylinderRadius.text()
                    )
                if "height" in fields:
                    obj.Stock.Height = FreeCAD.Units.Quantity(
                        self.form.stockCylinderHeight.text()
                    )
            else:
                Path.Log.error(translate("Path_Job", "Stock not a cylinder!"))
        except Exception:
            pass

    def setFields(self, obj):
        if self.force or not self.IsStock(obj):
            self.setStock(obj, PathStock.CreateCylinder(obj))
            self.force = False
        self.setLengthField(self.form.stockCylinderRadius, obj.Stock.Radius)
        self.setLengthField(self.form.stockCylinderHeight, obj.Stock.Height)

    def setupUi(self, obj):
        self.setFields(obj)
        self.form.stockCylinderRadius.textChanged.connect(
            lambda: self.getFields(obj, ["radius"])
        )
        self.form.stockCylinderHeight.textChanged.connect(
            lambda: self.getFields(obj, ["height"])
        )


class StockFromExistingEdit(StockEdit):
    Index = 3
    StockType = PathStock.StockType.Unknown
    StockLabelPrefix = "Stock"

    def editorFrame(self):
        return self.form.stockFromExisting

    def getFields(self, obj):
        stock = self.form.stockExisting.itemData(self.form.stockExisting.currentIndex())
        if not (
            hasattr(obj.Stock, "Objects")
            and len(obj.Stock.Objects) == 1
            and obj.Stock.Objects[0] == stock
        ):
            if stock:
                stock = PathJob.createResourceClone(
                    obj, stock, self.StockLabelPrefix, "Stock"
                )
                stock.ViewObject.Visibility = True
                PathStock.SetupStockObject(stock, PathStock.StockType.Unknown)
                stock.Proxy.execute(stock)
                self.setStock(obj, stock)

    def candidates(self, obj):
        solids = [o for o in obj.Document.Objects if PathUtil.isSolid(o)]
        if hasattr(obj, "Model"):
            job = obj
        else:
            job = PathUtils.findParentJob(obj)
        for base in job.Model.Group:
            if base in solids and PathJob.isResourceClone(job, base, "Model"):
                solids.remove(base)
        if job.Stock in solids:
            # regardless, what stock is/was, it's not a valid choice
            solids.remove(job.Stock)
        return sorted(solids, key=lambda c: c.Label)

    def setFields(self, obj):
        self.form.stockExisting.clear()
        stockName = obj.Stock.Label if obj.Stock else None
        index = -1
        for i, solid in enumerate(self.candidates(obj)):
            self.form.stockExisting.addItem(solid.Label, solid)
            label = "{}-{}".format(self.StockLabelPrefix, solid.Label)

            if label == stockName:
                index = i
        self.form.stockExisting.setCurrentIndex(index if index != -1 else 0)

        if not self.IsStock(obj):
            self.getFields(obj)

    def setupUi(self, obj):
        self.setFields(obj)
        self.form.stockExisting.currentIndexChanged.connect(lambda: self.getFields(obj))


class TaskPanel:
    DataObject = QtCore.Qt.ItemDataRole.UserRole
    DataProperty = QtCore.Qt.ItemDataRole.UserRole + 1

    def __init__(self, vobj, deleteOnReject):
        FreeCAD.ActiveDocument.openTransaction("Edit Job")
        self.vobj = vobj
        self.vproxy = vobj.Proxy
        self.obj = vobj.Object
        self.deleteOnReject = deleteOnReject
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/PathEdit.ui")
        self.template = PathJobDlg.JobTemplateExport(
            self.obj, self.form.jobBox.widget(1)
        )
        self.name = self.obj.Name

        vUnit = FreeCAD.Units.Quantity(1, FreeCAD.Units.Velocity).getUserPreferred()[2]
        self.form.toolControllerList.horizontalHeaderItem(1).setText("#")
        self.form.toolControllerList.horizontalHeaderItem(2).setText(
            translate("Path", "H","H is horizontal feed rate. Must be as short as possible")
        )
        self.form.toolControllerList.horizontalHeaderItem(3).setText(
            translate("Path", "V","V is vertical feed rate. Must be as short as possible")
        )
        self.form.toolControllerList.horizontalHeader().setResizeMode(
            0, QtGui.QHeaderView.Stretch
        )
        self.form.toolControllerList.horizontalHeaderItem(1).setToolTip(
            translate("Path", "Tool number") + ' '
        )
        self.form.toolControllerList.horizontalHeaderItem(2).setToolTip(
            translate("Path", "Horizontal feedrate")+ ' ' + vUnit
        )
        self.form.toolControllerList.horizontalHeaderItem(3).setToolTip(
            translate("Path", "Vertical feedrate")+ ' ' + vUnit
        )
        self.form.toolControllerList.horizontalHeaderItem(4).setToolTip(
            translate("Path", "Spindle RPM")+ ' '
        )

        # ensure correct ellisis behaviour on tool controller names.
        self.form.toolControllerList.setWordWrap(False)

        self.form.toolControllerList.resizeColumnsToContents()

        currentPostProcessor = self.obj.PostProcessor
        postProcessors = Path.Preferences.allEnabledPostProcessors(
            ["", currentPostProcessor]
        )
        for post in postProcessors:
            self.form.postProcessor.addItem(post)
        # update the enumeration values, just to make sure all selections are valid
        self.obj.PostProcessor = postProcessors
        self.obj.PostProcessor = currentPostProcessor

        self.postProcessorDefaultTooltip = self.form.postProcessor.toolTip()
        self.postProcessorArgsDefaultTooltip = (
            self.form.postProcessorArguments.toolTip()
        )

        # Populate the other comboboxes with enums from the job class
        comboToPropertyMap = [("orderBy", "OrderOutputBy")]
        enumTups = PathJob.ObjectJob.propertyEnumerations(dataType="raw")
        self.populateCombobox(self.form, enumTups, comboToPropertyMap)

        self.vproxy.setupEditVisibility(self.obj)

        self.stockFromBase = None
        self.stockFromExisting = None
        self.stockCreateBox = None
        self.stockCreateCylinder = None
        self.stockEdit = None

        self.setupGlobal = PathSetupSheetGui.GlobalEditor(
            self.obj.SetupSheet, self.form
        )
        self.setupOps = PathSetupSheetGui.OpsDefaultEditor(
            self.obj.SetupSheet, self.form
        )

    def populateCombobox(self, form, enumTups, comboBoxesPropertyMap):
        """populateCombobox(form, enumTups, comboBoxesPropertyMap) ... populate comboboxes with translated enumerations
        ** comboBoxesPropertyMap will be unnecessary if UI files use strict combobox naming protocol.
        Args:
            form = UI form
            enumTups = list of (translated_text, data_string) tuples
            comboBoxesPropertyMap = list of (translated_text, data_string) tuples
        """
        # Load appropriate enumerations in each combobox
        for cb, prop in comboBoxesPropertyMap:
            box = getattr(form, cb)  # Get the combobox
            box.clear()  # clear the combobox
            for text, data in enumTups[prop]:  #  load enumerations
                box.addItem(text, data)

    def preCleanup(self):
        Path.Log.track()
        FreeCADGui.Selection.removeObserver(self)
        self.vproxy.resetEditVisibility(self.obj)
        self.vproxy.resetTaskPanel()

    def accept(self, resetEdit=True):
        Path.Log.track()
        self._jobIntegrityCheck()  # Check existence of Model and Tools
        self.preCleanup()
        self.getFields()
        self.setupGlobal.accept()
        self.setupOps.accept()
        FreeCAD.ActiveDocument.commitTransaction()
        self.cleanup(resetEdit)

    def reject(self, resetEdit=True):
        Path.Log.track()
        self.preCleanup()
        self.setupGlobal.reject()
        self.setupOps.reject()
        FreeCAD.ActiveDocument.abortTransaction()
        if self.deleteOnReject and FreeCAD.ActiveDocument.getObject(self.name):
            Path.Log.info("Uncreate Job")
            FreeCAD.ActiveDocument.openTransaction("Uncreate Job")
            if self.obj.ViewObject.Proxy.onDelete(self.obj.ViewObject, None):
                FreeCAD.ActiveDocument.removeObject(self.obj.Name)
            FreeCAD.ActiveDocument.commitTransaction()
        else:
            Path.Log.track(
                self.name,
                self.deleteOnReject,
                FreeCAD.ActiveDocument.getObject(self.name),
            )
        self.cleanup(resetEdit)
        return True

    def cleanup(self, resetEdit):
        Path.Log.track()
        FreeCADGui.Control.closeDialog()
        if resetEdit:
            FreeCADGui.ActiveDocument.resetEdit()
        FreeCAD.ActiveDocument.recompute()

    def updateTooltips(self):
        if (
            hasattr(self.obj, "Proxy")
            and hasattr(self.obj.Proxy, "tooltip")
            and self.obj.Proxy.tooltip
        ):
            self.form.postProcessor.setToolTip(self.obj.Proxy.tooltip)
            if hasattr(self.obj.Proxy, "tooltipArgs") and self.obj.Proxy.tooltipArgs:
                self.form.postProcessorArguments.setToolTip(self.obj.Proxy.tooltipArgs)
            else:
                self.form.postProcessorArguments.setToolTip(
                    self.postProcessorArgsDefaultTooltip
                )
        else:
            self.form.postProcessor.setToolTip(self.postProcessorDefaultTooltip)
            self.form.postProcessorArguments.setToolTip(
                self.postProcessorArgsDefaultTooltip
            )

    def getFields(self):
        """sets properties in the object to match the form"""
        if self.obj:
            self.obj.PostProcessor = str(self.form.postProcessor.currentText())
            self.obj.PostProcessorArgs = str(
                self.form.postProcessorArguments.displayText()
            )
            self.obj.PostProcessorOutputFile = str(
                self.form.postProcessorOutputFile.text()
            )

            self.obj.Label = str(self.form.jobLabel.text())
            self.obj.Description = str(self.form.jobDescription.toPlainText())
            self.obj.Operations.Group = [
                self.form.operationsList.item(i).data(self.DataObject)
                for i in range(self.form.operationsList.count())
            ]
            try:
                self.obj.SplitOutput = self.form.splitOutput.isChecked()
                self.obj.OrderOutputBy = str(self.form.orderBy.currentData())

                flist = []
                for i in range(self.form.wcslist.count()):
                    if (
                        self.form.wcslist.item(i).checkState()
                        == QtCore.Qt.CheckState.Checked
                    ):
                        flist.append(self.form.wcslist.item(i).text())
                self.obj.Fixtures = flist
            except Exception as e:
                Path.Log.debug(e)
                FreeCAD.Console.PrintWarning(
                    "The Job was created without fixture support.  Please delete and recreate the job\r\n"
                )

            self.updateTooltips()
            self.stockEdit.getFields(self.obj)

            self.obj.Proxy.execute(self.obj)

        self.setupGlobal.getFields()
        self.setupOps.getFields()

    def selectComboBoxText(self, widget, text):
        index = widget.findText(text, QtCore.Qt.MatchFixedString)
        if index >= 0:
            widget.blockSignals(True)
            widget.setCurrentIndex(index)
            widget.blockSignals(False)

    def updateToolController(self):
        tcRow = self.form.toolControllerList.currentRow()
        tcCol = self.form.toolControllerList.currentColumn()

        self.form.toolControllerList.blockSignals(True)
        self.form.toolControllerList.clearContents()
        self.form.toolControllerList.setRowCount(0)

        self.form.activeToolController.blockSignals(True)
        index = self.form.activeToolController.currentIndex()
        select = None if index == -1 else self.form.activeToolController.itemData(index)
        self.form.activeToolController.clear()

        vUnit = FreeCAD.Units.Quantity(1, FreeCAD.Units.Velocity).getUserPreferred()[2]

        for row, tc in enumerate(sorted(self.obj.Tools.Group, key=lambda tc: tc.Label)):
            self.form.activeToolController.addItem(tc.Label, tc)
            if tc == select:
                index = row

            self.form.toolControllerList.insertRow(row)

            item = QtGui.QTableWidgetItem(tc.Label)
            item.setData(self.DataObject, tc)
            item.setData(self.DataProperty, "Label")
            self.form.toolControllerList.setItem(row, 0, item)

            item = QtGui.QTableWidgetItem("%d" % tc.ToolNumber)
            item.setTextAlignment(QtCore.Qt.AlignRight)
            item.setData(self.DataObject, tc)
            item.setData(self.DataProperty, "Number")
            self.form.toolControllerList.setItem(row, 1, item)

            item = QtGui.QTableWidgetItem("%g" % tc.HorizFeed.getValueAs(vUnit))
            item.setTextAlignment(QtCore.Qt.AlignRight)
            item.setData(self.DataObject, tc)
            item.setData(self.DataProperty, "HorizFeed")
            self.form.toolControllerList.setItem(row, 2, item)

            item = QtGui.QTableWidgetItem("%g" % tc.VertFeed.getValueAs(vUnit))
            item.setTextAlignment(QtCore.Qt.AlignRight)
            item.setData(self.DataObject, tc)
            item.setData(self.DataProperty, "VertFeed")
            self.form.toolControllerList.setItem(row, 3, item)

            item = QtGui.QTableWidgetItem(
                "%s%g" % ("+" if tc.SpindleDir == "Forward" else "-", tc.SpindleSpeed)
            )
            item.setTextAlignment(QtCore.Qt.AlignRight)
            item.setData(self.DataObject, tc)
            item.setData(self.DataProperty, "Spindle")
            self.form.toolControllerList.setItem(row, 4, item)

        if index != -1:
            self.form.activeToolController.setCurrentIndex(index)
        if tcRow != -1 and tcCol != -1:
            self.form.toolControllerList.setCurrentCell(tcRow, tcCol)

        self.form.activeToolController.blockSignals(False)
        self.form.toolControllerList.blockSignals(False)

    def setFields(self):
        """sets fields in the form to match the object"""

        self.form.jobLabel.setText(self.obj.Label)
        self.form.jobDescription.setPlainText(self.obj.Description)

        if hasattr(self.obj, "SplitOutput"):
            self.form.splitOutput.setChecked(self.obj.SplitOutput)
        if hasattr(self.obj, "OrderOutputBy"):
            self.selectComboBoxText(self.form.orderBy, self.obj.OrderOutputBy)

        if hasattr(self.obj, "Fixtures"):
            for f in self.obj.Fixtures:
                item = self.form.wcslist.findItems(f, QtCore.Qt.MatchExactly)[0]
                item.setCheckState(QtCore.Qt.Checked)

        self.form.postProcessorOutputFile.setText(self.obj.PostProcessorOutputFile)
        self.selectComboBoxText(self.form.postProcessor, self.obj.PostProcessor)
        self.form.postProcessorArguments.setText(self.obj.PostProcessorArgs)
        # self.obj.Proxy.onChanged(self.obj, "PostProcessor")
        self.updateTooltips()

        self.form.operationsList.clear()
        for child in self.obj.Operations.Group:
            item = QtGui.QListWidgetItem(child.Label)
            item.setData(self.DataObject, child)
            self.form.operationsList.addItem(item)

        self.form.jobModel.clear()
        for name, count in Counter([
                    self.obj.Proxy.baseObject(self.obj, o).Label
                    for o in self.obj.Model.Group
            ]).items():
            if count == 1:
                self.form.jobModel.addItem(name)
            else:
                self.form.jobModel.addItem("%s (%d)" % (name, count))

        self.updateToolController()
        self.stockEdit.setFields(self.obj)
        self.setupGlobal.setFields()
        self.setupOps.setFields()

    def setPostProcessorOutputFile(self):
        filename = QtGui.QFileDialog.getSaveFileName(
            self.form,
            translate("Path_Job", "Select Output File"),
            None,
            translate("Path_Job", "All Files (*.*)"),
        )
        if filename and filename[0]:
            self.obj.PostProcessorOutputFile = str(filename[0])
            self.setFields()

    def operationSelect(self):
        if self.form.operationsList.selectedItems():
            self.form.operationModify.setEnabled(True)
            self.form.operationMove.setEnabled(True)
            row = self.form.operationsList.currentRow()
            self.form.operationUp.setEnabled(row > 0)
            self.form.operationDown.setEnabled(
                row < self.form.operationsList.count() - 1
            )
        else:
            self.form.operationModify.setEnabled(False)
            self.form.operationMove.setEnabled(False)

    def objectDelete(self, widget):
        for item in widget.selectedItems():
            obj = item.data(self.DataObject)
            if (
                obj.ViewObject
                and hasattr(obj.ViewObject, "Proxy")
                and hasattr(obj.ViewObject.Proxy, "onDelete")
            ):
                obj.ViewObject.Proxy.onDelete(obj.ViewObject, None)
            FreeCAD.ActiveDocument.removeObject(obj.Name)
        self.setFields()

    def operationDelete(self):
        self.objectDelete(self.form.operationsList)

    def operationMoveUp(self):
        row = self.form.operationsList.currentRow()
        if row > 0:
            item = self.form.operationsList.takeItem(row)
            self.form.operationsList.insertItem(row - 1, item)
            self.form.operationsList.setCurrentRow(row - 1)
            self.getFields()

    def operationMoveDown(self):
        row = self.form.operationsList.currentRow()
        if row < self.form.operationsList.count() - 1:
            item = self.form.operationsList.takeItem(row)
            self.form.operationsList.insertItem(row + 1, item)
            self.form.operationsList.setCurrentRow(row + 1)
            self.getFields()

    def toolControllerSelect(self):
        def canDeleteTC(tc):
            # if the TC is referenced anywhere but the job we don't want to delete it
            return len(tc.InList) == 1

        # if anything is selected it can be edited
        edit = True if self.form.toolControllerList.selectedItems() else False
        self.form.toolControllerEdit.setEnabled(edit)

        # can only delete what is selected
        delete = edit
        # ... but we want to make sure there's at least one TC left
        if len(self.obj.Tools.Group) == len(
            self.form.toolControllerList.selectedItems()
        ):
            delete = False
        # ... also don't want to delete any TCs that are already used
        if delete:
            for item in self.form.toolControllerList.selectedItems():
                if not canDeleteTC(item.data(self.DataObject)):
                    delete = False
                    break
        self.form.toolControllerDelete.setEnabled(delete)

    def toolControllerEdit(self):
        for item in self.form.toolControllerList.selectedItems():
            tc = item.data(self.DataObject)
            dlg = PathToolControllerGui.DlgToolControllerEdit(tc)
            dlg.exec_()
        self.setFields()
        self.toolControllerSelect()

    def toolControllerAdd(self):
        # adding a TC from a toolbit directly.
        # Try to find a tool number from the currently selected lib. Otherwise
        # use next available number

        tools = PathToolBitGui.LoadTools()

        curLib = Path.Preferences.lastFileToolLibrary()

        library = None
        if curLib is not None:
            with open(curLib) as fp:
                library = json.load(fp)

        for tool in tools:
            toolNum = self.obj.Proxy.nextToolNumber()
            if library is not None:
                for toolBit in library["tools"]:

                    if toolBit["path"] == tool.File:
                        toolNum = toolBit["nr"]

            tc = PathToolControllerGui.Create(
                name=tool.Label, tool=tool, toolNumber=toolNum
            )
            self.obj.Proxy.addToolController(tc)

        FreeCAD.ActiveDocument.recompute()
        self.updateToolController()

    def toolControllerDelete(self):
        self.objectDelete(self.form.toolControllerList)

    def toolControllerChanged(self, item):
        tc = item.data(self.DataObject)
        prop = item.data(self.DataProperty)
        if "Label" == prop:
            tc.Label = item.text()
            item.setText(tc.Label)
        elif "Number" == prop:
            try:
                tc.ToolNumber = int(item.text())
            except Exception:
                pass
            item.setText("%d" % tc.ToolNumber)
        elif "Spindle" == prop:
            try:
                speed = float(item.text())
                rot = "Forward"
                if speed < 0:
                    rot = "Reverse"
                    speed = -speed
                tc.SpindleDir = rot
                tc.SpindleSpeed = speed
            except Exception:
                pass
            item.setText(
                "%s%g" % ("+" if tc.SpindleDir == "Forward" else "-", tc.SpindleSpeed)
            )
        elif "HorizFeed" == prop or "VertFeed" == prop:
            vUnit = FreeCAD.Units.Quantity(
                1, FreeCAD.Units.Velocity
            ).getUserPreferred()[2]
            try:
                val = FreeCAD.Units.Quantity(item.text())
                if FreeCAD.Units.Velocity == val.Unit:
                    setattr(tc, prop, val)
                elif FreeCAD.Units.Unit() == val.Unit:
                    val = FreeCAD.Units.Quantity(item.text() + vUnit)
                    setattr(tc, prop, val)
            except Exception:
                pass
            item.setText("%g" % getattr(tc, prop).getValueAs(vUnit))
        else:
            try:
                val = FreeCAD.Units.Quantity(item.text())
                setattr(tc, prop, val)
            except Exception:
                pass
            item.setText("%g" % getattr(tc, prop).Value)

        self.template.updateUI()

    def modelSetAxis(self, axis):
        Path.Log.track(axis)

        def alignSel(sel, normal, flip=False):
            Path.Log.track(
                "Vector(%.2f, %.2f, %.2f)" % (normal.x, normal.y, normal.z), flip
            )
            v = axis
            if flip:
                v = axis.negative()

            if Path.Geom.pointsCoincide(abs(v), abs(normal)):
                # Selection is already aligned with the axis of rotation leading
                # to a (0,0,0) cross product for rotation.
                # --> Need to flip the object around one of the "other" axis.
                # Simplest way to achieve that is to rotate the coordinate system
                # of the axis and use that to rotate the object.
                r = FreeCAD.Vector(v.y, v.z, v.x)
                a = 180
            else:
                r = v.cross(normal)  # rotation axis
                a = DraftVecUtils.angle(normal, v, r) * 180 / math.pi
            Path.Log.debug(
                "oh boy: (%.2f, %.2f, %.2f) x (%.2f, %.2f, %.2f) -> (%.2f, %.2f, %.2f) -> %.2f"
                % (v.x, v.y, v.z, normal.x, normal.y, normal.z, r.x, r.y, r.z, a)
            )
            Draft.rotate(sel.Object, a, axis=r)

        selObject = None
        selFeature = None
        with selectionEx() as selection:
            for sel in selection:
                selObject = sel.Object
                for feature in sel.SubElementNames:
                    selFeature = feature
                    Path.Log.track(selObject.Label, feature)
                    sub = sel.Object.Shape.getElement(feature)

                    if "Face" == sub.ShapeType:
                        normal = sub.normalAt(0, 0)
                        if sub.Orientation == "Reversed":
                            normal = FreeCAD.Vector() - normal
                            Path.Log.debug(
                                "(%.2f, %.2f, %.2f) -> reversed (%s)"
                                % (normal.x, normal.y, normal.z, sub.Orientation)
                            )
                        else:
                            Path.Log.debug(
                                "(%.2f, %.2f, %.2f) -> forward  (%s)"
                                % (normal.x, normal.y, normal.z, sub.Orientation)
                            )

                        if Path.Geom.pointsCoincide(axis, normal):
                            alignSel(sel, normal, True)
                        elif Path.Geom.pointsCoincide(axis, FreeCAD.Vector() - normal):
                            alignSel(sel, FreeCAD.Vector() - normal, True)
                        else:
                            alignSel(sel, normal)

                    elif "Edge" == sub.ShapeType:
                        normal = (
                            sub.Vertexes[1].Point - sub.Vertexes[0].Point
                        ).normalize()
                        if Path.Geom.pointsCoincide(
                            axis, normal
                        ) or Path.Geom.pointsCoincide(axis, FreeCAD.Vector() - normal):
                            # Don't really know the orientation of an edge, so let's just flip the object
                            # and if the user doesn't like it they can flip again
                            alignSel(sel, normal, True)
                        else:
                            alignSel(sel, normal)

                    else:
                        Path.Log.track(sub.ShapeType)

        if selObject and selFeature:
            FreeCADGui.Selection.clearSelection()
            FreeCADGui.Selection.addSelection(selObject, selFeature)

    def restoreSelection(self, selection):
        FreeCADGui.Selection.clearSelection()
        for sel in selection:
            FreeCADGui.Selection.addSelection(sel.Object, sel.SubElementNames)

    def modelSet0(self, axis):
        Path.Log.track(axis)
        with selectionEx() as selection:
            for sel in selection:
                selObject = sel.Object
                Path.Log.track(selObject.Label)
                for name in sel.SubElementNames:
                    Path.Log.track(selObject.Label, name)
                    feature = selObject.Shape.getElement(name)
                    bb = feature.BoundBox
                    offset = FreeCAD.Vector(
                        axis.x * bb.XMax, axis.y * bb.YMax, axis.z * bb.ZMax
                    )
                    Path.Log.track(feature.BoundBox.ZMax, offset)
                    p = selObject.Placement
                    p.move(offset)
                    selObject.Placement = p

                    if self.form.linkStockAndModel.isChecked():
                        # Also move the objects not selected
                        # if selection is not model, move the model too
                        # if the selection is not stock and there is a stock, move the stock too
                        for model in self.obj.Model.Group:
                            if model != selObject:
                                Draft.move(model, offset)
                            if selObject != self.obj.Stock and self.obj.Stock:
                                Draft.move(self.obj.Stock, offset)

    def modelMove(self, axis):
        scale = self.form.modelMoveValue.value()
        with selectionEx() as selection:
            for sel in selection:
                offset = axis * scale
                Draft.move(sel.Object, offset)

    def modelRotate(self, axis):
        angle = self.form.modelRotateValue.value()
        with selectionEx() as selection:
            if self.form.modelRotateCompound.isChecked() and len(selection) > 1:
                bb = PathStock.shapeBoundBox([sel.Object for sel in selection])
                for sel in selection:
                    Draft.rotate(sel.Object, angle, bb.Center, axis)
            else:
                for sel in selection:
                    Draft.rotate(
                        sel.Object, angle, sel.Object.Shape.BoundBox.Center, axis
                    )

    def alignSetOrigin(self):
        (obj, by) = self.alignMoveToOrigin()

        for base in self.obj.Model.Group:
            if base != obj:
                Draft.move(base, by)

        if obj != self.obj.Stock and self.obj.Stock:
            Draft.move(self.obj.Stock, by)

        placement = FreeCADGui.ActiveDocument.ActiveView.viewPosition()
        placement.Base = placement.Base + by
        FreeCADGui.ActiveDocument.ActiveView.viewPosition(placement, 0)

    def alignMoveToOrigin(self):
        selObject = None
        selFeature = None
        p = None
        for sel in FreeCADGui.Selection.getSelectionEx():
            selObject = sel.Object
            for feature in sel.SubElementNames:
                selFeature = feature
                sub = sel.Object.Shape.getElement(feature)
                if "Vertex" == sub.ShapeType:
                    p = FreeCAD.Vector() - sub.Point
                if "Edge" == sub.ShapeType:
                    p = FreeCAD.Vector() - sub.Curve.Location
                if "Face" == sub.ShapeType:
                    p = FreeCAD.Vector() - sub.BoundBox.Center

                if p:
                    Draft.move(sel.Object, p)

        if selObject and selFeature:
            FreeCADGui.Selection.clearSelection()
            FreeCADGui.Selection.addSelection(selObject, selFeature)
        return (selObject, p)

    def updateStockEditor(self, index, force=False):
        def setupFromBaseEdit():
            Path.Log.track(index, force)
            if force or not self.stockFromBase:
                self.stockFromBase = StockFromBaseBoundBoxEdit(
                    self.obj, self.form, force
                )
            self.stockEdit = self.stockFromBase

        def setupCreateBoxEdit():
            Path.Log.track(index, force)
            if force or not self.stockCreateBox:
                self.stockCreateBox = StockCreateBoxEdit(self.obj, self.form, force)
            self.stockEdit = self.stockCreateBox

        def setupCreateCylinderEdit():
            Path.Log.track(index, force)
            if force or not self.stockCreateCylinder:
                self.stockCreateCylinder = StockCreateCylinderEdit(
                    self.obj, self.form, force
                )
            self.stockEdit = self.stockCreateCylinder

        def setupFromExisting():
            Path.Log.track(index, force)
            if force or not self.stockFromExisting:
                self.stockFromExisting = StockFromExistingEdit(
                    self.obj, self.form, force
                )
            if self.stockFromExisting.candidates(self.obj):
                self.stockEdit = self.stockFromExisting
                return True
            return False

        if index == -1:
            if self.obj.Stock is None or StockFromBaseBoundBoxEdit.IsStock(self.obj):
                setupFromBaseEdit()
            elif StockCreateBoxEdit.IsStock(self.obj):
                setupCreateBoxEdit()
            elif StockCreateCylinderEdit.IsStock(self.obj):
                setupCreateCylinderEdit()
            elif StockFromExistingEdit.IsStock(self.obj):
                setupFromExisting()
            else:
                Path.Log.error(
                    translate("Path_Job", "Unsupported stock object %s")
                    % self.obj.Stock.Label
                )
        else:
            if index == StockFromBaseBoundBoxEdit.Index:
                setupFromBaseEdit()
            elif index == StockCreateBoxEdit.Index:
                setupCreateBoxEdit()
            elif index == StockCreateCylinderEdit.Index:
                setupCreateCylinderEdit()
            elif index == StockFromExistingEdit.Index:
                if not setupFromExisting():
                    setupFromBaseEdit()
                    index = -1
            else:
                Path.Log.error(
                    translate("Path_Job", "Unsupported stock type %s (%d)")
                    % (self.form.stock.currentText(), index)
                )
        self.stockEdit.activate(self.obj, index == -1)

        if -1 != index:
            self.template.updateUI()

    def refreshStock(self):
        self.updateStockEditor(self.form.stock.currentIndex(), True)

    def alignCenterInStock(self):
        bbs = self.obj.Stock.Shape.BoundBox
        for sel in FreeCADGui.Selection.getSelectionEx():
            bbb = sel.Object.Shape.BoundBox
            by = bbs.Center - bbb.Center
            Draft.move(sel.Object, by)

    def alignCenterInStockXY(self):
        bbs = self.obj.Stock.Shape.BoundBox
        for sel in FreeCADGui.Selection.getSelectionEx():
            bbb = sel.Object.Shape.BoundBox
            by = bbs.Center - bbb.Center
            by.z = 0
            Draft.move(sel.Object, by)

    def isValidDatumSelection(self, sel):
        if sel.ShapeType in ["Vertex", "Edge", "Face"]:
            if hasattr(sel, "Curve") and type(sel.Curve) not in [Part.Circle]:
                return False
            return True

        # no valid selection
        return False

    def isValidAxisSelection(self, sel):
        if sel.ShapeType in ["Vertex", "Edge", "Face"]:
            if hasattr(sel, "Curve") and type(sel.Curve) in [Part.Circle]:
                return False
            if hasattr(sel, "Surface") and sel.Surface.curvature(0, 0, "Max") != 0:
                return False
            return True

        # no valid selection
        return False

    def updateSelection(self):
        # Remove Job object if present in Selection: source of phantom paths
        if self.obj in FreeCADGui.Selection.getSelection():
            FreeCADGui.Selection.removeSelection(self.obj)

        sel = FreeCADGui.Selection.getSelectionEx()

        self.form.setOrigin.setEnabled(False)
        self.form.moveToOrigin.setEnabled(False)
        self.form.modelSetXAxis.setEnabled(False)
        self.form.modelSetYAxis.setEnabled(False)
        self.form.modelSetZAxis.setEnabled(False)

        if len(sel) == 1 and len(sel[0].SubObjects) == 1:
            subObj = sel[0].SubObjects[0]
            if self.isValidDatumSelection(subObj):
                self.form.setOrigin.setEnabled(True)
                self.form.moveToOrigin.setEnabled(True)
            if self.isValidAxisSelection(subObj):
                self.form.modelSetXAxis.setEnabled(True)
                self.form.modelSetYAxis.setEnabled(True)
                self.form.modelSetZAxis.setEnabled(True)

        if len(sel) == 0 or self.obj.Stock in [s.Object for s in sel]:
            self.form.centerInStock.setEnabled(False)
            self.form.centerInStockXY.setEnabled(False)
        else:
            self.form.centerInStock.setEnabled(True)
            self.form.centerInStockXY.setEnabled(True)

        if len(sel) > 0:
            self.form.modelSetX0.setEnabled(True)
            self.form.modelSetY0.setEnabled(True)
            self.form.modelSetZ0.setEnabled(True)
            self.form.modelMoveGroup.setEnabled(True)
            self.form.modelRotateGroup.setEnabled(True)
            self.form.modelRotateCompound.setEnabled(len(sel) > 1)
        else:
            self.form.modelSetX0.setEnabled(False)
            self.form.modelSetY0.setEnabled(False)
            self.form.modelSetZ0.setEnabled(False)
            self.form.modelMoveGroup.setEnabled(False)
            self.form.modelRotateGroup.setEnabled(False)

    def jobModelEdit(self):
        dialog = PathJobDlg.JobCreate()
        dialog.setupTitle(translate("Path_Job", "Model Selection"))
        dialog.setupModel(self.obj)
        if dialog.exec_() == 1:
            models = dialog.getModels()
            if models:
                obj = self.obj
                proxy = obj.Proxy

                want = Counter(models)
                have = Counter([proxy.baseObject(obj, o) for o in obj.Model.Group])

                obsolete = have - want
                additions = want - have

                # first remove all obsolete base models
                for model, count in obsolete.items():
                    for i in range(count):
                        # it seems natural to remove the last of all the base objects for a given model
                        base = [
                            b
                            for b in obj.Model.Group
                            if proxy.baseObject(obj, b) == model
                        ][-1]
                        self.vproxy.forgetBaseVisibility(obj, base)
                        self.obj.Proxy.removeBase(obj, base, True)
                # do not access any of the retired objects after this point, they don't exist anymore

                # then add all rookie base models
                for model, count in additions.items():
                    for i in range(count):
                        base = PathJob.createModelResourceClone(obj, model)
                        obj.Model.addObject(base)
                        self.vproxy.rememberBaseVisibility(obj, base)

                # refresh the view
                if obsolete or additions:
                    self.setFields()
                else:
                    Path.Log.track("no changes to model")

    def tabPageChanged(self, index):
        if index == 0:
            # update the template with potential changes
            self.getFields()
            self.setupGlobal.accept()
            self.setupOps.accept()
            self.obj.Document.recompute()
            self.template.updateUI()

    def setupUi(self, activate):
        self.setupGlobal.setupUi()
        try:
            self.setupOps.setupUi()
        except Exception as ee:
            Path.Log.error(str(ee))
        self.updateStockEditor(-1, False)
        self.setFields()

        # Info
        self.form.jobLabel.editingFinished.connect(self.getFields)
        self.form.jobModelEdit.clicked.connect(self.jobModelEdit)

        # Post Processor
        self.form.postProcessor.currentIndexChanged.connect(self.getFields)
        self.form.postProcessorArguments.editingFinished.connect(self.getFields)
        self.form.postProcessorOutputFile.editingFinished.connect(self.getFields)
        self.form.postProcessorSetOutputFile.clicked.connect(
            self.setPostProcessorOutputFile
        )

        # Workplan
        self.form.operationsList.itemSelectionChanged.connect(self.operationSelect)
        self.form.operationsList.indexesMoved.connect(self.getFields)
        self.form.operationDelete.clicked.connect(self.operationDelete)
        self.form.operationUp.clicked.connect(self.operationMoveUp)
        self.form.operationDown.clicked.connect(self.operationMoveDown)

        self.form.operationEdit.hide()  # not supported yet
        self.form.activeToolGroup.hide()  # not supported yet

        # Tool controller
        self.form.toolControllerList.itemSelectionChanged.connect(
            self.toolControllerSelect
        )
        self.form.toolControllerList.itemChanged.connect(self.toolControllerChanged)
        self.form.toolControllerEdit.clicked.connect(self.toolControllerEdit)
        self.form.toolControllerDelete.clicked.connect(self.toolControllerDelete)
        self.form.toolControllerAdd.clicked.connect(self.toolControllerAdd)

        self.operationSelect()
        self.toolControllerSelect()

        # Stock, Orientation and Alignment
        self.form.centerInStock.clicked.connect(self.alignCenterInStock)
        self.form.centerInStockXY.clicked.connect(self.alignCenterInStockXY)

        self.form.stock.currentIndexChanged.connect(self.updateStockEditor)
        self.form.refreshStock.clicked.connect(self.refreshStock)

        self.form.modelSetXAxis.clicked.connect(
            lambda: self.modelSetAxis(FreeCAD.Vector(1, 0, 0))
        )
        self.form.modelSetYAxis.clicked.connect(
            lambda: self.modelSetAxis(FreeCAD.Vector(0, 1, 0))
        )
        self.form.modelSetZAxis.clicked.connect(
            lambda: self.modelSetAxis(FreeCAD.Vector(0, 0, 1))
        )
        self.form.modelSetX0.clicked.connect(
            lambda: self.modelSet0(FreeCAD.Vector(-1, 0, 0))
        )
        self.form.modelSetY0.clicked.connect(
            lambda: self.modelSet0(FreeCAD.Vector(0, -1, 0))
        )
        self.form.modelSetZ0.clicked.connect(
            lambda: self.modelSet0(FreeCAD.Vector(0, 0, -1))
        )

        self.form.setOrigin.clicked.connect(self.alignSetOrigin)
        self.form.moveToOrigin.clicked.connect(self.alignMoveToOrigin)

        self.form.modelMoveLeftUp.clicked.connect(
            lambda: self.modelMove(FreeCAD.Vector(-1, 1, 0))
        )
        self.form.modelMoveLeft.clicked.connect(
            lambda: self.modelMove(FreeCAD.Vector(-1, 0, 0))
        )
        self.form.modelMoveLeftDown.clicked.connect(
            lambda: self.modelMove(FreeCAD.Vector(-1, -1, 0))
        )

        self.form.modelMoveUp.clicked.connect(
            lambda: self.modelMove(FreeCAD.Vector(0, 1, 0))
        )
        self.form.modelMoveDown.clicked.connect(
            lambda: self.modelMove(FreeCAD.Vector(0, -1, 0))
        )

        self.form.modelMoveRightUp.clicked.connect(
            lambda: self.modelMove(FreeCAD.Vector(1, 1, 0))
        )
        self.form.modelMoveRight.clicked.connect(
            lambda: self.modelMove(FreeCAD.Vector(1, 0, 0))
        )
        self.form.modelMoveRightDown.clicked.connect(
            lambda: self.modelMove(FreeCAD.Vector(1, -1, 0))
        )

        self.form.modelRotateLeft.clicked.connect(
            lambda: self.modelRotate(FreeCAD.Vector(0, 0, 1))
        )
        self.form.modelRotateRight.clicked.connect(
            lambda: self.modelRotate(FreeCAD.Vector(0, 0, -1))
        )

        self.updateSelection()

        # set active page
        if activate in ["General", "Model"]:
            self.form.setCurrentIndex(0)
        if activate in ["Output", "Post Processor"]:
            self.form.setCurrentIndex(1)
        if activate in ["Layout", "Stock"]:
            self.form.setCurrentIndex(2)
        if activate in ["Tools", "Tool Controller"]:
            self.form.setCurrentIndex(3)
        if activate in ["Workplan", "Operations"]:
            self.form.setCurrentIndex(4)

        self.form.currentChanged.connect(self.tabPageChanged)
        self.template.exportButton().clicked.connect(self.templateExport)

    def templateExport(self):
        self.getFields()
        PathJobCmd.CommandJobTemplateExport.SaveDialog(self.obj, self.template)

    def open(self):
        FreeCADGui.Selection.addObserver(self)

    def _jobIntegrityCheck(self):
        """_jobIntegrityCheck() ... Check Job object for existence of Model and Tools
        If either Model or Tools is empty, change GUI tab, issue appropriate warning,
        and offer chance to add appropriate item."""

        def _displayWarningWindow(msg):
            """Display window with warning message and Add action button.
            Return action state."""
            txtHeader = translate("Path_Job", "Warning")
            txtPleaseAddOne = translate("Path_Job", "Please add one.")
            txtOk = translate("Path_Job", "Ok")
            txtAdd = translate("Path_Job", "Add")

            msgbox = QtGui.QMessageBox(
                QtGui.QMessageBox.Warning, txtHeader, msg + " " + txtPleaseAddOne
            )
            msgbox.addButton(txtOk, QtGui.QMessageBox.AcceptRole)  # Add 'Ok' button
            msgbox.addButton(txtAdd, QtGui.QMessageBox.ActionRole)  # Add 'Add' button
            return msgbox.exec_()

        # Check if at least on base model is present
        if len(self.obj.Model.Group) == 0:
            self.form.setCurrentIndex(0)  # Change tab to General tab
            no_model_txt = translate("Path_Job", "This job has no base model.")
            if _displayWarningWindow(no_model_txt) == 1:
                self.jobModelEdit()

        # Check if at least one tool is present
        if len(self.obj.Tools.Group) == 0:
            self.form.setCurrentIndex(3)  # Change tab to Tools tab
            no_tool_txt = translate("Path_Job", "This job has no tool.")
            if _displayWarningWindow(no_tool_txt) == 1:
                self.toolControllerAdd()

    # SelectionObserver interface
    def addSelection(self, doc, obj, sub, pnt):
        self.updateSelection()

    def removeSelection(self, doc, obj, sub):
        self.updateSelection()

    def setSelection(self, doc):
        self.updateSelection()

    def clearSelection(self, doc):
        self.updateSelection()


def Create(base, template=None, openTaskPanel=True):
    """Create(base, template) ... creates a job instance for the given base object
    using template to configure it."""
    FreeCADGui.addModule("Path.Main.Job")
    FreeCAD.ActiveDocument.openTransaction("Create Job")
    try:
        obj = PathJob.Create("Job", base, template)
        obj.ViewObject.Proxy = ViewProvider(obj.ViewObject)
        obj.ViewObject.addExtension("Gui::ViewProviderGroupExtensionPython")
        FreeCAD.ActiveDocument.commitTransaction()
        obj.Document.recompute()
        if openTaskPanel:
            obj.ViewObject.Proxy.editObject(obj.Stock)
        else:
            obj.ViewObject.Proxy.deleteOnReject = False
        return obj
    except Exception as exc:
        Path.Log.error(exc)
        traceback.print_exc()
        FreeCAD.ActiveDocument.abortTransaction()


# make sure the UI has been initialized
PathGuiInit.Startup()
