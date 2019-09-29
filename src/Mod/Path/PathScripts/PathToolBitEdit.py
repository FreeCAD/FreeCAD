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
import Path
import PathScripts.PathGui as PathGui
import PathScripts.PathLog as PathLog
import PathScripts.PathToolBit as PathToolBit
import copy
import math
import re

from PySide import QtGui

PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
PathLog.trackModule(PathLog.thisModule())

LastPath = 'src/Mod/Path/Tools/Template'

class ToolBitEditor:
    '''UI and controller for editing a ToolBit.
    The controller embeds the UI to the parentWidget which has to have a layout attached to it.
    '''

    def __init__(self, tool, parentWidget=None):
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/ToolBitEditor.ui")

        if parentWidget:
            self.form.setParent(parentWidget)
            parentWidget.layout().addWidget(self.form)

        self.tool = tool
        if not tool.BitTemplate:
            self.tool.BitTemplate = 'src/Mod/Path/Tools/Template/endmill-straight.fcstd'
        self.tool.Proxy.loadBitBody(self.tool)
        self.setupTool(self.tool)

    def setupTool(self, tool):
        layout = self.form.bitParams.layout()
        for i in range(layout.rowCount() - 1, -1, -1):
            layout.removeRow(i)
        editor = {}
        ui = FreeCADGui.UiLoader()
        for name in tool.PropertiesList:
            if tool.getGroupOfProperty(name) == PathToolBit.PropertyGroupBit:
                qsb = ui.createWidget('Gui::QuantitySpinBox')
                editor[name] = PathGui.QuantitySpinBox(qsb, tool, name)
                label = QtGui.QLabel(re.sub('([A-Z][a-z]+)', r' \1', re.sub('([A-Z]+)', r' \1', name)))
                #if parameter.get('Desc'):
                #    qsb.setToolTip(parameter['Desc'])
                layout.addRow(label, qsb)
        self.bitEditor = editor
        img = tool.Proxy.getBitThumbnail(tool)
        if img:
            self.form.image.setPixmap(QtGui.QPixmap(QtGui.QImage.fromData(img)))
        else:
            self.form.image.setPixmap(QtGui.QPixmap())

    def accept(self):
        self.refresh()
        self.tool.Proxy.unloadBitBody(self.tool)

    def reject(self):
        self.tool.Proxy.unloadBitBody(self.tool)
        pass

    def updateUI(self):
        PathLog.track()
        self.form.toolName.setText(self.tool.Label)
        self.form.templatePath.setText(self.tool.BitTemplate)

        for editor in self.bitEditor:
            self.bitEditor[editor].updateSpinBox()

    def updateTemplate(self):
        self.tool.BitTemplate = str(self.form.templatePath.text())
        self.setupTool(self.tool)
        self.form.toolName.setText(self.tool.Label)

        for editor in self.bitEditor:
            self.bitEditor[editor].updateSpinBox()

    def updateTool(self):
        PathLog.track()
        self.tool.Label = str(self.form.toolName.text())
        self.tool.BitTemplate = str(self.form.templatePath.text())

        for editor in self.bitEditor:
            self.bitEditor[editor].updateProperty()

        self.tool.Proxy._updateBitShape(self.tool)

    def refresh(self):
        PathLog.track()
        self.form.blockSignals(True)
        self.updateTool()
        self.updateUI()
        self.form.blockSignals(False)

    def selectTemplate(self):
        global LastPath
        path = self.tool.BitTemplate
        if not path:
            path = LastPath
        foo = QtGui.QFileDialog.getOpenFileName(QtGui.QApplication.activeWindow(),
                "Path - Tool Template",
                path,
                "*.fcstd")[0]
        if foo:
            self.form.templatePath.setText(foo)
            self.updateTemplate()

    def setupUI(self):
        PathLog.track()
        self.updateUI()

        self.form.toolName.editingFinished.connect(self.refresh)
        self.form.templatePath.editingFinished.connect(self.updateTemplate)
        self.form.templateSet.clicked.connect(self.selectTemplate)
