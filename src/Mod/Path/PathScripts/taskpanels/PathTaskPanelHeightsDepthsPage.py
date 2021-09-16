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

import PathScripts.PathGeom as PathGeom
import PathScripts.PathGui as PathGui
import PathScripts.PathLog as PathLog
import PathScripts.operations.PathOp2 as PathOp2
import PathScripts.taskpanels.PathTaskPanelPage as PathTaskPanelPage

from PySide import QtGui


__title__ = "Path UI Task Panel Pages base classes"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecadweb.org"
__doc__ = "Base classes for UI features within Path operations"


FreeCAD = PathTaskPanelPage.FreeCAD
FreeCADGui = PathTaskPanelPage.FreeCADGui
translate = PathTaskPanelPage.translate


PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
# PathLog.trackModule(PathLog.thisModule())


class TaskPanelHeightsDepthsPage(PathTaskPanelPage.TaskPanelPage):
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
        return translate("PathOp2", "Heights and Depths")

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


FreeCAD.Console.PrintLog("Loading PathTaskPanelHeightsDepthsPage... done\n")
