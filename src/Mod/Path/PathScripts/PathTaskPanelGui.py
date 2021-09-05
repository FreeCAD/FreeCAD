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

import FreeCAD
import FreeCADGui
import PathScripts.PathGeom as PathGeom
import PathScripts.PathGetPoint as PathGetPoint
import PathScripts.PathGui as PathGui
import PathScripts.PathJob as PathJob
import PathScripts.PathLog as PathLog
import PathScripts.operations.PathOp2 as PathOp2
import PathScripts.PathPreferences as PathPreferences
import PathScripts.PathSelection as PathSelection
import PathScripts.PathUtil as PathUtil
import PathScripts.PathUtils as PathUtils
from pivy import coin

from PySide import QtCore, QtGui

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Part = LazyLoader("Part", globals(), "Part")
FeatureExtensions = LazyLoader(
    "PathScripts.PathFeatureExtensions", globals(), "PathScripts.PathFeatureExtensions"
)


__title__ = "Path UI Task Panel Pages base classes"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecadweb.org"
__doc__ = "Base classes for UI features within Path operations"


PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule(PathLog.thisModule())


def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class TaskPanel(object):
    """
    Generic TaskPanel implementation handling the standard Path operation layout.
    This class only implements the framework and takes care of bringing all pages up and down in a controller fashion.
    It implements the standard editor behaviour for OK, Cancel and Apply and tracks if the model is still in sync with
    the UI.
    However, all display and processing of fields is handled by the page controllers which are managed in a list. All
    event callbacks and framework actions are forwarded to the page controllers in turn and each page controller is
    expected to process all events concerning the data it manages.
    """

    def __init__(self, obj, deleteOnReject, opPage, selectionFactory, parent=None):
        PathLog.track(obj.Label, deleteOnReject, opPage, selectionFactory)
        FreeCAD.ActiveDocument.openTransaction(translate("Path", "AreaOp Operation"))
        self.obj = obj
        self.deleteOnReject = deleteOnReject
        self.featurePages = dict()
        self.parent = parent
        self.opPage = opPage

        # Working shape preview requirements
        self.switch = coin.SoSwitch()  # pylint: disable=attribute-defined-outside-init
        self.obj.ViewObject.RootNode.addChild(self.switch)
        self.switch.whichChild = coin.SO_SWITCH_ALL

        # members initialized later
        self.clearanceHeight = None
        self.safeHeight = None
        self.startDepth = None
        self.finishDepth = None
        self.finalDepth = None
        self.stepDown = None
        self.buttonBox = None
        self.minDiameter = None
        self.maxDiameter = None

        self._initOpAndFeaturePages(opPage, obj)
        self._applyLayoutToPages(opPage, obj)

        self.selectionFactory = selectionFactory
        self.isdirty = deleteOnReject
        self.visibility = obj.ViewObject.Visibility
        obj.ViewObject.Visibility = True

    def _initOpAndFeaturePages(self, opPage, obj):
        """_initOpAndFeaturePages(opPage, obj) ... Compile and initialize the operation page and required feature pages."""
        features = obj.Proxy.opFeatures(obj)
        opPage.features = features

        if PathOp2.FeatureBaseGeometry & features:
            if hasattr(opPage, "taskPanelBaseGeometryPage"):
                page = opPage.taskPanelBaseGeometryPage(obj, features)
            else:
                page = TaskPanelBaseGeometryPage(obj, features)
            self.featurePages["BaseGeometry"] = page

        if PathOp2.FeatureLocations & features:
            if hasattr(opPage, "taskPanelBaseLocationPage"):
                page = opPage.taskPanelBaseLocationPage(obj, features)
            else:
                page = TaskPanelBaseLocationPage(obj, features)
            self.featurePages["Locations"] = page

        if PathOp2.FeatureHeightsDepths & features:
            if hasattr(opPage, "taskPanelHeightsDepthsPage"):
                page = opPage.taskPanelHeightsDepthsPage(obj, features)
            else:
                page = TaskPanelHeightsDepthsPage(obj, features)
            self.featurePages["Locations"] = page

        if PathOp2.FeatureDiameters & features:
            if hasattr(opPage, "taskPanelDiametersPage"):
                page = opPage.taskPanelDiametersPage(obj, features)
            else:
                page = TaskPanelDiametersPage(obj, features)
            self.featurePages["Diameters"] = page

        if PathOp2.FeatureExtensions & features:
            if hasattr(opPage, "taskPanelExtensionPage"):
                page = opPage.taskPanelExtensionPage(obj, features)
            else:
                page = TaskPanelExtensionPage(obj, features)
            self.featurePages["Extensions"] = page

        self.featurePages["Operation"] = opPage

        for page in self.featurePages.values():
            page.parent = self  # save pointer to this current class as "parent"
            page.initPage(obj)
            page.onDirtyChanged(self.pageDirtyChanged)

    def _applyLayoutToPages(self, opPage, obj):
        """_applyLayoutToPages(opPage, obj) ... Apply user preferred layout to op and feature pages."""
        taskPanelLayout = PathPreferences.defaultTaskPanelLayout()
        featurePages = self.featurePages.values()

        if taskPanelLayout < 2:
            opTitle = opPage.getTitle(obj)
            opPage.setTitle(translate("PathOp2", "Operation"))
            toolbox = QtGui.QToolBox()
            if taskPanelLayout == 0:
                for page in featurePages:
                    toolbox.addItem(page.form, page.getTitle(obj))
                    itemIdx = toolbox.count() - 1
                    if page.icon:
                        toolbox.setItemIcon(itemIdx, QtGui.QIcon(page.icon))
                toolbox.setCurrentIndex(len(featurePages) - 1)
            else:
                for page in reversed(featurePages):
                    toolbox.addItem(page.form, page.getTitle(obj))
                    itemIdx = toolbox.count() - 1
                    if page.icon:
                        toolbox.setItemIcon(itemIdx, QtGui.QIcon(page.icon))
            toolbox.setWindowTitle(opTitle)
            if opPage.getIcon(obj):
                toolbox.setWindowIcon(QtGui.QIcon(opPage.getIcon(obj)))

            self.form = toolbox
        elif taskPanelLayout == 2:
            forms = []
            for page in featurePages:
                page.form.setWindowTitle(page.getTitle(obj))
                forms.append(page.form)
            self.form = forms
        elif taskPanelLayout == 3:
            forms = []
            for page in reversed(featurePages):
                page.form.setWindowTitle(page.getTitle(obj))
                forms.append(page.form)
            self.form = forms

    def isDirty(self):
        """isDirty() ... returns true if the model is not in sync with the UI anymore."""
        for page in self.featurePages.values():
            if page.isdirty:
                return True
        return self.isdirty

    def setClean(self):
        """setClean() ... set the receiver and all its pages clean."""
        self.isdirty = False
        for page in self.featurePages.values():
            page.setClean()

    def accept(self, resetEdit=True):
        """accept() ... callback invoked when user presses the task panel OK button."""
        self.preCleanup()
        if self.isDirty():
            self.panelGetFields()
        FreeCAD.ActiveDocument.commitTransaction()
        self.cleanup(resetEdit)

    def reject(self, resetEdit=True):
        """reject() ... callback invoked when user presses the task panel Cancel button."""
        self.preCleanup()
        FreeCAD.ActiveDocument.abortTransaction()
        self.cleanup(resetEdit)
        if self.deleteOnReject:
            FreeCAD.ActiveDocument.openTransaction(
                translate("Path", "Uncreate AreaOp Operation")
            )
            try:
                PathUtil.clearExpressionEngine(self.obj)
                FreeCAD.ActiveDocument.removeObject(self.obj.Name)
            except Exception as ee:
                PathLog.debug("{}\n".format(ee))
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
        return True

    def helpRequested(self):
        """helpRequested() ... callback invoked when user presses the task panel standard Help button."""
        # PathLog.debug("helpRequested()")
        self.setClean()
        pass

    def preCleanup(self):
        """
        for page in self.featurePages:
            page.onDirtyChanged(None)
        PathSelection.clear()
        FreeCADGui.Selection.removeObserver(self)
        self.obj.ViewObject.Proxy.clearTaskPanel()
        self.obj.ViewObject.Visibility = self.visibility
        """

        # Clean-up feature pages in Task Panel
        for page in self.featurePages.values():
            page.onDirtyChanged(None)

        # Cleanup working shape previews
        removeSwitch = False
        page = self.featurePages["Operation"]
        if page.targetShapeList and self.switch:
            for (__, __, ds) in page.targetShapeList:
                self.switch.removeChild(ds.root)
            removeSwitch = True

        # Remove Coin3D switch object
        if removeSwitch:
            if hasattr(self, "obj") and self.obj:
                self.obj.ViewObject.RootNode.removeChild(self.switch)

        PathSelection.clear()
        FreeCADGui.Selection.removeObserver(self)
        self.obj.ViewObject.Proxy.clearTaskPanel()
        self.obj.ViewObject.Visibility = self.visibility

    def cleanup(self, resetEdit):
        """cleanup() ... implements common cleanup tasks."""
        self.panelCleanup()
        FreeCADGui.Control.closeDialog()
        if resetEdit:
            FreeCADGui.ActiveDocument.resetEdit()
        FreeCAD.ActiveDocument.recompute()

    def pageDirtyChanged(self, page):
        """pageDirtyChanged(page) ... internal callback"""
        # pylint: disable=unused-argument
        self.buttonBox.button(QtGui.QDialogButtonBox.Apply).setEnabled(self.isDirty())

    def clicked(self, button):
        """clicked(button) ... callback invoked when the user presses any of the task panel buttons."""
        if button == QtGui.QDialogButtonBox.Apply:
            self.panelGetFields()
            self.setClean()
            FreeCAD.ActiveDocument.recompute()

    def modifyStandardButtons(self, buttonBox):
        """modifyStandarButtons(buttonBox) ... callback in case the task panel buttons need to be modified."""
        self.buttonBox = buttonBox
        for page in self.featurePages.values():
            page.modifyStandardButtons(buttonBox)
        self.pageDirtyChanged(None)

    def panelGetFields(self):
        """panelGetFields() ... invoked to trigger a complete transfer of UI data to the model."""
        PathLog.track()
        for page in self.featurePages.values():
            page.pageGetFields()

    def panelSetFields(self):
        """panelSetFields() ... invoked to trigger a complete transfer of the model's properties to the UI."""
        PathLog.track()
        self.obj.Proxy.sanitizeBase(self.obj)
        for page in self.featurePages.values():
            page.pageSetFields()

    def panelCleanup(self):
        """panelCleanup() ... invoked before the receiver is destroyed."""
        PathLog.track()
        for page in self.featurePages.values():
            page.pageCleanup()

    def open(self):
        """open() ... callback invoked when the task panel is opened."""
        self.selectionFactory()
        FreeCADGui.Selection.addObserver(self)

    def getStandardButtons(self):
        """getStandardButtons() ... returns the Buttons for the task panel."""
        return int(
            QtGui.QDialogButtonBox.Ok
            | QtGui.QDialogButtonBox.Apply
            | QtGui.QDialogButtonBox.Cancel
        )

    def setupUi(self):
        """setupUi() ... internal function to initialise all pages."""
        PathLog.track(self.deleteOnReject)

        if (
            self.deleteOnReject
            and PathOp2.FeatureBaseGeometry & self.obj.Proxy.opFeatures(self.obj)
        ):
            sel = FreeCADGui.Selection.getSelectionEx()
            for page in self.featurePages.values():
                if getattr(page, "InitBase", True) and hasattr(page, "addBase"):
                    page.clearBase()
                    page.addBaseGeometry(sel)

        # Update properties based upon expressions in case expression value has changed
        for (prp, expr) in self.obj.ExpressionEngine:
            val = FreeCAD.Units.Quantity(self.obj.evalExpression(expr))
            value = val.Value if hasattr(val, "Value") else val
            prop = getattr(self.obj, prp)
            if hasattr(prop, "Value"):
                prop.Value = value
            else:
                prop = value

        self.panelSetFields()

        for page in self.featurePages.values():
            page.pageRegisterSignalHandlers()

    def updateData(self, obj, prop):
        """updateDate(obj, prop) ... callback invoked whenever a model's property is assigned a value."""
        # PathLog.track(obj.Label, prop) # creates a lot of noise
        for page in self.featurePages.values():
            page.pageUpdateData(obj, prop)

    def needsFullSpace(self):
        return True

    def updateSelection(self):
        sel = FreeCADGui.Selection.getSelectionEx()
        for page in self.featurePages.values():
            page.updateSelection(self.obj, sel)

    # SelectionObserver interface
    def addSelection(self, doc, obj, sub, pnt):
        # pylint: disable=unused-argument
        self.updateSelection()

    def removeSelection(self, doc, obj, sub):
        # pylint: disable=unused-argument
        self.updateSelection()

    def setSelection(self, doc):
        # pylint: disable=unused-argument
        self.updateSelection()

    def clearSelection(self, doc):
        # pylint: disable=unused-argument
        self.updateSelection()


