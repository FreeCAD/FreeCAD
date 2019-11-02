# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
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
import FreeCAD
import FreeCADGui
import PathScripts.PathDressupPathBoundary as PathDressupPathBoundary
import PathScripts.PathLog as PathLog

from PySide import QtGui, QtCore

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule()


# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class TaskPanel(object):

    def __init__(self, obj, viewProvider):
        self.obj = obj
        self.viewProvider = viewProvider
        self.form = FreeCADGui.PySideUic.loadUi(':/panels/DressupPathBoundary.ui')
        self.visibilityBase = obj.Base.ViewObject.Visibility if obj.Base else None
        self.visibilityBoundary = obj.Boundary.ViewObject.Visibility if obj.Boundary else None

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Apply | QtGui.QDialogButtonBox.Cancel)

    def abort(self):
        FreeCAD.ActiveDocument.abortTransaction()
        self.cleanup(False)

    def reject(self):
        FreeCAD.ActiveDocument.abortTransaction()
        self.cleanup(True)

    def accept(self):
        FreeCAD.ActiveDocument.commitTransaction()
        self.cleanup(True)
        #if self.isDirty:
        #    self.getFields()
        #    FreeCAD.ActiveDocument.recompute()

    def cleanup(self, gui):
        self.viewProvider.clearTaskPanel()
        if gui:
            FreeCADGui.ActiveDocument.resetEdit()
            FreeCADGui.Control.closeDialog()
            FreeCAD.ActiveDocument.recompute()
            if self.obj.Base:
                self.obj.Base.ViewObject.Visibility = self.visibilityBase
            if self.obj.Boundary:
                self.obj.Boundary.ViewObject.Visibility = self.visibilityBoundary

    def getFields(self):
        pass
    def setFields(self):
        pass

    def setupUi(self):
        pass


class DressupPathBoundaryViewProvider(object):

    def __init__(self, vobj):
        self.attach(vobj)

    def __getstate__(self):
        return None
    def __setstate_(self, state):
        return None


    def attach(self, vobj):
        self.vobj = vobj
        self.obj = vobj.Object
        self.panel = None

    def claimChildren(self):
        return [self.obj.Base, self.obj.Boundary]

    def onDelete(self, vobj, args=None):
        vobj.Object.Proxy.onDelete(vobj.Object, args)

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


def Create(base, name='DressupPathBoundary'):
    FreeCAD.ActiveDocument.openTransaction(translate('Path_DressupPathBoundary', 'Create a Boundary dressup'))
    obj = PathDressupPathBoundary.Create(base, name)
    obj.ViewObject.Proxy = DressupPathBoundaryViewProvider(obj.ViewObject)
    FreeCAD.ActiveDocument.commitTransaction()
    obj.ViewObject.Document.setEdit(obj.ViewObject, 0)
    return obj

class CommandPathDressupPathBoundary:
    # pylint: disable=no-init

    def GetResources(self):
        return {'Pixmap': 'Path-Dressup',
                'MenuText': QtCore.QT_TRANSLATE_NOOP('Path_DressupPathBoundary', 'Boundary Dress-up'),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP('Path_DressupPathBoundary', 'Creates a Path Boundary Dress-up object from a selected path')}

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
            PathLog.error(translate('Path_DressupPathBoundary', 'Please select one path object')+'\n')
            return
        baseObject = selection[0]

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction(translate('Path_DressupPathBoundary', 'Create Path Boundary Dress-up'))
        FreeCADGui.addModule('PathScripts.PathDressupPathBoundaryGui')
        FreeCADGui.doCommand("PathScripts.PathDressupPathBoundaryGui.Create(App.ActiveDocument.%s)" % baseObject.Name)
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_DressupPathBoundary', CommandPathDressupPathBoundary())

PathLog.notice('Loading PathDressupPathBoundaryGui... done\n')
