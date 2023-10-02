# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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
import Path
import Path.Main.Stock as PathStock
from Path.Post.Processor import PostProcessor
import json

from FreeCAD import Units
from PySide import QtCore, QtGui


Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class JobPreferencesPage:
    def __init__(self, parent=None):
        import FreeCADGui

        self.form = FreeCADGui.PySideUic.loadUi(":preferences/PathJob.ui")
        self.form.toolBox.setCurrentIndex(0)  # Take that qt designer!

        self.postProcessorDefaultTooltip = self.form.defaultPostProcessor.toolTip()
        self.postProcessorArgsDefaultTooltip = (
            self.form.defaultPostProcessorArgs.toolTip()
        )
        self.processor = {}

    def saveSettings(self):
        filePath = self.form.leDefaultFilePath.text()
        jobTemplate = self.form.leDefaultJobTemplate.text()
        geometryTolerance = Units.Quantity(self.form.geometryTolerance.text())
        curveAccuracy = Units.Quantity(self.form.curveAccuracy.text())
        Path.Preferences.setJobDefaults(
            filePath, jobTemplate, geometryTolerance, curveAccuracy
        )

        if curveAccuracy:
            Path.Area.setDefaultParams(Accuracy=curveAccuracy)

        processor = str(self.form.defaultPostProcessor.currentText())
        args = str(self.form.defaultPostProcessorArgs.text())
        blacklist = []
        for i in range(0, self.form.postProcessorList.count()):
            item = self.form.postProcessorList.item(i)
            if item.checkState() == QtCore.Qt.CheckState.Unchecked:
                blacklist.append(item.text())
        Path.Preferences.setPostProcessorDefaults(processor, args, blacklist)

        path = str(self.form.leOutputFile.text())
        policy = str(self.form.cboOutputPolicy.currentText())
        Path.Preferences.setOutputFileDefaults(path, policy)
        self.saveStockSettings()
        self.saveToolsSettings()

    def saveStockSettings(self):
        if self.form.stockGroup.isChecked():
            attrs = {}
            attrs["version"] = 1
            typ = [
                PathStock.StockType.CreateBox,
                PathStock.StockType.CreateCylinder,
                PathStock.StockType.FromBase,
            ][self.form.stock.currentIndex()]
            attrs["create"] = typ
            if typ == PathStock.StockType.CreateBox:
                attrs["length"] = FreeCAD.Units.Quantity(
                    self.form.stockBoxLength.text()
                ).UserString
                attrs["width"] = FreeCAD.Units.Quantity(
                    self.form.stockBoxWidth.text()
                ).UserString
                attrs["height"] = FreeCAD.Units.Quantity(
                    self.form.stockBoxHeight.text()
                ).UserString
            if typ == PathStock.StockType.CreateCylinder:
                attrs["radius"] = FreeCAD.Units.Quantity(
                    self.form.stockCylinderRadius.text()
                ).UserString
                attrs["height"] = FreeCAD.Units.Quantity(
                    self.form.stockCylinderHeight.text()
                ).UserString
            if typ == PathStock.StockType.FromBase:
                attrs["xneg"] = FreeCAD.Units.Quantity(
                    self.form.stockExtXneg.text()
                ).UserString
                attrs["xpos"] = FreeCAD.Units.Quantity(
                    self.form.stockExtXpos.text()
                ).UserString
                attrs["yneg"] = FreeCAD.Units.Quantity(
                    self.form.stockExtYneg.text()
                ).UserString
                attrs["ypos"] = FreeCAD.Units.Quantity(
                    self.form.stockExtYpos.text()
                ).UserString
                attrs["zneg"] = FreeCAD.Units.Quantity(
                    self.form.stockExtZneg.text()
                ).UserString
                attrs["zpos"] = FreeCAD.Units.Quantity(
                    self.form.stockExtZpos.text()
                ).UserString
            if self.form.stockPlacementGroup.isChecked():
                angle = FreeCAD.Units.Quantity(self.form.stockAngle.text()).Value
                axis = FreeCAD.Vector(
                    self.form.stockAxisX.value(),
                    self.form.stockAxisY.value(),
                    self.form.stockAxisZ.value(),
                )
                rot = FreeCAD.Rotation(axis, angle)
                attrs["rotX"] = rot.Q[0]
                attrs["rotY"] = rot.Q[1]
                attrs["rotZ"] = rot.Q[2]
                attrs["rotW"] = rot.Q[3]
                attrs["posX"] = FreeCAD.Units.Quantity(
                    self.form.stockPositionX.text()
                ).Value
                attrs["posY"] = FreeCAD.Units.Quantity(
                    self.form.stockPositionY.text()
                ).Value
                attrs["posZ"] = FreeCAD.Units.Quantity(
                    self.form.stockPositionZ.text()
                ).Value
            Path.Preferences.setDefaultStockTemplate(json.dumps(attrs))
        else:
            Path.Preferences.setDefaultStockTemplate("")

    def saveToolsSettings(self):
        Path.Preferences.setToolsSettings(self.form.toolsAbsolutePaths.isChecked())

    def selectComboEntry(self, widget, text):
        index = widget.findText(text, QtCore.Qt.MatchFixedString)
        if index >= 0:
            widget.blockSignals(True)
            widget.setCurrentIndex(index)
            widget.blockSignals(False)

    def verifyAndUpdateDefaultPostProcessorWith(self, processor):
        defaultIsValid = False
        self.form.defaultPostProcessor.blockSignals(True)
        self.form.defaultPostProcessor.clear()
        self.form.defaultPostProcessor.addItem("")
        for i in range(0, self.form.postProcessorList.count()):
            item = self.form.postProcessorList.item(i)
            if item.checkState() == QtCore.Qt.CheckState.Checked:
                self.form.defaultPostProcessor.addItem(item.text())
                if item.text() == processor:
                    defaultIsValid = True
        # if we get here the default processor was disabled
        if not defaultIsValid:
            self.form.defaultPostProcessorArgs.setText("")
            processor = ""
        self.selectComboEntry(self.form.defaultPostProcessor, processor)
        self.form.defaultPostProcessor.blockSignals(False)

    def verifyAndUpdateDefaultPostProcessor(self):
        self.verifyAndUpdateDefaultPostProcessorWith(
            str(self.form.defaultPostProcessor.currentText())
        )

    def loadSettings(self):
        self.form.leDefaultFilePath.setText(Path.Preferences.defaultFilePath())
        self.form.leDefaultJobTemplate.setText(Path.Preferences.defaultJobTemplate())

        blacklist = Path.Preferences.postProcessorBlacklist()
        for processor in Path.Preferences.allAvailablePostProcessors():
            item = QtGui.QListWidgetItem(processor)
            if processor in blacklist:
                item.setCheckState(QtCore.Qt.CheckState.Unchecked)
            else:
                item.setCheckState(QtCore.Qt.CheckState.Checked)
            item.setFlags(
                QtCore.Qt.ItemFlag.ItemIsSelectable
                | QtCore.Qt.ItemFlag.ItemIsEnabled
                | QtCore.Qt.ItemFlag.ItemIsUserCheckable
            )
            self.form.postProcessorList.addItem(item)
        self.verifyAndUpdateDefaultPostProcessorWith(
            Path.Preferences.defaultPostProcessor()
        )

        self.form.defaultPostProcessorArgs.setText(
            Path.Preferences.defaultPostProcessorArgs()
        )

        geomTol = Units.Quantity(
            Path.Preferences.defaultGeometryTolerance(), Units.Length
        )
        self.form.geometryTolerance.setText(geomTol.UserString)
        self.form.curveAccuracy.setText(
            Units.Quantity(
                Path.Preferences.defaultLibAreaCurveAccuracy(), Units.Length
            ).UserString
        )

        self.form.leOutputFile.setText(Path.Preferences.defaultOutputFile())
        self.selectComboEntry(
            self.form.cboOutputPolicy, Path.Preferences.defaultOutputPolicy()
        )

        self.form.tbDefaultFilePath.clicked.connect(self.browseDefaultFilePath)
        self.form.tbDefaultJobTemplate.clicked.connect(self.browseDefaultJobTemplate)
        self.form.postProcessorList.itemEntered.connect(self.setProcessorListTooltip)
        self.form.postProcessorList.itemChanged.connect(
            self.verifyAndUpdateDefaultPostProcessor
        )
        self.form.defaultPostProcessor.currentIndexChanged.connect(
            self.updateDefaultPostProcessorToolTip
        )
        self.form.tbOutputFile.clicked.connect(self.browseOutputFile)

        self.loadStockSettings()
        self.loadToolSettings()

    def loadStockSettings(self):
        stock = Path.Preferences.defaultStockTemplate()
        index = -1
        if stock:
            attrs = json.loads(stock)
            if attrs.get("version") and 1 == int(attrs["version"]):
                stockType = attrs.get("create")
                if stockType == PathStock.StockType.FromBase:
                    index = 2
                elif stockType == PathStock.StockType.CreateBox:
                    index = 0
                elif stockType == PathStock.StockType.CreateCylinder:
                    index = 1
                else:
                    index = -1
        if -1 == index:
            attrs = {}
            self.form.stockGroup.setChecked(False)
        else:
            self.form.stockGroup.setChecked(True)
            self.form.stock.setCurrentIndex(index)

        # this either sets the default value or the value from the template for each field
        self.form.stockExtXneg.setText(attrs.get("xneg", "1 mm"))
        self.form.stockExtXpos.setText(attrs.get("xpos", "1 mm"))
        self.form.stockExtYneg.setText(attrs.get("yneg", "1 mm"))
        self.form.stockExtYpos.setText(attrs.get("ypos", "1 mm"))
        self.form.stockExtZneg.setText(attrs.get("zneg", "1 mm"))
        self.form.stockExtZpos.setText(attrs.get("zpos", "1 mm"))
        self.form.stockBoxLength.setText(attrs.get("length", "10 mm"))
        self.form.stockBoxWidth.setText(attrs.get("width", "10 mm"))
        self.form.stockBoxHeight.setText(attrs.get("height", "10 mm"))
        self.form.stockCylinderRadius.setText(attrs.get("radius", "5 mm"))
        self.form.stockCylinderHeight.setText(attrs.get("height", "10 mm"))

        posX = attrs.get("posX")
        posY = attrs.get("posY")
        posZ = attrs.get("posZ")
        rotX = attrs.get("rotX")
        rotY = attrs.get("rotY")
        rotZ = attrs.get("rotZ")
        rotW = attrs.get("rotW")
        if (
            posX is not None
            and posY is not None
            and posZ is not None
            and rotX is not None
            and rotY is not None
            and rotZ is not None
            and rotW is not None
        ):
            pos = FreeCAD.Vector(float(posX), float(posY), float(posZ))
            rot = FreeCAD.Rotation(float(rotX), float(rotY), float(rotZ), float(rotW))
            placement = FreeCAD.Placement(pos, rot)
            self.form.stockPlacementGroup.setChecked(True)
        else:
            placement = FreeCAD.Placement()
            self.form.stockPlacementGroup.setChecked(False)

        self.form.stockAngle.setText(
            FreeCAD.Units.Quantity("%f rad" % placement.Rotation.Angle).UserString
        )
        self.form.stockAxisX.setValue(placement.Rotation.Axis.x)
        self.form.stockAxisY.setValue(placement.Rotation.Axis.y)
        self.form.stockAxisZ.setValue(placement.Rotation.Axis.z)
        self.form.stockPositionX.setText(
            FreeCAD.Units.Quantity(placement.Base.x, FreeCAD.Units.Length).UserString
        )
        self.form.stockPositionY.setText(
            FreeCAD.Units.Quantity(placement.Base.y, FreeCAD.Units.Length).UserString
        )
        self.form.stockPositionZ.setText(
            FreeCAD.Units.Quantity(placement.Base.z, FreeCAD.Units.Length).UserString
        )

        self.setupStock(index)
        self.form.stock.currentIndexChanged.connect(self.setupStock)

    def setupStock(self, index):
        if 0 == index:
            self.form.stockFromBase.hide()
            self.form.stockCreateBox.show()
            self.form.stockCreateCylinder.hide()
        elif 1 == index:
            self.form.stockFromBase.hide()
            self.form.stockCreateBox.hide()
            self.form.stockCreateCylinder.show()
        else:
            self.form.stockFromBase.show()
            self.form.stockCreateBox.hide()
            self.form.stockCreateCylinder.hide()

    def loadToolSettings(self):
        self.form.toolsAbsolutePaths.setChecked(
            Path.Preferences.toolsStoreAbsolutePaths()
        )

    def getPostProcessor(self, name):
        if not name in self.processor:
            processor = PostProcessor.load(name)
            self.processor[name] = processor
            return processor
        return self.processor[name]

    def setPostProcessorTooltip(self, widget, name, default):
        processor = self.getPostProcessor(name)
        if processor.tooltip:
            widget.setToolTip(processor.tooltip)
        else:
            widget.setToolTip(default)

    def setProcessorListTooltip(self, item):
        self.setPostProcessorTooltip(self.form.postProcessorList, item.text(), "")

    def updateDefaultPostProcessorToolTip(self):
        name = str(self.form.defaultPostProcessor.currentText())
        if name:
            self.setPostProcessorTooltip(
                self.form.defaultPostProcessor, name, self.postProcessorDefaultTooltip
            )
            processor = self.getPostProcessor(name)
            if processor.tooltipArgs:
                self.form.defaultPostProcessorArgs.setToolTip(processor.tooltipArgs)
            else:
                self.form.defaultPostProcessorArgs.setToolTip(
                    self.postProcessorArgsDefaultTooltip
                )
        else:
            self.form.defaultPostProcessor.setToolTip(self.postProcessorDefaultTooltip)
            self.form.defaultPostProcessorArgs.setToolTip(
                self.postProcessorArgsDefaultTooltip
            )

    def bestGuessForFilePath(self):
        path = self.form.leDefaultFilePath.text()
        if not path:
            path = Path.Preferences.filePath()
        return path

    def browseDefaultJobTemplate(self):
        path = self.form.leDefaultJobTemplate.text()
        if not path:
            path = self.bestGuessForFilePath()
        foo = QtGui.QFileDialog.getOpenFileName(
            QtGui.QApplication.activeWindow(), "Path - Job Template", path, "job_*.json"
        )[0]
        if foo:
            self.form.leDefaultJobTemplate.setText(foo)

    def browseDefaultFilePath(self):
        path = self.bestGuessForFilePath()
        foo = QtGui.QFileDialog.getExistingDirectory(
            QtGui.QApplication.activeWindow(), "Path - External File Directory", path
        )
        if foo:
            self.form.leDefaultFilePath.setText(foo)

    def browseOutputFile(self):
        path = self.form.leOutputFile.text()
        foo = QtGui.QFileDialog.getExistingDirectory(
            QtGui.QApplication.activeWindow(), "Path - Output File/Directory", path
        )
        if foo:
            self.form.leOutputFile.setText(foo)
