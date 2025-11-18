# ***************************************************************************
# *   Copyright (c) 2025 Stefan Tröger <stefantroeger@gmx.net>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

__title__ = "FreeCAD FEM mesh refinement task panel for the document object"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package task_mesh_region
#  \ingroup FEM
#  \brief task panel for mesh refinement object

from PySide import QtCore, QtGui

import FreeCAD
import FreeCADGui

from femguiutils import selection_widgets
from . import base_femtaskpanel


class _TaskPanel(base_femtaskpanel._BaseTaskPanel):
    """
    The TaskPanel for editing References property of FemMeshRegion objects
    """

    def __init__(self, obj):
        super().__init__(obj)


        self.parameter_widget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/MeshTransfiniteSurface.ui"
        )
        self.parameter_widget.setWindowTitle("Structured transfinite surface")
        self.parameter_widget.setWindowIcon(FreeCADGui.getIcon(":icons/FEM_MeshDistance.svg"))
        self._init_parameter_widget()

        self._init_parameter_widget()

        # geometry selection widget
        # only allow valid distance objects!
        self.selection_widget = selection_widgets.GeometryElementsSelection(
            obj.References, ["Face", "Vertex"], True, False
        )
        self.selection_widget.setWindowTitle("Reference geometries")
        self.selection_widget.setWindowIcon(FreeCADGui.getIcon(":icons/FEM_MeshDistance.svg"))

        # form made from param and selection widget
        self.form = [self.parameter_widget, self.selection_widget]

    def _init_parameter_widget(self):

        ui = self.parameter_widget

        # There is no known way to access the colors set by stylesheets. It is hence not posssible to make a universal
        # correct desicion on which image to use. Workaround is to check stylesheet name if one ist set for "dark" and "ligth"
        stylesheet = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/MainWindow").GetString("StyleSheet")
        if "dark" in stylesheet.lower():
            lightness = "Light"
        elif "light" in stylesheet.lower():
            lightness = "Dark"
        else:
            # use the qt style background and text color to detect the image to use
            palette = ui.palette()
            if palette.color(QtGui.QPalette.Text).lightness() > palette.color(QtGui.QPalette.Window).lightness():
                lightness = "Light"
            else:
                lightness = "Dark"

        ui.Diagram.setPixmap(QtGui.QPixmap(f":images/FEM_MeshTransfiniteSurface{lightness}.svg"))

        ui.Recombine.setChecked(self.obj.Recombine)
        ui.Recombine.toggled.connect(self.recombineChanged)

        ui.Orientation.addItems(self.obj.getEnumerationsOfProperty("TriangleOrientation"))
        ui.Orientation.setCurrentIndex(ui.Orientation.findData(self.obj.TriangleOrientation))
        ui.Orientation.currentIndexChanged.connect(self.orientationChanged)
        ui.Orientation.setDisabled(self.obj.Recombine)

        info = FreeCADGui.getIcon("info.svg")
        ui.Icon.setPixmap(info.pixmap(QtCore.QSize(32,32)))

        # automation
        ui.Automation.setChecked(self.obj.UseAutomation)
        ui.Automation.toggled.connect(self.automationChanged)

        ui.Nodes.setValue(self.obj.Nodes)
        FreeCADGui.ExpressionBinding(ui.Nodes).bind(self.obj, "Nodes")
        ui.Nodes.valueChanged.connect(self.nodesChanged)

        ui.Coefficient.setValue(self.obj.Coefficient)
        FreeCADGui.ExpressionBinding(ui.Coefficient).bind(self.obj, "Coefficient")
        ui.Coefficient.valueChanged.connect(self.coefficientChanged)

        match self.obj.Distribution:
            case "Constant":
                ui.Distribution.setCurrentIndex(0)
            case "Bump":
                ui.Distribution.setCurrentIndex(1+int(self.obj.Invert))
            case "Progression":
                ui.Distribution.setCurrentIndex(3+int(self.obj.Invert))

        ui.Distribution.currentIndexChanged.connect(self.distributionChanged)

    def accept(self):
        self.obj.References = self.selection_widget.references
        self.selection_widget.finish_selection()
        return super().accept()

    def reject(self):
        self.selection_widget.finish_selection()
        return super().reject()

    @QtCore.Slot(bool)
    def recombineChanged(self, value):
        self.obj.Recombine = value

    @QtCore.Slot(int)
    def orientationChanged(self, value):
        self.obj.TriangleOrientation = value

    @QtCore.Slot(bool)
    def automationChanged(self, value):
        self.obj.UseAutomation = value

    @QtCore.Slot(int)
    def nodesChanged(self, value):
        self.obj.Nodes = int(value)

    @QtCore.Slot(int)
    def distributionChanged(self, value):

        match value:
            case 0:
                self.obj.Distribution = "Constant"
            case 1:
                self.obj.Distribution = "Bump"
                self.obj.Invert = False
            case 2:
                self.obj.Distribution = "Bump"
                self.obj.Invert = True
            case 3:
                self.obj.Distribution = "Progression"
                self.obj.Invert = False
            case 4:
                self.obj.Distribution = "Progression"
                self.obj.Invert = True

    @QtCore.Slot(float)
    def coefficientChanged(self, value):
        self.obj.Coefficient = float(value)
