# SPDX-License-Identifier: LGPL-2.1-or-later

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
import Path.Op.Custom as PathCustom
import Path.Op.Gui.Base as PathOpGui
from Path.Main.Gui.Editor import CodeEditor

from PySide import QtGui
from PySide.QtCore import QT_TRANSLATE_NOOP

import os

__title__ = "CAM Custom Operation UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Custom operation page controller and command implementation."

translate = FreeCAD.Qt.translate


class TaskPanelOpPage(PathOpGui.TaskPanelPage):
    """Page controller class for the Custom operation."""

    def getForm(self):
        """getForm() ... returns UI"""
        form = FreeCADGui.PySideUic.loadUi(":/panels/PageOpCustomEdit.ui")

        comboToPropertyMap = [("source", "Source")]
        enumTups = PathCustom.ObjectCustom.propertyEnumerations(dataType="raw")

        self.populateCombobox(form, enumTups, comboToPropertyMap)

        # add editor with lines enumeration
        self.editor = CodeEditor()
        toolTip = (
            "Form to enter G-code"
            "\n\nTo add an expression, surround string with characters '{{expression}}'"
            "\nExample:"
            "\nG0 Z{{VarSet.HeightZ.Value+5}}"
            "\nG0 X{{Profile.Path.Commands[3].x}} Y{{Profile.Path.Commands[3].y}}"
        )
        self.editor.setToolTip(toolTip)
        form.txtGCodeBox.layout().removeWidget(form.txtGCode)
        form.txtGCode.deleteLater()
        form.txtGCodeBox.layout().addWidget(self.editor)

        return form

    def getFields(self, obj):
        """getFields(obj) ... transfers values from UI to obj's properties"""
        if obj.Source != str(self.form.source.currentData()):
            obj.Source = str(self.form.source.currentData())
        if obj.GcodeFile != str(self.form.fileName.text()):
            obj.GcodeFile = str(self.form.fileName.text())
        if obj.Gcode != str(self.editor.toPlainText().split("\n")):
            obj.Gcode = self.editor.toPlainText().split("\n")

    def setFields(self, obj):
        """setFields(obj) ... transfers obj's property values to UI"""
        self.selectInComboBox(obj.Source, self.form.source)
        self.form.fileName.setText(obj.GcodeFile)
        self.editor.setText("\n".join(obj.Gcode))

        self.updateVisibility()

    def getSignalsForUpdate(self, obj):
        """getSignalsForUpdate(obj) ... return list of signals for updating obj"""
        signals = []
        signals.append(self.form.source.currentIndexChanged)
        signals.append(self.form.fileName.editingFinished)
        signals.append(self.editor.textChanged)

        return signals

    def updateVisibility(self):
        source = self.obj.getEnumerationsOfProperty("Source")[self.form.source.currentIndex()]
        if source == "File":
            self.form.fileNameBox.show()
            self.form.verticalSpacerBox.show()
            self.form.txtGCodeBox.hide()
        else:
            self.form.txtGCodeBox.show()
            self.form.fileNameBox.hide()
            self.form.verticalSpacerBox.hide()

    def registerSignalHandlers(self, obj):
        self.form.source.currentIndexChanged.connect(self.updateVisibility)
        self.form.setFileName.clicked.connect(self.setFileName)

    def setFileName(self):
        dirname = os.path.dirname(self.obj.GcodeFile)
        if not dirname:
            dirname = os.path.dirname(FreeCAD.activeDocument().FileName)
        filter1 = "All Files (*)"
        filter2 = "Text files (*.cnc *.g *.gc *.gco *.gcode *.nc *.ncc *.ngc *.tap *.txt)"
        filters = translate("CAM_Custom", ";;".join((filter1, filter2)))
        selected_filter = translate("CAM_Custom", filter2)
        filename = QtGui.QFileDialog.getOpenFileName(
            self.form,
            translate("CAM_Custom", "Select file containing the gcode"),
            dirname,
            filters,
            selected_filter,
        )
        if filename and filename[0]:
            self.obj.GcodeFile = str(filename[0])
            self.setFields(self.obj)


Command = PathOpGui.SetupOperation(
    "Custom",
    PathCustom.Create,
    TaskPanelOpPage,
    "CAM_Custom",
    QT_TRANSLATE_NOOP("CAM_Custom", "Custom"),
    QT_TRANSLATE_NOOP("CAM_Custom", "Create custom G-code snippet"),
    PathCustom.SetupProperties,
)

FreeCAD.Console.PrintLog("Loading PathCustomGui... done\n")
