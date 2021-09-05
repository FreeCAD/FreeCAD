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
import PathScripts.PathSelection as PathSelection
import PathScripts.PathSetupSheet as PathSetupSheet
import PathScripts.PathUtil as PathUtil
import PathScripts.taskpanels.PathTaskPanel as PathTaskPanel

import importlib

from PySide import QtCore, QtGui

__title__ = "Path Operation UI view provider base class"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecadweb.org"
__doc__ = "Base class of view provider for Path operations' UI"

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule(PathLog.thisModule())


def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ViewProvider(object):
    """Generic view provider for path objects.
    Deducts the icon name from operation name, brings up the TaskPanel
    with pages corresponding to the operation's opFeatures() and forwards
    property change notifications to the page controllers.
    """

    def __init__(self, vobj, resources):
        PathLog.track()
        self.deleteOnReject = True
        self.OpIcon = ":/icons/%s.svg" % resources.pixmap
        self.OpName = resources.name
        self.OpPageModule = resources.opPageClass.__module__
        self.OpPageClass = resources.opPageClass.__name__

        # initialized later
        self.vobj = vobj
        self.Object = None
        self.panel = None

    def attach(self, vobj):
        PathLog.track()
        self.vobj = vobj
        self.Object = vobj.Object
        self.panel = None
        return

    def deleteObjectsOnReject(self):
        """deleteObjectsOnReject() ... return true if all objects should
        be created if the user hits cancel. This is used during the initial
        edit session, if the user does not press OK, it is assumed they've
        changed their mind about creating the operation."""
        PathLog.track()
        return hasattr(self, "deleteOnReject") and self.deleteOnReject

    def setDeleteObjectsOnReject(self, state=False):
        PathLog.track()
        self.deleteOnReject = state
        return self.deleteOnReject

    def setEdit(self, vobj=None, mode=0):
        """setEdit(vobj, mode=0) ... initiate editing of receivers model."""
        PathLog.track()
        if 0 == mode:
            if vobj is None:
                vobj = self.vobj
            page = self.getTaskPanelOpPage(vobj.Object)
            page.setTitle(self.OpName)
            page.setIcon(self.OpIcon)
            selection = self.getSelectionFactory()
            self.setupTaskPanel(
                PathTaskPanel.TaskPanel(
                    vobj.Object,
                    self.deleteObjectsOnReject(),
                    page,
                    selection,
                    parent=self,
                )
            )
            self.deleteOnReject = False
            return True
        elif 5 == mode:
            if vobj is None:
                vobj = self.vobj
            self.deleteOnReject = False
            return True
        # no other editing possible
        return False

    def setupTaskPanel(self, panel):
        """setupTaskPanel(panel) ... internal function to start the editor."""
        self.panel = panel
        FreeCADGui.Control.closeDialog()
        FreeCADGui.Control.showDialog(panel)
        panel.setupUi()
        job = self.Object.Proxy.getJob(self.Object)
        if job:
            job.ViewObject.Proxy.setupEditVisibility(job)
        else:
            PathLog.info("did not find no job")

    def clearTaskPanel(self):
        """clearTaskPanel() ... internal callback function when editing has finished."""
        self.panel = None
        job = self.Object.Proxy.getJob(self.Object)
        if job:
            job.ViewObject.Proxy.resetEditVisibility(job)

    def unsetEdit(self, arg1, arg2):
        # pylint: disable=unused-argument
        if self.panel:
            self.panel.reject(False)

    def __getstate__(self):
        """__getstate__() ... callback before receiver is saved to a file.
        Returns a dictionary with the receiver's resources as strings."""
        PathLog.track()
        state = {}
        state["OpName"] = self.OpName
        state["OpIcon"] = self.OpIcon
        state["OpPageModule"] = self.OpPageModule
        state["OpPageClass"] = self.OpPageClass
        return state

    def __setstate__(self, state):
        """__setstate__(state) ... callback on restoring a saved instance, pendant to __getstate__()
        state is the dictionary returned by __getstate__()."""
        self.OpName = state["OpName"]
        self.OpIcon = state["OpIcon"]
        self.OpPageModule = state["OpPageModule"]
        self.OpPageClass = state["OpPageClass"]

    def getIcon(self):
        """getIcon() ... the icon used in the object tree"""
        if self.Object.Active:
            return self.OpIcon
        else:
            return ":/icons/Path_OpActive.svg"

    def getTaskPanelOpPage(self, obj):
        """getTaskPanelOpPage(obj) ... use the stored information to instantiate the receiver op's page controller."""
        mod = importlib.import_module(self.OpPageModule)
        cls = getattr(mod, self.OpPageClass)
        return cls(obj, 0)

    def getSelectionFactory(self):
        """getSelectionFactory() ... return a factory function that can be used to create the selection observer."""
        return PathSelection.select(self.OpName)

    def updateData(self, obj, prop):
        """updateData(obj, prop) ... callback whenever a property of the receiver's model is assigned.
        The callback is forwarded to the task panel - in case an editing session is ongoing."""
        # PathLog.track(obj.Label, prop) # Creates a lot of noise
        if self.panel:
            self.panel.updateData(obj, prop)

    def onDelete(self, vobj, arg2=None):
        # pylint: disable=unused-argument
        PathUtil.clearExpressionEngine(vobj.Object)
        return True

    def setupContextMenu(self, vobj, menu):
        # pylint: disable=unused-argument
        PathLog.track()
        for action in menu.actions():
            menu.removeAction(action)
        action = QtGui.QAction(translate("Path", "Edit"), menu)
        action.triggered.connect(self.setEdit)
        menu.addAction(action)


