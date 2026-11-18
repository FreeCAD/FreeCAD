# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2017 sliptonic shopinthewoods@gmail.com
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

import FreeCAD
import FreeCADGui
import Path
import Path.Op.Engrave as PathEngrave
import Path.Op.Gui.Base as PathOpGui

from PySide import QtCore, QtGui

__title__ = "CAM Engrave Operation UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Engrave operation page controller and command implementation."

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

    def selectionSupportedAsBaseGeometry(self, sel, ignoreErrors):
        # allow selection of an entire 2D object, which is generally not the case
        if (
            not sel.HasSubObjects
            and sel.Object.isDerivedFrom("Part::Feature")
            and Path.Geom.isRoughly(sel.Object.Shape.Volume, 0)
        ):
            return True

        # Let general logic handle all other cases.
        return self.super().selectionSupportedAsBaseGeometry(sel, ignoreErrors)

    def addBaseGeometry(self, selection):
        added = False
        shapes = self.obj.BaseShapes
        for sel in selection:
            base = sel.Object
            if base in shapes:
                Path.Log.notice(
                    (translate("CAM", "Base shape %s already in the list") + "\n")
                    % (sel.Object.Label)
                )
                continue
            if base.isDerivedFrom("Part::Feature") and Path.Geom.isRoughly(base.Shape.Volume, 0):
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
            elif self.super().addBaseGeometry(selection):
                # user wants us to engrave an edge of face of a base model
                added = True

        return added

    def clearBase(self):
        self.obj.BaseShapes = []
        self.super().clearBase()

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
        Path.Log.debug("Setting new base shapes: %s -> %s" % (self.obj.BaseShapes, shapes))
        self.obj.BaseShapes = shapes
        return self.super().updateBase()


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    """Page controller class for the Engrave operation."""

    def initPage(self, obj):
        self.form.cutPattern.setToolTip(
            translate("App::Property", self.obj.getDocumentationOfProperty("CutPattern"))
        )
        self.form.sorting.setToolTip(
            translate("App::Property", self.obj.getDocumentationOfProperty("SortingMode"))
        )
        self.form.startVertex.setToolTip(
            translate("App::Property", self.obj.getDocumentationOfProperty("StartVertex"))
        )
        self.form.chkReverse.setToolTip(
            translate("App::Property", self.obj.getDocumentationOfProperty("Reverse"))
        )
        self.form.chkApproximation.setToolTip(
            translate("App::Property", self.obj.getDocumentationOfProperty("Approximation"))
        )

    def getForm(self):
        """getForm() ... returns UI"""
        form = FreeCADGui.PySideUic.loadUi(":/panels/PageOpEngraveEdit.ui")

        comboToPropertyMap = [("cutPattern", "CutPattern"), ("sorting", "SortingMode")]
        enumTups = PathEngrave.ObjectEngrave.engraveOpPropertyEnumerations(dataType="raw")
        self.populateCombobox(form, enumTups, comboToPropertyMap)

        return form

    def getFields(self, obj):
        """getFields(obj) ... transfers values from UI to obj's properties"""
        if obj.CutPattern != str(self.form.cutPattern.currentData()):
            obj.CutPattern = str(self.form.cutPattern.currentData())
        if obj.SortingMode != str(self.form.sorting.currentData()):
            obj.SortingMode = str(self.form.sorting.currentData())
        if obj.StartVertex != self.form.startVertex.value():
            obj.StartVertex = self.form.startVertex.value()
        if obj.Reverse != self.form.chkReverse.isChecked():
            obj.Reverse = self.form.chkReverse.isChecked()
        if obj.Approximation != self.form.chkApproximation.isChecked():
            obj.Approximation = self.form.chkApproximation.isChecked()

    def setFields(self, obj):
        """setFields(obj) ... transfers obj's property values to UI"""
        self.form.startVertex.setValue(obj.StartVertex)
        self.selectInComboBox(obj.CutPattern, self.form.cutPattern)
        self.selectInComboBox(obj.SortingMode, self.form.sorting)
        self.form.chkReverse.setChecked(obj.Reverse)
        self.form.chkApproximation.setChecked(obj.Approximation)

    def getSignalsForUpdate(self, obj):
        """getSignalsForUpdate(obj) ... return list of signals for updating obj"""
        signals = []
        signals.append(self.form.startVertex.editingFinished)
        signals.append(self.form.cutPattern.currentIndexChanged)
        signals.append(self.form.sorting.currentIndexChanged)
        signals.append(self.form.chkReverse.checkStateChanged)
        signals.append(self.form.chkApproximation.checkStateChanged)
        return signals

    def taskPanelBaseGeometryPage(self, obj, features):
        """taskPanelBaseGeometryPage(obj, features) ... return page for adding base geometries."""
        return TaskPanelBaseGeometryPage(obj, features)


Command = PathOpGui.SetupOperation(
    "Engrave",
    PathEngrave.Create,
    TaskPanelOpPage,
    "CAM_Engrave",
    QtCore.QT_TRANSLATE_NOOP("CAM_Engrave", "Engrave"),
    QtCore.QT_TRANSLATE_NOOP(
        "CAM_Engrave", "Creates an Engraving toolpath around a Draft ShapeString"
    ),
    PathEngrave.SetupProperties,
)

FreeCAD.Console.PrintLog("Loading PathEngraveGui... done\n")
