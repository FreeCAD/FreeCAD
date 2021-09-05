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
import PathScripts.PathLog as PathLog
import PathScripts.operations.PathOp2 as PathOp2
import PathScripts.PathPreferences as PathPreferences
import PathScripts.PathSelection as PathSelection
import PathScripts.PathUtil as PathUtil
from pivy import coin

from PySide import QtCore, QtGui

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

BaseGeometryPage = LazyLoader(
    "PathScripts.taskpanels.PathTaskPanelBaseGeometryPage",
    globals(),
    "PathScripts.taskpanels.PathTaskPanelBaseGeometryPage",
)
BaseLocationPage = LazyLoader(
    "PathScripts.taskpanels.PathTaskPanelBaseLocationPage",
    globals(),
    "PathScripts.taskpanels.PathTaskPanelBaseLocationPage",
)
HeightsDepthsPage = LazyLoader(
    "PathScripts.taskpanels.PathTaskPanelHeightsDepthsPage",
    globals(),
    "PathScripts.taskpanels.PathTaskPanelHeightsDepthsPage",
)
DiametersPage = LazyLoader(
    "PathScripts.taskpanels.PathTaskPanelDiametersPage",
    globals(),
    "PathScripts.taskpanels.PathTaskPanelDiametersPage",
)
ExtensionPage = LazyLoader(
    "PathScripts.taskpanels.PathTaskPanelExtensionPage",
    globals(),
    "PathScripts.taskpanels.PathTaskPanelExtensionPage",
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
                page = BaseGeometryPage.TaskPanelBaseGeometryPage(obj, features)
            self.featurePages["BaseGeometry"] = page

        if PathOp2.FeatureLocations & features:
            if hasattr(opPage, "taskPanelBaseLocationPage"):
                page = opPage.taskPanelBaseLocationPage(obj, features)
            else:
                page = BaseLocationPage.TaskPanelBaseLocationPage(obj, features)
            self.featurePages["Locations"] = page

        if PathOp2.FeatureHeightsDepths & features:
            if hasattr(opPage, "taskPanelHeightsDepthsPage"):
                page = opPage.taskPanelHeightsDepthsPage(obj, features)
            else:
                page = HeightsDepthsPage.TaskPanelHeightsDepthsPage(obj, features)
            self.featurePages["Locations"] = page

        if PathOp2.FeatureDiameters & features:
            if hasattr(opPage, "taskPanelDiametersPage"):
                page = opPage.taskPanelDiametersPage(obj, features)
            else:
                page = DiametersPage.TaskPanelDiametersPage(obj, features)
            self.featurePages["Diameters"] = page

        if PathOp2.FeatureExtensions & features:
            if hasattr(opPage, "taskPanelExtensionPage"):
                page = opPage.taskPanelExtensionPage(obj, features)
            else:
                page = ExtensionPage.TaskPanelExtensionPage(obj, features)
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


FreeCAD.Console.PrintLog("Loading PathTaskPanel... done\n")
