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

__title__ = "Path Operation UI base classes"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Base classes and framework for Path operation's UI"

# Standard
import importlib
# Third-party
# FreeCAD
import PathScripts.PathPreferences as PathPreferences
import PathScripts.PathSelection as PathSelection
import PathScripts.PathSetupSheet as PathSetupSheet
import PathScripts.path_features.gui_features as PathOpGuiFeatures


# Pointers to existing module imports
FreeCAD     = PathOpGuiFeatures.FreeCAD
FreeCADGui  = PathOpGuiFeatures.FreeCADGui
OpFeatures  = PathOpGuiFeatures.OpFeatures
PathGui     = PathOpGuiFeatures.PathGui
QtCore      = PathGui.PySide.QtCore
QtGui       = PathGui.PySide.QtGui
PathUtil    = OpFeatures.PathUtil
PathUtils   = OpFeatures.PathUtils
PathLog     = OpFeatures.PathLog

# Pointers to relocated classes
TaskPanelPage = PathOpGuiFeatures.TaskPanelPage
TaskPanelBaseGeometryPage = PathOpGuiFeatures.TaskPanelBaseGeometryPage
TaskPanelBaseLocationPage = PathOpGuiFeatures.TaskPanelBaseLocationPage
TaskPanelHeightsPage = PathOpGuiFeatures.TaskPanelHeightsPage
TaskPanelDepthsPage = PathOpGuiFeatures.TaskPanelDepthsPage


# Set logging level for this module
PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule(PathLog.thisModule())


def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ViewProvider(object):
    '''Generic view provider for path objects.
    Deducts the icon name from operation name, brings up the TaskPanel
    with pages corresponding to the operation's opFeatures() and forwards
    property change notifications to the page controllers.
    '''

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
        '''deleteObjectsOnReject() ... return true if all objects should
        be created if the user hits cancel. This is used during the initial
        edit session, if the user does not press OK, it is assumed they've
        changed their mind about creating the operation.'''
        PathLog.track()
        return hasattr(self, 'deleteOnReject') and self.deleteOnReject

    def setDeleteObjectsOnReject(self, state=False):
        PathLog.track()
        self.deleteOnReject = state
        return self.deleteOnReject

    def setEdit(self, vobj=None, mode=0):
        '''setEdit(vobj, mode=0) ... initiate editing of receivers model.'''
        PathLog.track()
        if 0 == mode:
            if vobj is None:
                vobj = self.vobj
            page = self.getTaskPanelOpPage(vobj.Object)
            page.setTitle(self.OpName)
            page.setIcon(self.OpIcon)
            page.set_op_class_attribute(self.OpName)  # set reference to operation's class
            selection = self.getSelectionFactory()
            panel = TaskPanel(vobj.Object,
                              self.deleteObjectsOnReject(), page, selection)
            panel.setParent(self)
            self.setupTaskPanel(panel)
            self.deleteOnReject = False
            return True
        # no other editing possible
        return False

    def setupTaskPanel(self, panel):
        '''setupTaskPanel(panel) ... internal function to start the editor.'''
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
        '''clearTaskPanel() ...
        Internal callback function when editing has finished.
        '''
        self.panel = None
        job = self.Object.Proxy.getJob(self.Object)
        if job:
            job.ViewObject.Proxy.resetEditVisibility(job)

    def unsetEdit(self, arg1, arg2):
        # pylint: disable=unused-argument
        if self.panel:
            self.panel.reject(False)

    def __getstate__(self):
        '''__getstate__() ... callback before receiver is saved to a file.
        Returns a dictionary with the receiver's resources as strings.
        '''
        PathLog.track()
        state = {}
        state['OpName'] = self.OpName
        state['OpIcon'] = self.OpIcon
        state['OpPageModule'] = self.OpPageModule
        state['OpPageClass'] = self.OpPageClass
        return state

    def __setstate__(self, state):
        '''__setstate__(state) ...
        Callback on restoring a saved instance, pendant to __getstate__()
        state is the dictionary returned by __getstate__().
        '''
        self.OpName = state['OpName']
        self.OpIcon = state['OpIcon']
        self.OpPageModule = state['OpPageModule']
        self.OpPageClass = state['OpPageClass']

    def getIcon(self):
        '''getIcon() ... the icon used in the object tree'''
        if self.Object.Active:
            return self.OpIcon
        else:
            return ":/icons/Path-OpActive.svg"

    def getTaskPanelOpPage(self, obj):
        '''getTaskPanelOpPage(obj) ...
        Use the stored information to instantiate the receiver op's
        page controller.
        '''
        mod = importlib.import_module(self.OpPageModule)
        cls = getattr(mod, self.OpPageClass)
        return cls(obj, 0)

    def getSelectionFactory(self):
        '''getSelectionFactory() ...
        Return a factory function that can be used to create
        the selection observer.
        '''
        return PathSelection.select(self.OpName)

    def updateData(self, obj, prop):
        '''updateData(obj, prop) ...
        Callback whenever a property of the receiver's model is assigned.
        The callback is forwarded to the task panel - in case
        an editing session is ongoing.
        '''
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
        action = QtGui.QAction(translate('Path', 'Edit'), menu)
        action.triggered.connect(self.setEdit)
        menu.addAction(action)