# Eclass


class TaskPanelPage(object):
    """Base class for all task panel pages."""

    # task panel interaction framework
    def __init__(self, obj, features):
        """__init__(obj, features) ... framework initialisation.
        Do not overwrite, implement initPage(obj) instead."""
        self.title = "-"
        self.obj = obj
        self.job = PathUtils.findParentJob(obj)
        self.form = self.getForm()  # pylint: disable=assignment-from-no-return
        self.signalDirtyChanged = None
        self.setClean()
        self.setTitle(self.title)
        self.setIcon(None)
        self.features = features
        self.isdirty = False
        self.parent = None

        # Instance variables for previewing working shapes
        self.targetShapeList = (
            list()
        )  # stores tuples of "(label, coinObj)" for removal shape preview

        if self._installTCUpdate():
            PathJob.Notification.updateTC.connect(self.resetToolController)

    def _installTCUpdate(self):
        return hasattr(self.form, "toolController")

    def onDirtyChanged(self, callback):
        """onDirtyChanged(callback) ... set callback when dirty state changes."""
        self.signalDirtyChanged = callback

    def setDirty(self):
        """setDirty() ... mark receiver as dirty, causing the model to be recalculated if OK or Apply is pressed."""
        self.isdirty = True
        if self.signalDirtyChanged:
            self.signalDirtyChanged(self)

    def setClean(self):
        """setClean() ... mark receiver as clean, indicating there is no need to recalculate the model even if the user presses OK or Apply."""
        self.isdirty = False
        if self.signalDirtyChanged:
            self.signalDirtyChanged(self)

    def pageGetFields(self):
        """pageGetFields() ... internal callback.
        Do not overwrite, implement getFields(obj) instead."""
        self.getFields(self.obj)
        self.setDirty()

    def pageSetFields(self):
        """pageSetFields() ... internal callback.
        Do not overwrite, implement setFields(obj) instead."""
        self.setFields(self.obj)

    def pageCleanup(self):
        """pageCleanup() ... internal callback.
        Do not overwrite, implement cleanupPage(obj) instead."""
        if self._installTCUpdate():
            PathJob.Notification.updateTC.disconnect(self.resetToolController)
        self.cleanupPage(self.obj)

    def pageRegisterSignalHandlers(self):
        """pageRegisterSignalHandlers() .. internal callback.
        Registers a callback for all signals returned by getSignalsForUpdate(obj).
        Do not overwrite, implement getSignalsForUpdate(obj) and/or registerSignalHandlers(obj) instead."""
        for signal in self.getSignalsForUpdate(self.obj):
            signal.connect(self.pageGetFields)
        self.registerSignalHandlers(self.obj)

    def pageUpdateData(self, obj, prop):
        """pageUpdateData(obj, prop) ... internal callback.
        Do not overwrite, implement updateData(obj) instead."""
        self.updateData(obj, prop)

    def setTitle(self, title):
        """setTitle(title) ... sets a title for the page."""
        self.title = title

    def getTitle(self, obj):
        """getTitle(obj) ... return title to be used for the receiver page.
        The default implementation returns what was previously set with setTitle(title).
        Can safely be overwritten by subclasses."""
        # pylint: disable=unused-argument
        return self.title

    def setIcon(self, icon):
        """setIcon(icon) ... sets the icon for the page."""
        self.icon = icon

    def getIcon(self, obj):
        """getIcon(obj) ... return icon for page or None.
        Can safely be overwritten by subclasses."""
        # pylint: disable=unused-argument
        return self.icon

    # helpers
    def selectInComboBox(self, name, combo):
        """selectInComboBox(name, combo) ...
        helper function to select a specific value in a combo box."""
        index = combo.findText(name, QtCore.Qt.MatchFixedString)
        if index >= 0:
            combo.blockSignals(True)
            combo.setCurrentIndex(index)
            combo.blockSignals(False)

    def resetToolController(self, job, tc):
        if self.obj is not None:
            self.obj.ToolController = tc
            combo = self.form.toolController
            self.setupToolController(self.obj, combo)

    def setupToolController(self, obj, combo):
        """setupToolController(obj, combo) ...
        helper function to setup obj's ToolController
        in the given combo box."""
        controllers = PathUtils.getToolControllers(self.obj)
        labels = [c.Label for c in controllers]
        combo.blockSignals(True)
        combo.clear()
        combo.addItems(labels)
        combo.blockSignals(False)

        if obj.ToolController is None:
            obj.ToolController = PathUtils.findToolController(obj, obj.Proxy)
        if obj.ToolController is not None:
            self.selectInComboBox(obj.ToolController.Label, combo)

    def updateToolController(self, obj, combo):
        """updateToolController(obj, combo) ...
        helper function to update obj's ToolController property if a different
        one has been selected in the combo box."""
        tc = PathUtils.findToolController(obj, obj.Proxy, combo.currentText())
        if obj.ToolController != tc:
            obj.ToolController = tc

    def setupCoolant(self, obj, combo):
        """setupCoolant(obj, combo) ...
        helper function to setup obj's Coolant option."""
        job = PathUtils.findParentJob(obj)
        options = job.SetupSheet.CoolantModes
        combo.blockSignals(True)
        combo.clear()
        combo.addItems(options)
        combo.blockSignals(False)

        if hasattr(obj, "CoolantMode"):
            self.selectInComboBox(obj.CoolantMode, combo)

    def updateCoolant(self, obj, combo):
        """updateCoolant(obj, combo) ...
        helper function to update obj's Coolant property if a different
        one has been selected in the combo box."""
        option = combo.currentText()
        if hasattr(obj, "CoolantMode"):
            if obj.CoolantMode != option:
                obj.CoolantMode = option

    def updatePanelVisibility(self, pageTitle, obj):
        """updatePanelVisibility(pageTitle, obj) ...
        Function to call the `updateVisibility()` GUI method of the
        page whose panel title is as indicated."""
        if hasattr(self, "parent"):
            parent = getattr(self, "parent")
            if parent and hasattr(parent, "featurePages"):
                for page in parent.featurePages:
                    if page.title == pageTitle:
                        page.updateVisibility()
                        break

    # Child classes to be overwritten
    def initPage(self, obj):
        """initPage(obj) ... overwrite to customize UI for specific model.
        Note that this function is invoked after all page controllers have been created.
        Should be overwritten by subclasses."""
        # pylint: disable=unused-argument
        pass  # pylint: disable=unnecessary-pass

    def cleanupPage(self, obj):
        """cleanupPage(obj) ... overwrite to perform any cleanup tasks before page is destroyed.
        Can safely be overwritten by subclasses."""
        pass  # pylint: disable=unnecessary-pass

    def modifyStandardButtons(self, buttonBox):
        """modifyStandardButtons(buttonBox) ... overwrite if the task panel standard buttons need to be modified.
        Can safely be overwritten by subclasses."""
        pass  # pylint: disable=unnecessary-pass

    def getForm(self):
        """getForm() ... return UI form for this page.
        Must be overwritten by subclasses."""
        pass  # pylint: disable=unnecessary-pass

    def getFields(self, obj):
        """getFields(obj) ... overwrite to transfer values from UI to obj's properties.
        Can safely be overwritten by subclasses."""
        pass  # pylint: disable=unnecessary-pass

    def setFields(self, obj):
        """setFields(obj) ... overwrite to transfer obj's property values to UI.
        Can safely be overwritten by subclasses."""
        pass  # pylint: disable=unnecessary-pass

    def getSignalsForUpdate(self, obj):
        """getSignalsForUpdate(obj) ... return signals which, when triggered, cause the receiver to update the model.
        See also registerSignalHandlers(obj)
        Can safely be overwritten by subclasses."""
        # pylint: disable=unused-argument
        return []

    def registerSignalHandlers(self, obj):
        """registerSignalHandlers(obj) ... overwrite to register custom signal handlers.
        In case an update of a model is not the desired operation of a signal invocation
        (see getSignalsForUpdate(obj)) this function can be used to register signal handlers
        manually.
        Can safely be overwritten by subclasses."""
        # pylint: disable=unused-argument
        pass  # pylint: disable=unnecessary-pass

    def updateData(self, obj, prop):
        """updateData(obj, prop) ... overwrite if the receiver needs to react to property changes that might not have been caused by the receiver itself.
        Sometimes a model will recalculate properties based on a change of another property. In order to keep the UI up to date with such changes this
        function can be used.
        Please note that the callback is synchronous with the property assignment operation. Also note that the notification is invoked regardless of the
        actual value of the property assignment. In other words it also fires if a property gets assigned the same value it already has.
        Taking above observations into account the implementation has to take care that it doesn't overwrite modified UI values by invoking setFields(obj).
        This can happen if a subclass unconditionally transfers all values in getFields(obj) to the model and just calls setFields(obj) in this callback.
        In such a scenario the first property assignment will cause all changes in the UI of the other fields to be overwritten by setFields(obj).
        You have been warned."""
        # pylint: disable=unused-argument
        pass  # pylint: disable=unnecessary-pass

    def updateSelection(self, obj, sel):
        """updateSelection(obj, sel) ...
        overwrite to customize UI depending on current selection.
        Can safely be overwritten by subclasses."""
        # pylint: disable=unused-argument
        pass  # pylint: disable=unnecessary-pass

    def updateVisibility(self, sentObj=None):
        """updateVisibility(sentObj=None) ...
        Called by sibling task panel pages via the `updatePanelVisibility()` method.
        Place panel updates within this method that should occur when triggered by the sibling caller.
        For example, a change in the Base Geometry panel should trigger updates in the Operation panel.
        Can safely be overwritten by subclasses."""
        pass  # pylint: disable=unnecessary-pass


