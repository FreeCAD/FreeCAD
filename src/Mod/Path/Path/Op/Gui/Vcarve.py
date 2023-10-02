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
import Path
import Path.Op.Gui.Base as PathOpGui
import Path.Op.Vcarve as PathVcarve
import PathGui
import PathScripts.PathUtils as PathUtils
from PySide import QtCore, QtGui


__title__ = "Path Vcarve Operation UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecad.org"
__doc__ = "Vcarve operation page controller and command implementation."


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate


class TaskPanelBaseGeometryPage(PathOpGui.TaskPanelBaseGeometryPage):
    """Enhanced base geometry page to also allow special base objects."""

    def super(self):
        return super(TaskPanelBaseGeometryPage, self)

    def addBaseGeometry(self, selection):
        Path.Log.track(selection)
        added = False
        shapes = self.obj.BaseShapes
        for sel in selection:
            job = PathUtils.findParentJob(self.obj)
            base = job.Proxy.resourceClone(job, sel.Object)
            if not base:
                Path.Log.notice(
                    (
                        translate("Path", "%s is not a Base Model object of the job %s")
                        + "\n"
                    )
                    % (sel.Object.Label, job.Label)
                )
                continue
            if base in shapes:
                Path.Log.notice(
                    "Base shape %s already in the list".format(sel.Object.Label)
                )
                continue
            if base.isDerivedFrom("Part::Part2DObject"):
                if sel.HasSubObjects:
                    # selectively add some elements of the drawing to the Base
                    for sub in sel.SubElementNames:
                        if "Vertex" in sub:
                            Path.Log.info("Ignoring vertex")
                        else:
                            self.obj.Proxy.addBase(self.obj, base, sub)
                else:
                    # when adding an entire shape to BaseShapes we can take its sub shapes out of Base
                    self.obj.Base = [(p, el) for p, el in self.obj.Base if p != base]
                    shapes.append(base)
                    self.obj.BaseShapes = shapes
                added = True

        if not added:
            # user wants us to engrave an edge of face of a base model
            Path.Log.info("  call default")
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
        Path.Log.track()
        shapes = []
        for i in range(self.form.baseList.count()):
            item = self.form.baseList.item(i)
            obj = item.data(self.super().DataObject)
            sub = item.data(self.super().DataObjectSub)
            if not sub:
                shapes.append(obj)
        Path.Log.debug(
            "Setting new base shapes: %s -> %s" % (self.obj.BaseShapes, shapes)
        )
        self.obj.BaseShapes = shapes
        return self.super().updateBase()


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    """Page controller class for the Vcarve operation."""

    def getForm(self):
        """getForm() ... returns UI"""
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpVcarveEdit.ui")

    def getFields(self, obj):
        """getFields(obj) ... transfers values from UI to obj's properties"""
        if obj.Discretize != self.form.discretize.value():
            obj.Discretize = self.form.discretize.value()
        if obj.Colinear != self.form.colinearFilter.value():
            obj.Colinear = self.form.colinearFilter.value()
        self.updateToolController(obj, self.form.toolController)
        self.updateCoolant(obj, self.form.coolantController)

    def setFields(self, obj):
        """setFields(obj) ... transfers obj's property values to UI"""
        self.form.discretize.setValue(obj.Discretize)
        self.form.colinearFilter.setValue(obj.Colinear)
        self.setupToolController(obj, self.form.toolController)
        self.setupCoolant(obj, self.form.coolantController)

    def getSignalsForUpdate(self, obj):
        """getSignalsForUpdate(obj) ... return list of signals for updating obj"""
        signals = []
        signals.append(self.form.discretize.editingFinished)
        signals.append(self.form.colinearFilter.editingFinished)
        signals.append(self.form.toolController.currentIndexChanged)
        signals.append(self.form.coolantController.currentIndexChanged)
        return signals

    def taskPanelBaseGeometryPage(self, obj, features):
        """taskPanelBaseGeometryPage(obj, features) ... return page for adding base geometries."""
        return TaskPanelBaseGeometryPage(obj, features)


Command = PathOpGui.SetupOperation(
    "Vcarve",
    PathVcarve.Create,
    TaskPanelOpPage,
    "Path_Vcarve",
    QtCore.QT_TRANSLATE_NOOP("Path_Vcarve", "Vcarve"),
    QtCore.QT_TRANSLATE_NOOP("Path_Vcarve", "Creates a medial line engraving path"),
    PathVcarve.SetupProperties,
)

FreeCAD.Console.PrintLog("Loading PathVcarveGui... done\n")