class CommandSetStartPoint:
    """Command to set the start point for an operation."""

    # pylint: disable=no-init

    def GetResources(self):
        return {
            "Pixmap": "Path_StartPoint",
            "MenuText": QtCore.QT_TRANSLATE_NOOP("Path", "Pick Start Point"),
            "ToolTip": QtCore.QT_TRANSLATE_NOOP("Path", "Pick Start Point"),
        }

    def IsActive(self):
        if FreeCAD.ActiveDocument is None:
            return False
        sel = FreeCADGui.Selection.getSelection()
        if not sel:
            return False
        obj = sel[0]
        return obj and hasattr(obj, "StartPoint")

    def setpoint(self, point, o):
        # pylint: disable=unused-argument
        obj = FreeCADGui.Selection.getSelection()[0]
        obj.StartPoint.x = point.x
        obj.StartPoint.y = point.y
        obj.StartPoint.z = obj.ClearanceHeight.Value

    def Activated(self):
        if not hasattr(FreeCADGui, "Snapper"):
            import DraftTools
        FreeCADGui.Snapper.getPoint(callback=self.setpoint)


def Create(res):
    """Create(res) ... generic implementation of a create function.
    res is an instance of CommandResources. It is not expected that the user invokes
    this function directly, but calls the Activated() function of the Command object
    that is created in each operations Gui implementation."""
    FreeCAD.ActiveDocument.openTransaction("Create %s" % res.name)
    obj = res.objFactory(res.name, obj=None, parentJob=res.job)
    if obj.Proxy:
        obj.ViewObject.Proxy = ViewProvider(obj.ViewObject, res)
        obj.ViewObject.Visibility = False
        FreeCAD.ActiveDocument.commitTransaction()

        obj.ViewObject.Document.setEdit(obj.ViewObject, 0)
        return obj
    FreeCAD.ActiveDocument.abortTransaction()
    return None


class CommandPathOp:
    """Generic, data driven implementation of a Path operation creation command.
    Instances of this class are stored in all Path operation Gui modules and can
    be used to create said operations with view providers and all."""

    def __init__(self, resources):
        self.res = resources

    def GetResources(self):
        ress = {
            "Pixmap": self.res.pixmap,
            "MenuText": self.res.menuText,
            "ToolTip": self.res.toolTip,
        }
        if self.res.accelKey:
            ress["Accel"] = self.res.accelKey
        return ress

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                    return True
        return False

    def Activated(self):
        return Create(self.res)


class CommandResources:
    """POD class to hold command specific resources."""

    def __init__(
        self, name, objFactory, opPageClass, pixmap, menuText, accelKey, toolTip
    ):
        self.name = name
        self.objFactory = objFactory
        self.opPageClass = opPageClass
        self.pixmap = pixmap
        self.menuText = menuText
        self.accelKey = accelKey
        self.toolTip = toolTip
        self.job = None


def SetupOperation(
    name, objFactory, opPageClass, pixmap, menuText, toolTip, setupProperties=None
):
    """SetupOperation(name, objFactory, opPageClass, pixmap, menuText, toolTip, setupProperties=None)
    Creates an instance of CommandPathOp with the given parameters and registers the command with FreeCAD.
    When activated it creates a model with proxy (by invoking objFactory), assigns a view provider to it
    (see ViewProvider in this module) and starts the editor specifically for this operation (driven by opPageClass).
    This is an internal function that is automatically called by the initialisation code for each operation.
    It is not expected to be called manually.
    """

    res = CommandResources(
        name, objFactory, opPageClass, pixmap, menuText, None, toolTip
    )

    command = CommandPathOp(res)
    FreeCADGui.addCommand("Path_%s" % name.replace(" ", "_"), command)

    if setupProperties is not None:
        PathSetupSheet.RegisterOperation(name, objFactory, setupProperties)

    return command


FreeCADGui.addCommand("Path_SetStartPoint", CommandSetStartPoint())

FreeCAD.Console.PrintLog("Loading PathOpGui... done\n")