# Eclass


class TaskPanelBaseGeometryPage(TaskPanelPage):
    """Page controller for the base geometry."""

    DataObject = QtCore.Qt.ItemDataRole.UserRole
    DataObjectSub = QtCore.Qt.ItemDataRole.UserRole + 1

    def __init__(self, obj, features):
        super(TaskPanelBaseGeometryPage, self).__init__(obj, features)

        self.title = "Base Geometry"
        self.OpIcon = ":/icons/Path_BaseGeometry.svg"
        self.setIcon(self.OpIcon)

    def getForm(self):
        panel = FreeCADGui.PySideUic.loadUi(":/panels/PageBaseGeometryEdit.ui")
        self.modifyPanel(panel)
        return panel

    def modifyPanel(self, panel):
        """modifyPanel(self, panel) ...
        Helper method to modify the current form immediately after
        it is loaded."""
        # Determine if Job operations are available with Base Geometry
        availableOps = list()
        ops = self.job.Operations.Group
        for op in ops:
            if hasattr(op, "Base") and isinstance(op.Base, list):
                if len(op.Base) > 0:
                    availableOps.append(op.Label)

        # Load available operations into combobox
        if len(availableOps) > 0:
            # Populate the operations list
            panel.geometryImportList.blockSignals(True)
            panel.geometryImportList.clear()
            availableOps.sort()
            for opLbl in availableOps:
                panel.geometryImportList.addItem(opLbl)
            panel.geometryImportList.blockSignals(False)
        else:
            panel.geometryImportList.hide()
            panel.geometryImportButton.hide()

    def getTitle(self, obj):
        return translate("PathOp2", "Base Geometry")

    def getFields(self, obj):
        pass

    def setFields(self, obj):
        self.form.baseList.blockSignals(True)
        self.form.baseList.clear()
        for base in self.obj.Base:
            for sub in base[1]:
                item = QtGui.QListWidgetItem("%s.%s" % (base[0].Label, sub))
                item.setData(self.DataObject, base[0])
                item.setData(self.DataObjectSub, sub)
                self.form.baseList.addItem(item)
        self.form.baseList.blockSignals(False)
        self.resizeBaseList()

    def itemActivated(self):
        FreeCADGui.Selection.clearSelection()
        for item in self.form.baseList.selectedItems():
            obj = item.data(self.DataObject)
            sub = item.data(self.DataObjectSub)
            if sub:
                FreeCADGui.Selection.addSelection(obj, sub)
            else:
                FreeCADGui.Selection.addSelection(obj)
        # FreeCADGui.updateGui()

    def supportsVertexes(self):
        return self.features & PathOp2.FeatureBaseVertexes

    def supportsEdges(self):
        return self.features & PathOp2.FeatureBaseEdges

    def supportsFaces(self):
        return self.features & PathOp2.FeatureBaseFaces

    def supportsPanels(self):
        return self.features & PathOp2.FeatureBasePanels

    def featureName(self):
        if self.supportsEdges() and self.supportsFaces():
            return "features"
        if self.supportsFaces():
            return "faces"
        if self.supportsEdges():
            return "edges"
        return "nothing"

    def selectionSupportedAsBaseGeometry(self, selection, ignoreErrors):
        if len(selection) != 1:
            if not ignoreErrors:
                msg = translate(
                    "PathProject",
                    "Please select %s from a single solid" % self.featureName(),
                )
                FreeCAD.Console.PrintError(msg + "\n")
                PathLog.debug(msg)
            return False
        sel = selection[0]
        if sel.HasSubObjects:
            if (
                not self.supportsVertexes()
                and selection[0].SubObjects[0].ShapeType == "Vertex"
            ):
                if not ignoreErrors:
                    PathLog.error(
                        translate("PathProject", "Vertexes are not supported")
                    )
                return False
            if (
                not self.supportsEdges()
                and selection[0].SubObjects[0].ShapeType == "Edge"
            ):
                if not ignoreErrors:
                    PathLog.error(translate("PathProject", "Edges are not supported"))
                return False
            if (
                not self.supportsFaces()
                and selection[0].SubObjects[0].ShapeType == "Face"
            ):
                if not ignoreErrors:
                    PathLog.error(translate("PathProject", "Faces are not supported"))
                return False
        else:
            if not self.supportsPanels() or "Panel" not in sel.Object.Name:
                if not ignoreErrors:
                    PathLog.error(
                        translate(
                            "PathProject",
                            "Please select %s of a solid" % self.featureName(),
                        )
                    )
                return False
        return True

    def addBaseGeometry(self, selection):
        PathLog.track(selection)
        if self.selectionSupportedAsBaseGeometry(selection, False):
            sel = selection[0]
            for sub in sel.SubElementNames:
                self.obj.Proxy.addBase(self.obj, sel.Object, sub)
            return True
        return False

    def addBase(self):
        PathLog.track()
        if self.addBaseGeometry(FreeCADGui.Selection.getSelectionEx()):
            # self.obj.Proxy.execute(self.obj)
            self.setFields(self.obj)
            self.setDirty()
            self.updatePanelVisibility("Operation", self.obj)

    def deleteBase(self):
        PathLog.track()
        selected = self.form.baseList.selectedItems()
        for item in selected:
            self.form.baseList.takeItem(self.form.baseList.row(item))
            self.setDirty()
        self.updateBase()
        self.updatePanelVisibility("Operation", self.obj)
        self.resizeBaseList()

    def updateBase(self):
        newlist = []
        for i in range(self.form.baseList.count()):
            item = self.form.baseList.item(i)
            obj = item.data(self.DataObject)
            sub = item.data(self.DataObjectSub)
            if sub:
                base = (obj, str(sub))
                newlist.append(base)
        PathLog.debug("Setting new base: %s -> %s" % (self.obj.Base, newlist))
        self.obj.Base = newlist

        # self.obj.Proxy.execute(self.obj)
        # FreeCAD.ActiveDocument.recompute()

    def clearBase(self):
        self.obj.Base = []
        self.setDirty()
        self.updatePanelVisibility("Operation", self.obj)
        self.resizeBaseList()

    def importBaseGeometry(self):
        opLabel = str(self.form.geometryImportList.currentText())
        ops = FreeCAD.ActiveDocument.getObjectsByLabel(opLabel)
        if len(ops) > 1:
            msg = translate("PathOpGui", "Mulitiple operations are labeled as")
            msg += " {}\n".format(opLabel)
            FreeCAD.Console.PrintWarning(msg)
        (base, subList) = ops[0].Base[0]
        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addSelection(base, subList)
        self.addBase()

    def registerSignalHandlers(self, obj):
        self.form.baseList.itemSelectionChanged.connect(self.itemActivated)
        self.form.addBase.clicked.connect(self.addBase)
        self.form.deleteBase.clicked.connect(self.deleteBase)
        self.form.clearBase.clicked.connect(self.clearBase)
        self.form.geometryImportButton.clicked.connect(self.importBaseGeometry)

    def pageUpdateData(self, obj, prop):
        if prop in ["Base"]:
            self.setFields(obj)

    def updateSelection(self, obj, sel):
        if self.selectionSupportedAsBaseGeometry(sel, True):
            self.form.addBase.setEnabled(True)
        else:
            self.form.addBase.setEnabled(False)

    def resizeBaseList(self):
        # Set base geometry list window to resize based on contents
        # Code reference:
        # https://stackoverflow.com/questions/6337589/qlistwidget-adjust-size-to-content
        # ml: disabling this logic because I can't get it to work on HPD monitor.
        #     On my systems the values returned by the list object are also incorrect on
        #     creation, leading to a list object of size 15. count() always returns 0 until
        #     the list is actually displayed. The same is true for sizeHintForRow(0), which
        #     returns -1 until the widget is rendered. The widget claims to have a size of
        #     (100, 30), once it becomes visible the size is (535, 192).
        #     Leaving the framework here in case somebody figures out how to set this up
        #     properly.
        qList = self.form.baseList
        row = (qList.count() + qList.frameWidth()) * 15
        # qList.setMinimumHeight(row)
        PathLog.debug(
            "baseList({}, {}) {} * {}".format(
                qList.size(), row, qList.count(), qList.sizeHintForRow(0)
            )
        )