# Eclass


class TaskPanel(object):
    '''
    Generic TaskPanel implementation handling standard Path operations layout.

    This class only implements the framework and takes care of bringing
    all pages up and down in a controller fashion.
    It implements the standard editor behaviour for OK, Cancel and Apply
    and tracks if the model is still in sync withthe UI.
    However, all display and processing of fields is handled by
    the page controllers which are managed in a list. All event callbacks and
    framework actions are forwarded to the page controllers in turn and
    each page controller is expected to process all events concerning
    the data it manages.
    '''

    def __init__(self, obj, deleteOnReject, opPage, selectionFactory):
        PathLog.track(obj.Label, deleteOnReject, opPage, selectionFactory)
        msg = translate("Path", "General Operation")
        FreeCAD.ActiveDocument.openTransaction(msg)
        self.deleteOnReject = deleteOnReject
        self.featurePages = []
        self.parent = None

        # members initialized later
        self.clearanceHeight = None
        self.safeHeight = None
        self.startDepth = None
        self.finishDepth = None
        self.finalDepth = None
        self.stepDown = None
        self.buttonBox = None

        features = obj.Proxy.opFeatures(obj)
        opPage.features = features

        if OpFeatures.FeatureBaseGeometry & features:
            if hasattr(opPage, 'taskPanelBaseGeometryPage'):
                page = opPage.taskPanelBaseGeometryPage(obj, features)
            else:
                page = TaskPanelBaseGeometryPage(obj, features)
            self.featurePages.append(page)

        if OpFeatures.FeatureLocations & features:
            if hasattr(opPage, 'taskPanelBaseLocationPage'):
                page = opPage.taskPanelBaseLocationPage(obj, features)
            else:
                page = TaskPanelBaseLocationPage(obj, features)
            self.featurePages.append(page)

        if (OpFeatures.FeatureDepths & features or
                OpFeatures.FeatureStepDown & features):
            if hasattr(opPage, 'taskPanelDepthsPage'):
                page = opPage.taskPanelDepthsPage(obj, features)
            else:
                page = TaskPanelDepthsPage(obj, features)
            self.featurePages.append(page)

        if OpFeatures.FeatureHeights & features:
            if hasattr(opPage, 'taskPanelHeightsPage'):
                page = opPage.taskPanelHeightsPage(obj, features)
            else:
                page = TaskPanelHeightsPage(obj, features)
            self.featurePages.append(page)

        self.featurePages.append(opPage)

        for page in self.featurePages:
            page.setParent(self)
            page.initPage(obj)
            page.onDirtyChanged(self.pageDirtyChanged)

        taskPanelLayout = PathPreferences.defaultTaskPanelLayout()

        if taskPanelLayout < 2:
            opTitle = opPage.getTitle(obj)
            opPage.setTitle(translate('PathOpGui', 'Operation'))
            toolbox = QtGui.QToolBox()
            if taskPanelLayout == 0:
                for page in self.featurePages:
                    toolbox.addItem(page.form, page.getTitle(obj))
                    itemIdx = toolbox.count() - 1
                    if page.icon:
                        toolbox.setItemIcon(itemIdx, QtGui.QIcon(page.icon))
                toolbox.setCurrentIndex(len(self.featurePages)-1)
            else:
                for page in reversed(self.featurePages):
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
            for page in self.featurePages:
                page.form.setWindowTitle(page.getTitle(obj))
                forms.append(page.form)
            self.form = forms
        elif taskPanelLayout == 3:
            forms = []
            for page in reversed(self.featurePages):
                page.form.setWindowTitle(page.getTitle(obj))
                forms.append(page.form)
            self.form = forms

        self.selectionFactory = selectionFactory
        self.obj = obj
        self.isdirty = deleteOnReject

    def setParent(self, parent):
        '''setParent() ...
        Used to transfer parent object link to child class.
        Do not overwrite.
        '''
        self.parent = parent

    def isDirty(self):
        '''isDirty() ...
        Returns true if the model is not in sync with the UI anymore.
        '''
        for page in self.featurePages:
            if page.isdirty:
                return True
        return self.isdirty

    def setClean(self):
        '''setClean() ... set the receiver and all its pages clean.'''
        self.isdirty = False
        for page in self.featurePages:
            page.setClean()

    def accept(self, resetEdit=True):
        '''accept() ...
        Callback invoked when user presses the task panel OK button.
        '''
        self.preCleanup()
        if self.isDirty:
            self.panelGetFields()
        FreeCAD.ActiveDocument.commitTransaction()
        self.cleanup(resetEdit)

    def reject(self, resetEdit=True):
        '''reject() ...
        Callback invoked when user presses the task panel Cancel button.
        '''
        self.preCleanup()
        FreeCAD.ActiveDocument.abortTransaction()
        if self.deleteOnReject:
            msg = translate("Path", "Uncreate AreaOp Operation")
            FreeCAD.ActiveDocument.openTransaction(msg)
            try:
                PathUtil.clearExpressionEngine(self.obj)
                FreeCAD.ActiveDocument.removeObject(self.obj.Name)
            except Exception as ee:
                PathLog.debug('{}\n'.format(ee))
            FreeCAD.ActiveDocument.commitTransaction()
        self.cleanup(resetEdit)
        return True

    def preCleanup(self):
        for page in self.featurePages:
            page.onDirtyChanged(None)
        PathSelection.clear()
        FreeCADGui.Selection.removeObserver(self)
        self.obj.ViewObject.Proxy.clearTaskPanel()

    def cleanup(self, resetEdit):
        '''cleanup() ... implements common cleanup tasks.'''
        self.panelCleanup()
        FreeCADGui.Control.closeDialog()
        if resetEdit:
            FreeCADGui.ActiveDocument.resetEdit()
        FreeCAD.ActiveDocument.recompute()

    def pageDirtyChanged(self, page):
        '''pageDirtyChanged(page) ... internal callback'''
        bBox = self.buttonBox
        bBox.button(QtGui.QDialogButtonBox.Apply).setEnabled(self.isDirty())

    def clicked(self, button):
        '''clicked(button) ...
        Callback invoked when the user presses any of the task panel buttons.
        '''
        if button == QtGui.QDialogButtonBox.Apply:
            self.panelGetFields()
            self.setClean()
            FreeCAD.ActiveDocument.recompute()

    def modifyStandardButtons(self, buttonBox):
        '''modifyStandarButtons(buttonBox) ...
        Callback in case the task panel buttons need to be modified.
        '''
        self.buttonBox = buttonBox
        for page in self.featurePages:
            page.modifyStandardButtons(buttonBox)
        self.pageDirtyChanged(None)

    def panelGetFields(self):
        '''panelGetFields() ...
        Invoked to trigger a complete transfer of UI data to the model.
        '''
        PathLog.track()
        for page in self.featurePages:
            page.pageGetFields()

    def panelSetFields(self):
        '''panelSetFields() ...
        Invoked to trigger a complete transfer of the model's properties
        to the UI.
        '''
        PathLog.track()
        for page in self.featurePages:
            page.pageSetFields()

    def panelCleanup(self):
        '''panelCleanup() ... invoked before the receiver is destroyed.'''
        PathLog.track()
        for page in self.featurePages:
            page.pageCleanup()

    def open(self):
        '''open() ... callback invoked when the task panel is opened.'''
        self.selectionFactory()
        FreeCADGui.Selection.addObserver(self)

    def getStandardButtons(self):
        '''getStandardButtons() ... returns the Buttons for the task panel.'''
        return int(QtGui.QDialogButtonBox.Ok |
                   QtGui.QDialogButtonBox.Apply |
                   QtGui.QDialogButtonBox.Cancel)

    def setupUi(self):
        '''setupUi() ... internal function to initialise all pages.'''
        PathLog.track(self.deleteOnReject)

        proxy = self.obj.Proxy
        if (self.deleteOnReject and
                OpFeatures.FeatureBaseGeometry & proxy.opFeatures(self.obj)):
            sel = FreeCADGui.Selection.getSelectionEx()
            for page in self.featurePages:
                if (getattr(page, 'InitBase', True) and
                        hasattr(page, 'addBase')):
                    page.clearBase()
                    page.addBaseGeometry(sel)

        self.panelSetFields()
        for page in self.featurePages:
            page.pageRegisterSignalHandlers()

    def updateData(self, obj, prop):
        '''updateDate(obj, prop) ...
        Callback invoked whenever a model's property is assigned a value.
        '''
        # PathLog.track(obj.Label, prop) # creates a lot of noise
        for page in self.featurePages:
            page.pageUpdateData(obj, prop)

    def needsFullSpace(self):
        return True

    def updateSelection(self):
        sel = FreeCADGui.Selection.getSelectionEx()
        for page in self.featurePages:
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


