# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2018 sliptonic <shopinthewoods@gmail.com>               *
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
import PathScripts.PathLog as PathLog
import math

from PySide import QtGui

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
#PathLog.trackModule(PathLog.thisModule())

class ToolEditorDefault:
    '''Generic Tool parameter editor for all Tools that don't have a customized edit function.
    Let's the user enter the raw internal data. Not the best approach but this is the starting point.'''
    def __init__(self, editor):
        self.editor = editor
        self.form = editor.form

    def setupUI(self):
        self.form.paramImage.hide()
        self.form.paramGeneric.show()

    def updateUI(self):
        self.form.toolDiameter.setText(FreeCAD.Units.Quantity(self.editor.tool.Diameter, FreeCAD.Units.Length).UserString)
        self.form.toolFlatRadius.setText(FreeCAD.Units.Quantity(self.editor.tool.FlatRadius, FreeCAD.Units.Length).UserString)
        self.form.toolCornerRadius.setText(FreeCAD.Units.Quantity(self.editor.tool.CornerRadius, FreeCAD.Units.Length).UserString)
        self.form.toolCuttingEdgeHeight.setText(FreeCAD.Units.Quantity(self.editor.tool.CuttingEdgeHeight, FreeCAD.Units.Length).UserString)
        self.form.toolCuttingEdgeAngle.setText(FreeCAD.Units.Quantity(self.editor.tool.CuttingEdgeAngle, FreeCAD.Units.Angle).UserString)

    def updateTool(self):
        self.editor.tool.Diameter = FreeCAD.Units.parseQuantity(self.form.toolDiameter.text())
        self.editor.tool.FlatRadius = FreeCAD.Units.parseQuantity(self.form.toolFlatRadius.text())
        self.editor.tool.CornerRadius = FreeCAD.Units.parseQuantity(self.form.toolCornerRadius.text())
        self.editor.tool.CuttingEdgeAngle = FreeCAD.Units.Quantity(self.form.toolCuttingEdgeAngle.text())
        self.editor.tool.CuttingEdgeHeight = FreeCAD.Units.parseQuantity(self.form.toolCuttingEdgeHeight.text())

class ToolEditorImage(object):
    '''Base implementation for all customized Tool parameter editors.
    While not required it is simplest to subclass specific editors.'''
    def __init__(self, editor, imageFile, hide='', disable=''):
        self.editor = editor
        self.form = editor.form
        self.imagePath = "{}Mod/Path/Images/Tools/{}".format(FreeCAD.getHomePath(), imageFile)
        self.image = QtGui.QPixmap(self.imagePath)
        self.hide = hide
        self.disable = disable

        form = editor.form
        self.widgets = {
                'D' : (form.label_D, form.value_D),
                'd' : (form.label_d, form.value_d),
                'H' : (form.label_H, form.value_H),
                'a' : (form.label_a, form.value_a),
                'S' : (form.label_S, form.value_S)
                }

    def setupUI(self):
        PathLog.track()
        self.form.paramGeneric.hide()
        self.form.paramImage.show()

        for key, widgets in self.widgets.items():
            hide    = key in self.hide
            disable = key in self.disable
            for w in widgets:
                w.setHidden(hide)
                w.setDisabled(disable)
            if not hide and not disable:
                widgets[1].editingFinished.connect(self.editor.refresh)

        self.form.image.setPixmap(self.image)

    def updateUI(self):
        PathLog.track()
        self.form.value_D.setText(self.quantityDiameter(True).UserString)
        self.form.value_d.setText(self.quantityFlatRadius(True).UserString)
        self.form.value_a.setText(self.quantityCuttingEdgeAngle(True).UserString)
        self.form.value_H.setText(self.quantityCuttingEdgeHeight(True).UserString)

    def updateTool(self):
        PathLog.track()
        toolDefault = Path.Tool()
        if 'D' in self.hide:
            self.editor.tool.Diameter = toolDefault.Diameter
        else:
            self.editor.tool.Diameter = self.quantityDiameter(False)

        if 'd' in self.hide:
            self.editor.tool.FlatRadius = toolDefault.FlatRadius
        else:
            self.editor.tool.FlatRadius = self.quantityFlatRadius(False)

        if 'a' in self.hide:
            self.editor.tool.CuttingEdgeAngle = toolDefault.CuttingEdgeAngle
        else:
            self.editor.tool.CuttingEdgeAngle = self.quantityCuttingEdgeAngle(False)

        if 'H' in self.hide:
            self.editor.tool.CuttingEdgeHeight = toolDefault.CuttingEdgeHeight
        else:
            self.editor.tool.CuttingEdgeHeight = self.quantityCuttingEdgeHeight(False)

        self.editor.tool.CornerRadius = toolDefault.CornerRadius

    def quantityDiameter(self, propertyToDisplay):
        if propertyToDisplay:
            return FreeCAD.Units.Quantity(self.editor.tool.Diameter, FreeCAD.Units.Length)
        return FreeCAD.Units.parseQuantity(self.form.value_D.text())

    def quantityFlatRadius(self, propertyToDisplay):
        if propertyToDisplay:
            return FreeCAD.Units.Quantity(self.editor.tool.FlatRadius, FreeCAD.Units.Length) * 2
        return FreeCAD.Units.parseQuantity(self.form.value_d.text()) / 2

    def quantityCuttingEdgeAngle(self, propertyToDisplay):
        if propertyToDisplay:
            return FreeCAD.Units.Quantity(self.editor.tool.CuttingEdgeAngle, FreeCAD.Units.Angle)
        return FreeCAD.Units.parseQuantity(self.form.value_a.text())

    def quantityCuttingEdgeHeight(self, propertyToDisplay):
        if propertyToDisplay:
            return FreeCAD.Units.Quantity(self.editor.tool.CuttingEdgeHeight, FreeCAD.Units.Length)
        return FreeCAD.Units.parseQuantity(self.form.value_H.text())