# Eclass


class TaskPanelBaseLocationPage(TaskPanelPage):
    """Page controller for base locations. Uses PathGetPoint."""

    DataLocation = QtCore.Qt.ItemDataRole.UserRole

    def __init__(self, obj, features):
        super(TaskPanelBaseLocationPage, self).__init__(obj, features)

        # members initialized later
        self.editRow = None
        self.title = "Base Location"

    def getForm(self):
        self.formLoc = FreeCADGui.PySideUic.loadUi(":/panels/PageBaseLocationEdit.ui")
        if QtCore.qVersion()[0] == "4":
            self.formLoc.baseList.horizontalHeader().setResizeMode(
                QtGui.QHeaderView.Stretch
            )
        else:
            self.formLoc.baseList.horizontalHeader().setSectionResizeMode(
                QtGui.QHeaderView.Stretch
            )
        self.getPoint = PathGetPoint.TaskPanel(self.formLoc.addRemoveEdit)
        return self.formLoc

    def modifyStandardButtons(self, buttonBox):
        self.getPoint.buttonBox = buttonBox

    def getTitle(self, obj):
        return translate("PathOp2", "Base Location")

    def getFields(self, obj):
        pass

    def setFields(self, obj):
        self.formLoc.baseList.blockSignals(True)
        self.formLoc.baseList.clearContents()
        self.formLoc.baseList.setRowCount(0)
        for location in self.obj.Locations:
            self.formLoc.baseList.insertRow(self.formLoc.baseList.rowCount())

            item = QtGui.QTableWidgetItem("%.2f" % location.x)
            item.setData(self.DataLocation, location.x)
            self.formLoc.baseList.setItem(self.formLoc.baseList.rowCount() - 1, 0, item)

            item = QtGui.QTableWidgetItem("%.2f" % location.y)
            item.setData(self.DataLocation, location.y)
            self.formLoc.baseList.setItem(self.formLoc.baseList.rowCount() - 1, 1, item)
        self.formLoc.baseList.resizeColumnToContents(0)
        self.formLoc.baseList.blockSignals(False)
        self.itemActivated()

    def removeLocation(self):
        deletedRows = []
        selected = self.formLoc.baseList.selectedItems()
        for item in selected:
            row = self.formLoc.baseList.row(item)
            if row not in deletedRows:
                deletedRows.append(row)
                self.formLoc.baseList.removeRow(row)
        self.updateLocations()
        FreeCAD.ActiveDocument.recompute()

    def updateLocations(self):
        PathLog.track()
        locations = []
        for i in range(self.formLoc.baseList.rowCount()):
            x = self.formLoc.baseList.item(i, 0).data(self.DataLocation)
            y = self.formLoc.baseList.item(i, 1).data(self.DataLocation)
            location = FreeCAD.Vector(x, y, 0)
            locations.append(location)
        self.obj.Locations = locations

    def addLocation(self):
        self.getPoint.getPoint(self.addLocationAt)

    def addLocationAt(self, point, obj):
        # pylint: disable=unused-argument
        if point:
            locations = self.obj.Locations
            locations.append(point)
            self.obj.Locations = locations
            FreeCAD.ActiveDocument.recompute()

    def editLocation(self):
        selected = self.formLoc.baseList.selectedItems()
        if selected:
            row = self.formLoc.baseList.row(selected[0])
            self.editRow = row
            x = self.formLoc.baseList.item(row, 0).data(self.DataLocation)
            y = self.formLoc.baseList.item(row, 1).data(self.DataLocation)
            start = FreeCAD.Vector(x, y, 0)
            self.getPoint.getPoint(self.editLocationAt, start)

    def editLocationAt(self, point, obj):
        # pylint: disable=unused-argument
        if point:
            self.formLoc.baseList.item(self.editRow, 0).setData(
                self.DataLocation, point.x
            )
            self.formLoc.baseList.item(self.editRow, 1).setData(
                self.DataLocation, point.y
            )
            self.updateLocations()
            FreeCAD.ActiveDocument.recompute()

    def itemActivated(self):
        if self.formLoc.baseList.selectedItems():
            self.form.removeLocation.setEnabled(True)
            self.form.editLocation.setEnabled(True)
        else:
            self.form.removeLocation.setEnabled(False)
            self.form.editLocation.setEnabled(False)

    def registerSignalHandlers(self, obj):
        self.form.baseList.itemSelectionChanged.connect(self.itemActivated)
        self.formLoc.addLocation.clicked.connect(self.addLocation)
        self.formLoc.removeLocation.clicked.connect(self.removeLocation)
        self.formLoc.editLocation.clicked.connect(self.editLocation)

    def pageUpdateData(self, obj, prop):
        if prop in ["Locations"]:
            self.setFields(obj)


# Eclass


