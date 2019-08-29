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

import FreeCAD
import FreeCADGui
import PathScripts.PathEngrave as PathEngrave
import PathScripts.PathLog as PathLog
import PathScripts.PathOpGui as PathOpGui
import PathScripts.PathUtils as PathUtils

from PySide import QtCore, QtGui

__title__ = "Path Engrave Operation UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Engrave operation page controller and command implementation."

LOGLEVEL = False

if LOGLEVEL:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class TaskPanelBaseGeometryPage(PathOpGui.TaskPanelBaseGeometryPage):
    '''Enhanced base geometry page to also allow special base objects.'''

    def super(self):
        return super(TaskPanelBaseGeometryPage, self)

    def addBaseGeometry(self, selection):
        added = False
        shapes = self.obj.BaseShapes
        for sel in selection:
            job = PathUtils.findParentJob(self.obj)
            base = job.Proxy.resourceClone(job, sel.Object)
            if not base:
                PathLog.notice((translate("Path", "%s is not a Base Model object of the job %s")+"\n") % (sel.Object.Label, job.Label))
                continue
            if base in shapes:
                PathLog.notice((translate("Path", "Base shape %s already in the list")+"\n") % (sel.Object.Label))
                continue
            if base.isDerivedFrom('Part::Part2DObject'):
                if sel.HasSubObjects:
                    # selectively add some elements of the drawing to the Base
                    for sub in sel.SubElementNames:
                        if 'Vertex' in sub:
                            PathLog.info(translate("Path", "Ignoring vertex"))
                        else:
                            self.obj.Proxy.addBase(self.obj, base, sub)
                else:
                    # when adding an entire shape to BaseShapes we can take its sub shapes out of Base
                    self.obj.Base = [(p,el) for p,el in self.obj.Base if p != base]
                    shapes.append(base)
                    self.obj.BaseShapes = shapes
                added = True
            else:
                # user wants us to engrave an edge of face of a base model
                base = self.super().addBaseGeometry(selection)
                added = added or base

        return added

    def setFields(self, obj):
        self.super().setFields(obj)
        self.form.baseList.blockSignals(True)
        for shape in self.obj.BaseShapes:
            item = QtGui.QListWidgetItem(shape.Label)
            item.setData(self.super().DataObject, shape)
            item.setData(self.super().DataObjectSub, None)
            self.form.baseList.addItem(item)
        self.form.baseList.blockSignals(False)

    def updateBase(self):
        PathLog.track()
        shapes = []
        for i in range(self.form.baseList.count()):
            item = self.form.baseList.item(i)
            obj = item.data(self.super().DataObject)
            sub = item.data(self.super().DataObjectSub)
            if not sub:
                shapes.append(obj)
        PathLog.debug("Setting new base shapes: %s -> %s" % (self.obj.BaseShapes, shapes))
        self.obj.BaseShapes = shapes
        return self.super().updateBase()

class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    '''Page controller class for the Engrave operation.'''

    def getForm(self):
        '''getForm() ... returns UI'''
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpEngraveEdit.ui")

    def getFields(self, obj):
        '''getFields(obj) ... transfers values from UI to obj's proprties'''
        if obj.StartVertex != self.form.startVertex.value():
            obj.StartVertex = self.form.startVertex.value()
        self.updateToolController(obj, self.form.toolController)

    def setFields(self, obj):
        '''setFields(obj) ... transfers obj's property values to UI'''
        self.form.startVertex.setValue(obj.StartVertex)
        self.setupToolController(obj, self.form.toolController)

    def getSignalsForUpdate(self, obj):
        '''getSignalsForUpdate(obj) ... return list of signals for updating obj'''
        signals = []
        signals.append(self.form.startVertex.editingFinished)
        signals.append(self.form.toolController.currentIndexChanged)
        return signals

    def taskPanelBaseGeometryPage(self, obj, features):
        '''taskPanelBaseGeometryPage(obj, features) ... return page for adding base geometries.'''
        return TaskPanelBaseGeometryPage(obj, features)

Command = PathOpGui.SetupOperation('Engrave',
        PathEngrave.Create,
        TaskPanelOpPage,
        'Path-Engrave',
        QtCore.QT_TRANSLATE_NOOP("PathEngrave", "Engrave"),
        QtCore.QT_TRANSLATE_NOOP("PathEngrave", "Creates an Engraving Path around a Draft ShapeString"),
        PathEngrave.SetupProperties)

FreeCAD.Console.PrintLog("Loading PathEngraveGui... done\n")