class ToolEditorEndmill(ToolEditorImage):
    '''Tool parameter editor for endmills.'''
    def __init__(self, editor):
        super(self.__class__, self).__init__(editor, 'endmill.svg', 'da', 'S')

class ToolEditorDrill(ToolEditorImage):
    '''Tool parameter editor for drills.'''
    def __init__(self, editor):
        super(self.__class__, self).__init__(editor, 'drill.svg', 'dS', '')

    def quantityCuttingEdgeAngle(self, propertyToDisplay):
        if propertyToDisplay:
            return FreeCAD.Units.Quantity(self.editor.tool.CuttingEdgeAngle, FreeCAD.Units.Angle)
        return FreeCAD.Units.parseQuantity(self.form.value_a.text())

class ToolEditorEngrave(ToolEditorImage):
    '''Tool parameter editor for v-bits.'''
    def __init__(self, editor):
        super(self.__class__, self).__init__(editor, 'v-bit.svg', '', 'HS')

    def quantityCuttingEdgeHeight(self, propertyToDisplay):
        PathLog.track()
        dr = (self.quantityDiameter(False) - self.quantityFlatRadius(False)) / 2
        da = self.quantityCuttingEdgeAngle(False).Value
        return dr / math.tan(math.radians(da) / 2)

class ToolEditor:
    '''UI and controller for editing a Tool.
    The controller embeds the UI to the parentWidget which has to have a layout attached to it.
    The editor maintains two Tools, self.tool and self.Tool. The former is the one being edited
    and always reflects the current state. self.Tool on the other hand is the "official" Tool
    which should be used externally. The state is transferred between the two with accept and
    reject.

    The editor uses instances of ToolEditorDefault and ToolEditorImage to deal with the changes
    of the actual parameters. For any ToolType not mapped in ToolTypeImage the editor uses
    an instance of ToolEditorDefault.
    '''

    ToolTypeImage = {
            'EndMill':  ToolEditorEndmill,
            'Drill':    ToolEditorDrill,
            'Engraver': ToolEditorEngrave }

    def __init__(self, tool, parentWidget, parent=None):
        self.Parent = parent
        self.Tool = tool
        self.tool = tool.copy()
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/ToolEditor.ui")

        self.form.setParent(parentWidget)
        parentWidget.layout().addWidget(self.form)

        for tooltype in Path.Tool.getToolTypes(tool):
            self.form.toolType.addItem(tooltype)
        for material in Path.Tool.getToolMaterials(tool):
            self.form.toolMaterial.addItem(material)

        self.setupToolType(self.tool.ToolType)

    def accept(self):
        self.Tool = self.tool

    def reject(self):
        self.tool = self.Tool

    def getType(self, tooltype):
        "gets a combobox index number for a given type or viceversa"
        toolslist = Path.Tool.getToolTypes(Path.Tool())
        if isinstance(tooltype, str):
            if tooltype in toolslist:
                return toolslist.index(tooltype)
            else:
                return 0
        return toolslist[tooltype]

    def getMaterial(self, material):
        "gets a combobox index number for a given material or viceversa"
        matslist = Path.Tool.getToolMaterials(Path.Tool())
        if isinstance(material, str):
            if material in matslist:
                return matslist.index(material)
            else:
                return 0
        return matslist[material]

    def updateUI(self):
        PathLog.track()
        self.form.toolName.setText(self.tool.Name)
        self.form.toolType.setCurrentIndex(self.getType(self.tool.ToolType))
        self.form.toolMaterial.setCurrentIndex(self.getMaterial(self.tool.Material))
        self.form.toolLengthOffset.setText(FreeCAD.Units.Quantity(self.tool.LengthOffset, FreeCAD.Units.Length).UserString)

        self.editor.updateUI()

    def updateToolName(self):
        self.tool.Name = str(self.form.toolName.text())

    def updateToolType(self):
        PathLog.track()
        self.form.blockSignals(True)
        self.tool.ToolType = self.getType(self.form.toolType.currentIndex())
        self.setupToolType(self.tool.ToolType)
        self.updateUI()
        self.form.blockSignals(False)

    def setupToolType(self, tt):
        PathLog.track()
        if 'Undefined' == tt:
            tt = Path.Tool.getToolTypes(Path.Tool())[0]
        if tt in self.ToolTypeImage:
            self.editor = self.ToolTypeImage[tt](self)
        else:
            PathLog.debug("weak supported ToolType = %s" % (tt))
            self.editor = ToolEditorDefault(self)
        self.editor.setupUI()

    def updateTool(self):
        PathLog.track()
        self.tool.Material = self.getMaterial(self.form.toolMaterial.currentIndex())
        self.tool.LengthOffset = FreeCAD.Units.parseQuantity(self.form.toolLengthOffset.text())
        self.editor.updateTool()

    def refresh(self):
        PathLog.track()
        self.form.blockSignals(True)
        self.updateTool()
        self.updateUI()
        self.form.blockSignals(False)

    def setupUI(self):
        PathLog.track()
        self.updateUI()

        self.form.toolName.editingFinished.connect(self.updateToolName)
        self.form.toolType.currentIndexChanged.connect(self.updateToolType)
        self.form.toolMaterial.currentIndexChanged.connect(self.refresh)
        self.form.toolLengthOffset.valueChanged.connect(self.refresh)