class TaskPanelHeightsDepthsPage(TaskPanelPage):
    """Page controller for depths."""

    def __init__(self, obj, features):
        super(TaskPanelHeightsDepthsPage, self).__init__(obj, features)

        # members initialized later
        self.startDepth = None
        self.finalDepth = None
        self.finishDepth = None
        self.stepDown = None
        self.title = "Heights and Depths"
        self.OpIcon = ":/icons/Path_Depths.svg"
        self.setIcon(self.OpIcon)
        self.refImagePath = "{}Mod/Path/Images/Ops/{}".format(
            FreeCAD.getHomePath(), "Path-DepthsAndHeights.gif"
        )  # pylint: disable=attribute-defined-outside-init
        self.refImage = QtGui.QPixmap(
            self.refImagePath
        )  # pylint: disable=attribute-defined-outside-init
        self.form.refImage.setPixmap(self.refImage)

        self.clearanceHeight = None
        self.safeHeight = None

    def getForm(self):
        return FreeCADGui.PySideUic.loadUi(":/panels/PageHeightsDepthsEdit.ui")

    def haveStartDepth(self):
        return PathOp2.FeatureHeightsDepths & self.features

    def haveFinalDepth(self):
        return (
            PathOp2.FeatureHeightsDepths & self.features
            and not PathOp2.FeatureNoFinalDepth & self.features
        )

    def haveFinishDepth(self):
        return (
            PathOp2.FeatureHeightsDepths & self.features
            and PathOp2.FeatureFinishDepth & self.features
        )

    def haveStepDown(self):
        return PathOp2.FeatureStepDown & self.features

    def initPage(self, obj):
        self.safeHeight = PathGui.QuantitySpinBox(
            self.form.safeHeight, obj, "SafeHeight"
        )
        self.clearanceHeight = PathGui.QuantitySpinBox(
            self.form.clearanceHeight, obj, "ClearanceHeight"
        )

        if self.haveStartDepth():
            self.startDepth = PathGui.QuantitySpinBox(
                self.form.startDepth, obj, "StartDepth"
            )
        else:
            self.form.startDepth.hide()
            self.form.startDepthLabel.hide()
            self.form.startDepthSet.hide()

        if self.haveFinalDepth():
            self.finalDepth = PathGui.QuantitySpinBox(
                self.form.finalDepth, obj, "FinalDepth"
            )
        else:
            if self.haveStartDepth():
                self.form.finalDepth.setEnabled(False)
                self.form.finalDepth.setToolTip(
                    translate(
                        "PathOp2",
                        "FinalDepth cannot be modified for this operation.\nIf it is necessary to set the FinalDepth manually please select a different operation.",
                    )
                )
            else:
                self.form.finalDepth.hide()
                self.form.finalDepthLabel.hide()
            self.form.finalDepthSet.hide()

        if self.haveStepDown():
            self.stepDown = PathGui.QuantitySpinBox(self.form.stepDown, obj, "StepDown")
        else:
            self.form.stepDown.hide()
            self.form.stepDownLabel.hide()

        if self.haveFinishDepth():
            self.finishDepth = PathGui.QuantitySpinBox(
                self.form.finishDepth, obj, "FinishDepth"
            )
        else:
            self.form.finishDepth.hide()
            self.form.finishDepthLabel.hide()

    def getTitle(self, obj):
        return translate("PathOp2", "Depths and Heights")

    def getFields(self, obj):
        self.safeHeight.updateProperty()
        self.clearanceHeight.updateProperty()
        if self.haveStartDepth():
            self.startDepth.updateProperty()
        if self.haveFinalDepth():
            self.finalDepth.updateProperty()
        if self.haveStepDown():
            self.stepDown.updateProperty()
        if self.haveFinishDepth():
            self.finishDepth.updateProperty()

    def setFields(self, obj):
        self.safeHeight.updateSpinBox()
        self.clearanceHeight.updateSpinBox()
        if self.haveStartDepth():
            self.startDepth.updateSpinBox()
        if self.haveFinalDepth():
            self.finalDepth.updateSpinBox()
        if self.haveStepDown():
            self.stepDown.updateSpinBox()
        if self.haveFinishDepth():
            self.finishDepth.updateSpinBox()
        self.updateSelection(obj, FreeCADGui.Selection.getSelectionEx())

    def getSignalsForUpdate(self, obj):
        signals = []
        signals.append(self.form.safeHeight.editingFinished)
        signals.append(self.form.clearanceHeight.editingFinished)
        if self.haveStartDepth():
            signals.append(self.form.startDepth.editingFinished)
        if self.haveFinalDepth():
            signals.append(self.form.finalDepth.editingFinished)
        if self.haveStepDown():
            signals.append(self.form.stepDown.editingFinished)
        if self.haveFinishDepth():
            signals.append(self.form.finishDepth.editingFinished)
        return signals

    def registerSignalHandlers(self, obj):
        if self.haveStartDepth():
            self.form.startDepthSet.clicked.connect(
                lambda: self.depthSet(obj, self.startDepth, "StartDepth")
            )
        if self.haveFinalDepth():
            self.form.finalDepthSet.clicked.connect(
                lambda: self.depthSet(obj, self.finalDepth, "FinalDepth")
            )

    def pageUpdateData(self, obj, prop):
        if prop in [
            "StartDepth",
            "FinalDepth",
            "StepDown",
            "FinishDepth",
            "SafeHeight",
            "ClearanceHeight",
        ]:
            self.setFields(obj)

    def depthSet(self, obj, spinbox, prop):
        z = self.selectionZLevel(FreeCADGui.Selection.getSelectionEx())
        if z is not None:
            PathLog.debug("depthSet(%s, %s, %.2f)" % (obj.Label, prop, z))
            if spinbox.expression():
                obj.setExpression(prop, None)
                self.setDirty()
            spinbox.updateSpinBox(FreeCAD.Units.Quantity(z, FreeCAD.Units.Length))
            if spinbox.updateProperty():
                self.setDirty()
        else:
            PathLog.info("depthSet(-)")

    def selectionZLevel(self, sel):
        if len(sel) == 1 and len(sel[0].SubObjects) == 1:
            sub = sel[0].SubObjects[0]
            if "Vertex" == sub.ShapeType:
                return sub.Z
            if PathGeom.isHorizontal(sub):
                if "Edge" == sub.ShapeType:
                    return sub.Vertexes[0].Z
                if "Face" == sub.ShapeType:
                    return sub.BoundBox.ZMax
        return None

    def updateSelection(self, obj, sel):
        if self.selectionZLevel(sel) is not None:
            self.form.startDepthSet.setEnabled(True)
            self.form.finalDepthSet.setEnabled(True)
        else:
            self.form.startDepthSet.setEnabled(False)
            self.form.finalDepthSet.setEnabled(False)


# Eclass


class TaskPanelDiametersPage(TaskPanelPage):
    """Page controller for diameters."""

    def __init__(self, obj, features):
        super(TaskPanelDiametersPage, self).__init__(obj, features)

        # members initialized later
        self.clearanceHeight = None
        self.safeHeight = None

    def getForm(self):
        return FreeCADGui.PySideUic.loadUi(":/panels/PageDiametersEdit.ui")

    def initPage(self, obj):
        self.minDiameter = PathGui.QuantitySpinBox(
            self.form.minDiameter, obj, "MinDiameter"
        )
        self.maxDiameter = PathGui.QuantitySpinBox(
            self.form.maxDiameter, obj, "MaxDiameter"
        )

    def getTitle(self, obj):
        return translate("Path", "Diameters")

    def getFields(self, obj):
        self.minDiameter.updateProperty()
        self.maxDiameter.updateProperty()

    def setFields(self, obj):
        self.minDiameter.updateSpinBox()
        self.maxDiameter.updateSpinBox()

    def getSignalsForUpdate(self, obj):
        signals = []
        signals.append(self.form.minDiameter.editingFinished)
        signals.append(self.form.maxDiameter.editingFinished)
        return signals

    def pageUpdateData(self, obj, prop):
        if prop in ["MinDiameter", "MaxDiameter"]:
            self.setFields(obj)


# Eclass


