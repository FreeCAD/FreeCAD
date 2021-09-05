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

import FreeCADGui
import PathScripts.PathJob as PathJob
import PathScripts.PathLog as PathLog
import PathScripts.PathUtils as PathUtils

from PySide import QtCore


__title__ = "Path UI Task Panel Pages base classes"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecadweb.org"
__doc__ = "Base classes for UI features within Path operations"


FreeCAD = PathJob.FreeCAD
translate = PathJob.translate

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule(PathLog.thisModule())


# Use the import to appease LGTM
# FreeCADGui is used by other modules that depend upon this one, PathTaskPanelPage
if FreeCADGui:
    True


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


FreeCAD.Console.PrintLog("Loading PathTaskPanelPage... done\n")
