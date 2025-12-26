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
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package task_mesh_distance
#  \ingroup FEM
#  \brief task panel for mesh refinement object

from PySide import QtCore, QtGui

import FreeCAD
import FreeCADGui

from femguiutils import selection_widgets
from . import base_femtaskpanel
from . import base_fempreviewpanel


class _TaskPanel(base_femtaskpanel._BaseTaskPanel, base_fempreviewpanel._TaskPanel):
    """
    The TaskPanel for editing References property of FemMeshRegion objects
    """

    def __init__(self, obj):

        base_femtaskpanel._BaseTaskPanel.__init__(self, obj)
        base_fempreviewpanel._TaskPanel.__init__(self, obj)

        self.parameter_widget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/MeshDistance.ui"
        )
        self.parameter_widget.setWindowTitle("Distance threshold settings")
        self.parameter_widget.setWindowIcon(FreeCADGui.getIcon(":icons/FEM_MeshDistance.svg"))
        self._init_parameter_widget()

        # geometry selection widget
        # only allow valid distance objects!
        self.selection_widget = selection_widgets.GeometryElementsSelection(
            obj.References, ["Face", "Edge", "Vertex"], True, False
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

        ui.Diagram.setPixmap(QtGui.QPixmap(f":images/FEM_MeshDistanceThreshold{lightness}.svg"))

        ui.SizeMin.setProperty("value", self.obj.SizeMinimum)
        FreeCADGui.ExpressionBinding(ui.SizeMin).bind(self.obj, "SizeMinimum")
        ui.SizeMin.valueChanged.connect(self.sizeMinChanged)

        ui.SizeMax.setProperty("value", self.obj.SizeMaximum)
        FreeCADGui.ExpressionBinding(ui.SizeMax).bind(self.obj, "SizeMaximum")
        ui.SizeMax.valueChanged.connect(self.sizeMaxChanged)

        ui.DistMin.setProperty("value", self.obj.DistanceMinimum)
        FreeCADGui.ExpressionBinding(ui.DistMin).bind(self.obj, "DistanceMinimum")
        ui.DistMin.valueChanged.connect(self.distMinChanged)


        ui.DistMax.setProperty("value", self.obj.DistanceMaximum)
        FreeCADGui.ExpressionBinding(ui.DistMax).bind(self.obj, "DistanceMaximum")
        ui.DistMax.valueChanged.connect(self.distMaxChanged)

        ui.Sampling.setProperty("value", FreeCAD.Units.Quantity(self.obj.Sampling))
        FreeCADGui.ExpressionBinding(ui.Sampling).bind(self.obj, "Sampling")
        ui.Sampling.valueChanged.connect(self.samplingChanged)

        ui.Linear.setChecked(self.obj.LinearInterpolation)
        ui.Linear.toggled.connect(self.linearChanged)

        ui.Visualize.toggled.connect(self.visualize)

    def accept(self):
        self.obj.References = self.selection_widget.references
        self.selection_widget.finish_selection()
        self.stop_preview()
        return super().accept()

    def reject(self):
        self.selection_widget.finish_selection()
        self.stop_preview()
        return super().reject()

    @QtCore.Slot(FreeCAD.Units.Quantity)
    def distMaxChanged(self, value):
        self.obj.DistanceMaximum = value
        self.update_preview()

    @QtCore.Slot(FreeCAD.Units.Quantity)
    def distMinChanged(self, value):
        self.obj.DistanceMinimum = value
        self.update_preview()

    @QtCore.Slot(FreeCAD.Units.Quantity)
    def sizeMaxChanged(self, value):
        self.obj.SizeMaximum = value
        self.update_preview()

    @QtCore.Slot(FreeCAD.Units.Quantity)
    def sizeMinChanged(self, value):
        self.obj.SizeMinimum = value
        self.update_preview()

    @QtCore.Slot(FreeCAD.Units.Quantity)
    def samplingChanged(self, value):
        self.obj.Sampling = int(value)
        self.update_preview()

    @QtCore.Slot(bool)
    def linearChanged(self, value):
        self.obj.LinearInterpolation = value
        self.update_preview()

