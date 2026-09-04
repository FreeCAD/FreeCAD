# ***************************************************************************
# *   Copyright (c) 2024 Tim Swait <timswait@gmail.com>                     *
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

__title__ = "FreeCAD FEM element geometry laminate task panel for the document object"
__author__ = "Tim Swait"
__url__ = "https://www.freecad.org"

## @package task_element_geometry_laminate
#  \ingroup FEM
#  \brief task panel for element geometry laminate object

from PySide import QtCore

import FreeCAD
import FreeCADGui

from femguiutils import selection_widgets
from . import base_femtaskpanel


class _TaskPanel(base_femtaskpanel._BaseTaskPanel):
    """
    The TaskPanel for editing References property of ElementGeometryLaminate objects
    """

    def __init__(self, obj):
        super().__init__(obj)

        # parameter widget
        self.parameter_widget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/ElementGeometryLaminate.ui"
        )
        # QtCore.QObject.connect(
        #    self.parameter_widget.qsb_thicknesses,
        #    QtCore.SIGNAL("Base::Quantity"),
        #    self.thicknesses_changed, # THIS ISN'T WORKING!
        # )
        # self.init_parameter_widget() # THIS ISN'T WORKING!

        # geometry selection widget
        self.selection_widget = selection_widgets.GeometryElementsSelection(
            obj.References, ["Face"], False, True
        )

        # form made from param and selection widget
        self.form = [self.parameter_widget, self.selection_widget]

    def accept(self):
        # self.obj.Thicknesses = self.thicknesses
        self.obj.References = self.selection_widget.references
        self.selection_widget.finish_selection()
        return super().accept()

    def reject(self):
        self.selection_widget.finish_selection()
        return super().reject()

    def init_parameter_widget(self):
        self.thicknesses = self.obj.Thicknesses
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_thicknesses).bind(
            self.obj, "Thicknesses"
        )
        self.parameter_widget.qsb_thicknesses.setProperty("value", self.thicknesses)

    def thicknesses_changed(self, value):
        self.thicknesses = value