class CommandSetStartPoint:
    '''Command to set the start point for an operation.'''
    # pylint: disable=no-init

    def GetResources(self):
        return {'Pixmap': 'Path-StartPoint',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path",
                                                     "Pick Start Point"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path",
                                                    "Pick Start Point")}

    def IsActive(self):
        if FreeCAD.ActiveDocument is None:
            return False
        sel = FreeCADGui.Selection.getSelection()
        if not sel:
            return False
        obj = sel[0]
        return obj and hasattr(obj, 'StartPoint')

    def setpoint(self, point, o):
        # pylint: disable=unused-argument
        obj = FreeCADGui.Selection.getSelection()[0]
        obj.StartPoint.x = point.x
        obj.StartPoint.y = point.y
        obj.StartPoint.z = obj.ClearanceHeight.Value

    def Activated(self):
        FreeCADGui.Snapper.getPoint(callback=self.setpoint)
# Eclass


class CommandPathOp:
    '''
    Generic, data driven implementation of a Path operation creation command.
    Instances of this class are stored in all Path operation Gui modules and
    can be used to create said operations with view providers and all.
    '''

    def __init__(self, resources):
        self.res = resources

    def GetResources(self):
        ress = {'Pixmap': self.res.pixmap,
                'MenuText': self.res.menuText,
                'ToolTip': self.res.toolTip}
        if self.res.accelKey:
            ress['Accel'] = self.res.accelKey
        return ress

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                    return True
        return False

    def Activated(self):
        return Create(self.res)