class TaskPanelExtensionPage(TaskPanelPage):
    DataObject = QtCore.Qt.ItemDataRole.UserRole
    DataSwitch = QtCore.Qt.ItemDataRole.UserRole + 2

    Direction = {
        FeatureExtensions.Extension.DirectionNormal: translate("PathPocket", "Normal"),
        FeatureExtensions.Extension.DirectionX: translate("PathPocket", "X"),
        FeatureExtensions.Extension.DirectionY: translate("PathPocket", "Y"),
    }

    def initPage(self, obj):
        self.obj = obj
        self.title = "Extensions"
        # self.setTitle("Extensions")
        self.OpIcon = ":/icons/view-axonometric.svg"
        self.setIcon(self.OpIcon)
        self.initialEdgeCount = -1
        self.edgeCountThreshold = 30
        self.fieldsSet = False
        self.operationPageForm = None
        self.usePerimeter = False
        self.extensionsCache = dict()
        self.extensionsReady = False
        self.enabled = True
        self.extensions = list()

        self.defaultLength = PathGui.QuantitySpinBox(
            self.form.defaultLength, obj, "ExtensionLengthDefault"
        )  # pylint: disable=attribute-defined-outside-init

        self.form.extensionTree.setEditTriggers(QtGui.QAbstractItemView.NoEditTriggers)
        self.form.extensionTree.setSelectionBehavior(QtGui.QAbstractItemView.SelectRows)

        self.switch = coin.SoSwitch()  # pylint: disable=attribute-defined-outside-init
        self.obj.ViewObject.RootNode.addChild(self.switch)
        self.switch.whichChild = coin.SO_SWITCH_ALL

        self.model = QtGui.QStandardItemModel(
            self.form.extensionTree
        )  # pylint: disable=attribute-defined-outside-init
        self.model.setHorizontalHeaderLabels(["Base", "Extension"])

        """
        # russ4262: This `if` block shows all available extensions upon edit of operation with any extension enabled.
        # This can cause the model(s) to overly obscured due to previews of extensions.
        # Would be great if only enabled extensions were shown.
        if 0 < len(obj.ExtensionFeature):
            self.form.showExtensions.setCheckState(QtCore.Qt.Checked)
        else:
            self.form.showExtensions.setCheckState(QtCore.Qt.Unchecked)
        """
        self.form.showExtensions.setCheckState(QtCore.Qt.Unchecked)

        self.blockUpdateData = False  # pylint: disable=attribute-defined-outside-init

    def cleanupPage(self, obj):
        try:
            self.obj.ViewObject.RootNode.removeChild(self.switch)
        except ReferenceError:
            PathLog.debug("obj already destroyed - no cleanup required")

    def getForm(self):
        form = FreeCADGui.PySideUic.loadUi(":/panels/PageExtensionsEdit.ui")
        # Hide warning label by default
        form.enableExtensionsWarning.hide()
        form.includeEdges.setChecked(False)
        form.includeSpecial.setChecked(False)
        return form

    def forAllItemsCall(self, cb):
        for modelRow in range(self.model.rowCount()):
            model = self.model.item(modelRow, 0)
            for featureRow in range(model.rowCount()):
                feature = model.child(featureRow, 0)
                for edgeRow in range(feature.rowCount()):
                    item = feature.child(edgeRow, 0)
                    ext = item.data(self.DataObject)
                    cb(item, ext)

    def currentExtensions(self):
        PathLog.debug("currentExtensions()")
        extensions = []

        def extractExtension(item, ext):
            if ext and ext.edge and item.checkState() == QtCore.Qt.Checked:
                extensions.append(ext.ext)

        if self.form.enableExtensions.isChecked():
            self.forAllItemsCall(extractExtension)
        PathLog.track("extensions", extensions)
        return extensions

    def updateProxyExtensions(self, obj):
        PathLog.debug("updateProxyExtensions()")
        self.extensions = (
            self.currentExtensions()
        )  # pylint: disable=attribute-defined-outside-init
        FeatureExtensions.setExtensions(obj, self.extensions)

    def getFields(self, obj):
        PathLog.track(obj.Label, self.model.rowCount(), self.model.columnCount())
        self.blockUpdateData = True  # pylint: disable=attribute-defined-outside-init

        if obj.ExtensionCorners != self.form.extendCorners.isChecked():
            obj.ExtensionCorners = self.form.extendCorners.isChecked()
        self.defaultLength.updateProperty()

        self.updateProxyExtensions(obj)
        self.blockUpdateData = False  # pylint: disable=attribute-defined-outside-init

    def setFields(self, obj):
        PathLog.track(obj.Label)
        PathLog.debug("setFields()")

        if obj.ExtensionCorners != self.form.extendCorners.isChecked():
            self.form.extendCorners.toggle()

        self._findOperationPageForm()
        self._getUsePerimeterState()  # Determine if only perimeter is being processed
        self._autoEnableExtensions()  # Check edge count for auto-disable Extensions on initial Task Panel loading
        self._initializeExtensions(obj)  # Efficiently initialize Extensions
        self.defaultLength.updateSpinBox()
        self.fieldsSet = True  # flag to identify initial values set

    def _initializeExtensions(self, obj):
        """_initializeExtensions()...
        Subroutine called inside `setFields()` to initialize Extensions efficiently."""
        PathLog.debug("_initializeExtensions(self.enabled={})".format(self.enabled))

        # Check for existing extensions first
        if (
            len(obj.ExtensionFeature) > 0
        ):  # latter test loads pre-existing extensions (editing of existing operation)
            hasEdges = False
            hasSpecial = False
            for (__, __, subFeat) in FeatureExtensions.readObjExtensionFeature(obj):
                if subFeat.startswith("Edge") or subFeat.startswith("Wire"):
                    hasEdges = True
                if subFeat.startswith("Extend") or subFeat.startswith("Waterline"):
                    hasSpecial = True

            self.extensions = FeatureExtensions.getExtensions(obj)
            self.form.enableExtensions.setChecked(True)

            if not hasEdges and not hasSpecial:
                self._enableExtensions()

            if hasEdges:
                self.form.includeEdges.setChecked(True)
                self._includeEdgesAndWires()
            if hasSpecial:
                self.form.includeSpecial.setChecked(True)
                self._includeSpecialExtensions()
        elif self.enabled:
            self.extensions = FeatureExtensions.getExtensions(obj)
            self.form.includeEdges.setChecked(True)
            # self.form.includeSpecial.setChecked(True)  # Leave special disabled until user enables them

        self.setExtensions(self.extensions)

    def updateQuantitySpinBoxes(self, index=None):
        prevValue = self.form.defaultLength.text()
        self.defaultLength.updateSpinBox()
        postValue = self.form.defaultLength.text()

        if postValue != prevValue:
            PathLog.debug("updateQuantitySpinBoxes() post != prev value")
            self._resetCachedExtensions()  # Reset extension cache because extension dimensions likely changed
            self._enableExtensions()  # Recalculate extensions

    def createItemForBaseModel(self, base, sub, edges, extensions):
        PathLog.track(
            base.Label, sub, "+", len(edges), len(base.Shape.getElement(sub).Edges)
        )
        # PathLog.debug("createItemForBaseModel() label: {}, sub: {}, {}, edgeCnt: {}, subEdges: {}".format(base.Label, sub, '+', len(edges), len(base.Shape.getElement(sub).Edges)))

        extendCorners = self.form.extendCorners.isChecked()
        includeEdges = self.form.includeEdges.isChecked()
        includeSpecial = self.form.includeSpecial.isChecked()
        subShape = base.Shape.getElement(sub)
        subHasItem = False

        def createSubItem(label, ext0):
            # PathLog.info("createSubItem({})".format(label))
            if ext0.root:
                self.switch.addChild(ext0.root)
                item0 = QtGui.QStandardItem()
                item0.setData(label, QtCore.Qt.EditRole)
                item0.setData(ext0, self.DataObject)
                item0.setCheckable(True)
                for e in extensions:
                    if e.obj == base and e.sub == label:
                        item0.setCheckState(QtCore.Qt.Checked)
                        ext0.enable()
                        break
                item.appendRow([item0])

        # ext = self._cachedExtension(self.obj, base, sub, None)
        ext = None
        item = QtGui.QStandardItem()
        item.setData(sub, QtCore.Qt.EditRole)
        item.setData(ext, self.DataObject)
        item.setSelectable(False)

        extensionEdges = {}
        if includeEdges:
            if self.usePerimeter and sub.startswith("Face"):
                # Only show exterior extensions if `Use Outline` is True
                subEdges = subShape.Wires[0].Edges
            else:
                # Show all exterior and interior extensions if `usePerimeter` is False, or sub is an edge
                subEdges = subShape.Edges

            for edge in subEdges:
                for (e, label) in edges:
                    if edge.isSame(e):
                        ext1 = self._cachedExtension(self.obj, base, sub, label)
                        if ext1.isValid():
                            extensionEdges[e] = label[4:]  # isolate edge number
                            if not extendCorners:
                                createSubItem(label, ext1)
                                subHasItem = True

        if extendCorners and includeEdges:

            def edgesMatchShape(e0, e1):
                flipped = PathGeom.flipEdge(e1)
                if flipped:
                    return PathGeom.edgesMatch(e0, e1) or PathGeom.edgesMatch(
                        e0, flipped
                    )
                else:
                    return PathGeom.edgesMatch(e0, e1)

            self.extensionEdges = extensionEdges
            PathLog.debug("extensionEdges.values(): {}".format(extensionEdges.values()))
            for edgeList in Part.sortEdges(
                list(extensionEdges.keys())
            ):  # Identify connected edges that form wires
                self.edgeList = edgeList
                if len(edgeList) == 1:
                    label = (
                        "Edge%s"
                        % [
                            extensionEdges[keyEdge]
                            for keyEdge in extensionEdges.keys()
                            if edgesMatchShape(keyEdge, edgeList[0])
                        ][0]
                    )
                else:
                    label = "Wire(%s)" % ",".join(
                        sorted(
                            [
                                extensionEdges[keyEdge]
                                for e in edgeList
                                for keyEdge in extensionEdges.keys()
                                if edgesMatchShape(e, keyEdge)
                            ],
                            key=lambda s: int(s),
                        )
                    )  # pylint: disable=unnecessary-lambda
                ext2 = self._cachedExtension(self.obj, base, sub, label)
                createSubItem(label, ext2)
                subHasItem = True

        # Only add these subItems for horizontally oriented faces, not edges or vertical faces (from vertical face loops)
        if sub.startswith("Face") and PathGeom.isHorizontal(subShape):
            if includeSpecial:
                # Add entry to extend outline of face
                label = "Extend_" + sub
                ext3 = self._cachedExtension(self.obj, base, sub, label)
                createSubItem(label, ext3)

                # Add entry for waterline at face
                label = "Waterline_" + sub
                ext4 = self._cachedExtension(self.obj, base, sub, label)
                createSubItem(label, ext4)

            # Add entry for avoid face
            label = "Avoid_" + sub
            ext5 = self._cachedExtension(self.obj, base, sub, label)
            createSubItem(label, ext5)
            subHasItem = True

        if subHasItem:
            # PathLog.info("subHasItem")
            return item

        return None

    def createItemForBaseModel_Edges(self, base, sub, edges, extensions):
        PathLog.track(
            base.Label, sub, "+", len(edges), len(base.Shape.getElement(sub).Edges)
        )
        # PathLog.debug("createItemForBaseModel_Edges() label: {}, sub: {}, {}, edgeCnt: {}, subEdges: {}".format(base.Label, sub, '+', len(edges), len(base.Shape.getElement(sub).Edges)))

        extendCorners = self.form.extendCorners.isChecked()
        includeEdges = self.form.includeEdges.isChecked()
        includeSpecial = self.form.includeSpecial.isChecked()
        subShape = base.Shape.getElement(sub)
        subHasItem = False

        def createSubItem(label, ext0):
            if ext0.root:
                self.switch.addChild(ext0.root)
                item0 = QtGui.QStandardItem()
                item0.setData(label, QtCore.Qt.EditRole)
                item0.setData(ext0, self.DataObject)
                item0.setCheckable(True)
                for e in extensions:
                    if e.obj == base and e.sub == label:
                        item0.setCheckState(QtCore.Qt.Checked)
                        ext0.enable()
                        break
                item.appendRow([item0])
                subHasItem = True

        # ext = self._cachedExtension(self.obj, base, sub, None)
        ext = None
        item = QtGui.QStandardItem()
        item.setData(sub, QtCore.Qt.EditRole)
        item.setData(ext, self.DataObject)
        item.setSelectable(False)

        extensionEdges = {}
        if includeEdges:
            if self.usePerimeter and sub.startswith("Face"):
                # Only show exterior extensions if `Use Outline` is True
                subEdges = subShape.Wires[0].Edges
            else:
                # Show all exterior and interior extensions if `usePerimeter` is False
                subEdges = subShape.Edges

            for edge in subEdges:
                for (e, label) in edges:
                    if edge.isSame(e):
                        ext1 = self._cachedExtension(self.obj, base, sub, label)
                        if ext1.isValid():
                            extensionEdges[e] = label[4:]  # isolate edge number
                            if not extendCorners:
                                createSubItem(label, ext1)

        if extendCorners and includeEdges:

            def edgesMatchShape(e0, e1):
                flipped = PathGeom.flipEdge(e1)
                if flipped:
                    return PathGeom.edgesMatch(e0, e1) or PathGeom.edgesMatch(
                        e0, flipped
                    )
                else:
                    return PathGeom.edgesMatch(e0, e1)

            self.extensionEdges = extensionEdges
            PathLog.debug("extensionEdges.values(): {}".format(extensionEdges.values()))
            for edgeList in Part.sortEdges(
                list(extensionEdges.keys())
            ):  # Identify connected edges that form wires
                self.edgeList = edgeList
                if len(edgeList) == 1:
                    label = (
                        "Edge%s"
                        % [
                            extensionEdges[keyEdge]
                            for keyEdge in extensionEdges.keys()
                            if edgesMatchShape(keyEdge, edgeList[0])
                        ][0]
                    )
                else:
                    label = "Wire(%s)" % ",".join(
                        sorted(
                            [
                                extensionEdges[keyEdge]
                                for e in edgeList
                                for keyEdge in extensionEdges.keys()
                                if edgesMatchShape(e, keyEdge)
                            ],
                            key=lambda s: int(s),
                        )
                    )  # pylint: disable=unnecessary-lambda
                ext2 = self._cachedExtension(self.obj, base, sub, label)
                createSubItem(label, ext2)

        if includeSpecial:
            pass

        if subHasItem:
            return item

        return None

    def setExtensions(self, extensions):
        PathLog.track(len(extensions))
        PathLog.debug("setExtensions()")

        if self.extensionsReady:
            PathLog.debug("setExtensions() returning per `extensionsReady` flag")
            return

        self.form.extensionTree.blockSignals(True)

        # remember current visual state
        if hasattr(self, "selectionModel"):
            selectedExtensions = [
                self.model.itemFromIndex(index).data(self.DataObject).ext
                for index in self.selectionModel.selectedIndexes()
            ]
        else:
            selectedExtensions = []
        collapsedModels = []
        collapsedFeatures = []
        for modelRow in range(self.model.rowCount()):
            model = self.model.item(modelRow, 0)
            modelName = model.data(QtCore.Qt.EditRole)
            if not self.form.extensionTree.isExpanded(model.index()):
                collapsedModels.append(modelName)
            for featureRow in range(model.rowCount()):
                feature = model.child(featureRow, 0)
                if not self.form.extensionTree.isExpanded(feature.index()):
                    collapsedFeatures.append(
                        "%s.%s" % (modelName, feature.data(QtCore.Qt.EditRole))
                    )

        # remove current extensions and all their visuals
        def removeItemSwitch(item, ext):
            # pylint: disable=unused-argument
            ext.hide()
            if ext.root:
                self.switch.removeChild(ext.root)

        self.forAllItemsCall(removeItemSwitch)
        self.model.clear()

        # create extensions for model and given argument
        if self.enabled:
            for base in self.obj.Base:
                show = True
                edges = [
                    (edge, "Edge%d" % (i + 1))
                    for i, edge in enumerate(base[0].Shape.Edges)
                ]
                baseItem = QtGui.QStandardItem()
                baseItem.setData(base[0].Label, QtCore.Qt.EditRole)
                baseItem.setSelectable(False)

                if self._allEdges(base[1]):
                    for sub in sorted(base[1]):
                        rowItem = self.createItemForBaseModel(
                            base[0], sub, edges, extensions
                        )
                        if rowItem:
                            baseItem.appendRow(rowItem)
                else:
                    for sub in sorted(base[1]):
                        rowItem = self.createItemForBaseModel(
                            base[0], sub, edges, extensions
                        )
                        if rowItem:
                            baseItem.appendRow(rowItem)
                if show:
                    self.model.appendRow(baseItem)

        self.form.extensionTree.setModel(self.model)
        self.form.extensionTree.expandAll()
        self.form.extensionTree.resizeColumnToContents(0)

        # restore previous state - at least the parts that are still valid
        for modelRow in range(self.model.rowCount()):
            model = self.model.item(modelRow, 0)
            modelName = model.data(QtCore.Qt.EditRole)
            if modelName in collapsedModels:
                self.form.extensionTree.setExpanded(model.index(), False)
            for featureRow in range(model.rowCount()):
                feature = model.child(featureRow, 0)
                featureName = "%s.%s" % (modelName, feature.data(QtCore.Qt.EditRole))
                if featureName in collapsedFeatures:
                    self.form.extensionTree.setExpanded(feature.index(), False)
        if hasattr(self, "selectionModel") and selectedExtensions:
            self.restoreSelection(selectedExtensions)

        self.form.extensionTree.blockSignals(False)
        self.extensionsReady = True
        PathLog.debug("  setExtensions() finished and setting `extensionsReady=True`")

    def _allEdges(self, featureLabels):
        for label in featureLabels:
            if not label.startswith("Edge"):
                return False
        return True

    def updateData(self, obj, prop):
        PathLog.track(obj.Label, prop, self.blockUpdateData)
        # PathLog.debug("updateData({})".format(prop))

        if not self.blockUpdateData:
            if self.fieldsSet:
                if self.form.enableExtensions.isChecked():
                    if prop == "ExtensionLengthDefault":
                        self.updateQuantitySpinBoxes()
                    elif prop == "Base":
                        self.extensionsReady = False
                        self.setExtensions(FeatureExtensions.getExtensions(obj))
                    elif prop in ["ProcessPerimiter", "ProcessHoles", "ProcessCircles"]:
                        self._getUsePerimeterState()  # Find `useOutline` checkbox and get its boolean value
                        self._includeEdgesAndWires()
                elif prop == "Base":
                    self.extensionsReady = False

    def restoreSelection(self, selection):
        PathLog.debug("restoreSelection()")
        PathLog.track()
        if 0 == self.model.rowCount():
            PathLog.track("-")
            self.form.buttonClear.setEnabled(False)
            self.form.buttonDisable.setEnabled(False)
            self.form.buttonEnable.setEnabled(False)
        else:
            self.form.buttonClear.setEnabled(True)

            if selection or self.selectionModel.selectedIndexes():
                self.form.buttonDisable.setEnabled(True)
                self.form.buttonEnable.setEnabled(True)
            else:
                self.form.buttonDisable.setEnabled(False)
                self.form.buttonEnable.setEnabled(False)

            FreeCADGui.Selection.clearSelection()

            def selectItem(item, ext):
                # pylint: disable=unused-argument
                for sel in selection:
                    if ext.base == sel.obj and ext.edge == sel.sub:
                        return True
                return False

            def setSelectionVisuals(item, ext):
                if selectItem(item, ext):
                    self.selectionModel.select(
                        item.index(), QtCore.QItemSelectionModel.Select
                    )

                selected = self.selectionModel.isSelected(item.index())
                if selected:
                    FreeCADGui.Selection.addSelection(ext.base, ext.face)
                    ext.select()
                else:
                    ext.deselect()

                if self.form.showExtensions.isChecked() or selected:
                    ext.show()
                else:
                    ext.hide()

            self.forAllItemsCall(setSelectionVisuals)

    def selectionChanged(self):
        PathLog.debug("selectionChanged()")
        self.restoreSelection([])

    def extensionsClear(self):
        PathLog.debug("extensionsClear()")

        def disableItem(item, ext):
            item.setCheckState(QtCore.Qt.Unchecked)
            ext.disable()

        self.forAllItemsCall(disableItem)
        self.setDirty()

    def _extensionsSetState(self, state):
        PathLog.debug("_extensionsSetState()")
        PathLog.track(state)
        for index in self.selectionModel.selectedIndexes():
            item = self.model.itemFromIndex(index)
            ext = item.data(self.DataObject)
            if ext.edge:
                item.setCheckState(state)
                ext.enable(state == QtCore.Qt.Checked)
        self.setDirty()

    def extensionsDisable(self):
        self._extensionsSetState(QtCore.Qt.Unchecked)

    def extensionsEnable(self):
        self._extensionsSetState(QtCore.Qt.Checked)

    def updateItemEnabled(self, item):
        PathLog.track(item)
        ext = item.data(self.DataObject)
        if item.checkState() == QtCore.Qt.Checked:
            ext.enable()
        else:
            ext.disable()
        self.updateProxyExtensions(self.obj)
        self.setDirty()

    def showHideExtension(self):
        if self.form.showExtensions.isChecked():

            def enableExtensionEdit(item, ext):
                # pylint: disable=unused-argument
                ext.show()

            self.forAllItemsCall(enableExtensionEdit)
        else:

            def disableExtensionEdit(item, ext):
                if not self.selectionModel.isSelected(item.index()):
                    ext.hide()

            self.forAllItemsCall(disableExtensionEdit)
        # self.setDirty()

    def toggleExtensionCorners(self):
        PathLog.debug("toggleExtensionCorners()")
        PathLog.track()
        self.extensionsReady = False
        extensions = FeatureExtensions.getExtensions(self.obj)
        self.setExtensions(extensions)
        self.selectionChanged()
        self.setDirty()

    def getSignalsForUpdate(self, obj):
        PathLog.track(obj.Label)
        signals = []
        signals.append(self.form.defaultLength.editingFinished)
        signals.append(self.form.enableExtensions.toggled)
        signals.append(self.form.includeEdges.toggled)
        return signals

    def registerSignalHandlers(self, obj):
        self.form.showExtensions.clicked.connect(self.showHideExtension)
        self.form.extendCorners.clicked.connect(self.toggleExtensionCorners)
        self.form.buttonClear.clicked.connect(self.extensionsClear)
        self.form.buttonDisable.clicked.connect(self.extensionsDisable)
        self.form.buttonEnable.clicked.connect(self.extensionsEnable)
        self.form.defaultLength.editingFinished.connect(self.updateQuantitySpinBoxes)
        self.form.enableExtensions.toggled.connect(self._enableExtensions)
        self.form.includeEdges.toggled.connect(self._includeEdgesAndWires)
        self.form.includeSpecial.toggled.connect(self._includeSpecialExtensions)

        self.model.itemChanged.connect(self.updateItemEnabled)

        self.selectionModel = (
            self.form.extensionTree.selectionModel()
        )  # pylint: disable=attribute-defined-outside-init
        self.selectionModel.selectionChanged.connect(self.selectionChanged)
        self.selectionChanged()

    # Support methods
    def _findOperationPageForm(self):
        """_findOperationPageForm() ...
        This method locates the `Operation` tab form page, and saves that page reference
        to self.operationPageForm.
        """
        PathLog.debug("_findOperationPageForm()")

        if self.operationPageForm:
            return

        for page in self.parent.featurePages:
            if page.title == "Operation":
                PathLog.debug("Found Operation page form ")
                self.operationPageForm = page.form
                return

    def _getUsePerimeterState(self):
        """_getUsePerimeterState() ...
        This method attempts to determine if only the perimeter of the selections are being processed,
        and sets a representative boolean value to self.usePerimeter.
        """
        PathLog.debug("_getUsePerimeterState()")

        opf = self.operationPageForm
        if not opf:
            return

        if (
            hasattr(opf, "processPerimeter")
            and hasattr(opf, "processHoles")
            and hasattr(opf, "processCircles")
        ):
            PathLog.debug("Found processPerimeter ")
            self.usePerimeter = (
                opf.processPerimeter.isChecked()
                and not opf.processHoles.isChecked()
                and not opf.processCircles.isChecked()
            )
            return

    # Methods for enable and disablement of Extensions feature
    def _autoEnableExtensions(self):
        """_autoEnableExtensions() ...
        This method is called to determine if the Extensions feature should be enabled,
        or auto disabled due to total edge count of selected faces.
        The auto enable/disable feature is designed to allow quicker access
        to operations that implement the Extensions feature when selected faces contain
        large numbers of edges, which require long computation times for preparation.

        The return value is a simple boolean to communicate whether or not Extensions
        are be enabled.
        """
        enabled = True

        if self.initialEdgeCount < 1 and enabled:
            self.initialEdgeCount = 0
            for base in self.obj.Base:
                for sub in sorted(base[1]):
                    self.initialEdgeCount += len(base[0].Shape.getElement(sub).Edges)
            if self.initialEdgeCount > self.edgeCountThreshold:
                # Block signals
                self.form.enableExtensions.blockSignals(True)
                self.form.enableExtensionsWarning.blockSignals(True)
                self.form.includeEdges.blockSignals(True)

                # Make changes to form
                msg = translate(
                    "PathPocketShape",
                    "Edge count greater than threshold of"
                    + " "
                    + str(self.edgeCountThreshold)
                    + ":  "
                    + str(self.initialEdgeCount),
                )
                self.form.enableExtensionsWarning.setText(msg)
                self.form.enableExtensions.setChecked(False)
                self.form.enableExtensionsWarning.show()
                msg = translate("PathFeatureExtensions", "Click to enable Extensions")
                self.form.enableExtensions.setText(msg)
                self.form.extensionEdit.setDisabled(True)
                self.form.includeEdges.setChecked(False)
                msg = translate("PathFeatureExtensions", "Click to include Edges/Wires")
                self.form.includeEdges.setText(msg)

                # Unblock signals
                self.form.enableExtensions.blockSignals(False)
                self.form.enableExtensionsWarning.blockSignals(False)
                self.form.includeEdges.blockSignals(False)

                enabled = False
        elif not self.form.enableExtensions.isChecked():
            enabled = False

        PathLog.debug("_autoEnableExtensions() is {}".format(enabled))
        self.enabled = enabled

    def _enableExtensions(self):
        """_enableExtensions() ...
        This method is called when the enableExtensions push button is toggled.
        This method manages the enabled or disabled state of the extensionsEdit
        Task Panel input group.
        """
        PathLog.debug("_enableExtensions()")

        if self.form.enableExtensions.isChecked():
            self.enabled = True
            self.extensionsReady = False
            msg = translate("PathFeatureExtensions", "Extensions enabled")
            self.form.enableExtensions.setText(msg)
            self.form.enableExtensionsWarning.hide()
            self.form.extensionEdit.setEnabled(True)
            self.extensions = FeatureExtensions.getExtensions(self.obj)
            self.setExtensions(self.extensions)
        else:
            msg = translate("PathFeatureExtensions", "Click to enable Extensions")
            self.form.enableExtensions.setText(msg)
            self.form.extensionEdit.setDisabled(True)
            self.enabled = False

    def _includeEdgesAndWires(self):
        """_includeEdgesAndWires() ...
        This method is called when the includeEdges checkbox is toggled.
        This method manages the message thereof.
        """
        PathLog.debug("_includeEdgesAndWires()")

        self._getUsePerimeterState()  # Find `useOutline` checkbox and get its boolean value
        if self.form.includeEdges.isChecked():
            msg = translate("PathFeatureExtensions", "Including Edges/Wires")
            self.form.includeEdges.setText(msg)
        else:
            msg = translate("PathFeatureExtensions", "Include Edges/Wires")
            self.form.includeEdges.setText(msg)
        self.extensionsReady = False
        self._enableExtensions()

    def _includeSpecialExtensions(self):
        """_includeSpecialExtensions() ...
        This method is called when the includeSpecial checkbox is toggled.
        This method manages the message thereof.
        """
        PathLog.debug("_includeSpecialExtensions()")

        if self.form.includeSpecial.isChecked():
            msg = translate("PathFeatureExtensions", "Including special options")
            self.form.includeSpecial.setText(msg)
        else:
            msg = translate("PathFeatureExtensions", "Include special options")
            self.form.includeSpecial.setText(msg)
        self.extensionsReady = False
        self._enableExtensions()

    # Methods for creating and managing cached extensions
    def _cachedExtension(self, obj, base, sub, label):
        """_cachedExtension(obj, base, sub, label)...
        This method creates a new _Extension object if none is found within
        the extensionCache dictionary."""

        if label:
            cacheLabel = base.Name + "_" + sub + "_" + label
        else:
            cacheLabel = base.Name + "_" + sub + "_None"

        if cacheLabel in self.extensionsCache.keys():
            # PathLog.debug("return _cachedExtension({})".format(cacheLabel))
            return self.extensionsCache[cacheLabel]
        else:
            # PathLog.debug("_cachedExtension({}) created".format(cacheLabel))
            _ext = _Extension(obj, base, sub, label)
            self.extensionsCache[cacheLabel] = _ext  # cache the extension
            return _ext

    def _resetCachedExtensions(self):
        PathLog.debug("_resetCachedExtensions()")
        reset = dict()
        # Keep waterline extensions as they will not change
        for k in self.extensionsCache.keys():
            if k.startswith("Waterline"):
                reset[k] = self.extensionsCache[k]
        self.extensionsCache = reset
        self.extensionsReady = False


# Eclass


# Helper class for TaskPanelExtensionPage() class to provide extension visualizations in viewport
class _Extension(object):
    ColourEnabled = (1.0, 0.5, 1.0)
    ColourDisabled = (1.0, 1.0, 0.5)
    TransparencySelected = 0.0
    TransparencyDeselected = 0.7

    def __init__(self, obj, base, face, edge):
        self.obj = obj
        self.base = base
        self.face = face
        self.edge = edge
        self.ext = None

        if edge:
            self.ext = FeatureExtensions.createExtension(obj, base, face, edge)

        self.switch = self.createExtensionSoSwitch(self.ext)
        self.root = self.switch

    def createExtensionSoSwitch(self, ext):
        if not ext:
            return None

        sep = coin.SoSeparator()
        pos = coin.SoTranslation()
        mat = coin.SoMaterial()
        crd = coin.SoCoordinate3()
        fce = coin.SoFaceSet()
        hnt = coin.SoShapeHints()
        numVert = list()  # track number of vertices in each polygon face

        try:
            wire = ext.getWire()
        except FreeCAD.Base.FreeCADError:
            wire = None

        if not wire:
            return None

        if isinstance(wire, (list, tuple)):
            p0 = [p for p in wire[0].discretize(Deflection=0.02)]
            p1 = [p for p in wire[1].discretize(Deflection=0.02)]
            p2 = list(reversed(p1))
            polygon = [(p.x, p.y, p.z) for p in (p0 + p2)]
        else:
            if ext.extFaces:
                # Create polygon for each extension face in compound extensions
                allPolys = list()
                extFaces = ext.getExtensionFaces(wire)
                for f in extFaces:
                    pCnt = 0
                    wCnt = 0
                    for w in f.Wires:
                        if wCnt == 0:
                            poly = [p for p in w.discretize(Deflection=0.01)]
                        else:
                            poly = [p for p in w.discretize(Deflection=0.01)][:-1]
                        pCnt += len(poly)
                        allPolys.extend(poly)
                    numVert.append(pCnt)
                polygon = [(p.x, p.y, p.z) for p in allPolys]
            else:
                # poly = [p for p in wire.discretize(Deflection=0.02)][:-1]
                poly = [p for p in wire.discretize(Deflection=0.02)]
                polygon = [(p.x, p.y, p.z) for p in poly]
        crd.point.setValues(polygon)

        mat.diffuseColor = self.ColourDisabled
        mat.transparency = self.TransparencyDeselected

        hnt.faceType = coin.SoShapeHints.UNKNOWN_FACE_TYPE
        hnt.vertexOrdering = coin.SoShapeHints.CLOCKWISE

        if numVert:
            # Transfer vertex counts for polygon faces
            fce.numVertices.setValues(tuple(numVert))

        sep.addChild(pos)
        sep.addChild(mat)
        sep.addChild(hnt)
        sep.addChild(crd)
        sep.addChild(fce)

        # Finalize SoSwitch
        switch = coin.SoSwitch()
        switch.addChild(sep)
        switch.whichChild = coin.SO_SWITCH_NONE

        self.material = mat

        return switch

    def _setColour(self, r, g, b):
        self.material.diffuseColor = (r, g, b)

    def isValid(self):
        return not self.root is None

    def show(self):
        if self.switch:
            self.switch.whichChild = coin.SO_SWITCH_ALL

    def hide(self):
        if self.switch:
            self.switch.whichChild = coin.SO_SWITCH_NONE

    def enable(self, ena=True):
        if ena:
            self.material.diffuseColor = self.ColourEnabled
        else:
            self.disable()

    def disable(self):
        self.material.diffuseColor = self.ColourDisabled

    def select(self):
        self.material.transparency = self.TransparencySelected

    def deselect(self):
        self.material.transparency = self.TransparencyDeselected


# Eclass

FreeCAD.Console.PrintLog("Loading PathTaskPanelPagesGui... done\n")