# Eclass


class CommandResources:
    '''POD class to hold command specific resources.'''
    def __init__(self, name, objFactory, opPageClass,
                 pixmap, menuText, accelKey, toolTip):
        self.name = name
        self.objFactory = objFactory
        self.opPageClass = opPageClass
        self.pixmap = pixmap
        self.menuText = menuText
        self.accelKey = accelKey
        self.toolTip = toolTip
# Eclass


def Create(res):
    '''Create(res) ... generic implementation of a create function.
    res is an instance of CommandResources. It is not expected that
    the user invokes this function directly, but calls the Activated()
    function of the Command object that is created in each operation's
    Gui implementation.
    '''
    FreeCAD.ActiveDocument.openTransaction("Create %s" % res.name)
    obj = res.objFactory(res.name)
    if obj.Proxy:
        obj.ViewObject.Proxy = ViewProvider(obj.ViewObject, res)

        FreeCAD.ActiveDocument.commitTransaction()
        obj.ViewObject.Document.setEdit(obj.ViewObject, 0)
        return obj
    FreeCAD.ActiveDocument.abortTransaction()
    return None


def SetupOperation(name,
                   objFactory,
                   opPageClass,
                   pixmap,
                   menuText,
                   toolTip,
                   setupProperties=None):
    '''
    SetupOperation(name, objFactory,
                   opPageClass, pixmap,
                   menuText, toolTip,
                   setupProperties=None)
    Creates an instance of CommandPathOp with the given parameters and
    registers the command with FreeCAD.
    When activated it creates a model with proxy (by invoking objFactory),
    assigns a view provider to it (see ViewProvider in this module) and starts
    the editor specifically for this operation (driven by opPageClass).
    This is an internal function that is automatically called by
    the initialisation code for each operation.
    It is not expected to be called manually.
    '''

    res = CommandResources(name, objFactory, opPageClass, pixmap,
                           menuText, None, toolTip)

    command = CommandPathOp(res)
    FreeCADGui.addCommand("Path_%s" % name.replace(' ', '_'), command)

    if setupProperties is not None:
        PathSetupSheet.RegisterOperation(name, objFactory, setupProperties)

    return command


FreeCADGui.addCommand('Path_SetStartPoint', CommandSetStartPoint())

FreeCAD.Console.PrintLog("Loading PathOpGui... done\n")
