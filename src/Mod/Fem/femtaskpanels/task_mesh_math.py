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

## @package task_mesh_math
#  \ingroup FEM
#  \brief task panel for mesh refinement object

from PySide import QtCore, QtGui

import FreeCAD
import FreeCADGui

from femguiutils import selection_widgets
from . import base_femtaskpanel


class _TaskPanel(base_femtaskpanel._BaseTaskPanel):
    """
    The TaskPanel for editing References property of FemMeshMath objects
    """

    def __init__(self, obj):
        super().__init__(obj)

        self.parameter_widget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/MeshMath.ui"
        )
        self.parameter_widget.setWindowTitle("Math equation settings")
        self.parameter_widget.setWindowIcon(FreeCADGui.getIcon(":icons/FEM_MeshMath.svg"))
        self._init_parameter_widget()

        # form made from param and selection widget
        self.form = [self.parameter_widget]

        FreeCAD.addDocumentObserver(self)

    def _update_field_list(self):
        # update the available math fields

        self.parameter_widget.FieldList.clear()

        cnt = 1
        for obj in self.obj.Refinements:
            if not obj.Suppressed:
                self.parameter_widget.FieldList.addItem(f"F{cnt}: {obj.Label}")
                cnt += 1

    def _init_parameter_widget(self):

        ui = self.parameter_widget

        ui.Equation.setText(self.obj.Equation)
        ui.Equation.editingFinished.connect(self.equationChanged)

        info = FreeCADGui.getIcon("info.svg")
        ui.Icon.setPixmap(info.pixmap(QtCore.QSize(32,32)))
        ui.EqIcon.setPixmap(info.pixmap(QtCore.QSize(32,32)))

        self._update_field_list()

    def slotChangedObject(self, obj, prop):
        # callback of document observer for changed property
        if (obj == self.obj) and (prop == "Refinements"):
            self._update_field_list()

        # if some childrens suppress value is changed we also need to recreate the list
        if (obj in self.obj.Refinements) and (prop == "Suppressed"):
            self._update_field_list()

    def accept(self):
        FreeCAD.removeDocumentObserver(self)
        return super().accept()

    def reject(self):
        FreeCAD.removeDocumentObserver(self)
        return super().reject()

    @QtCore.Slot()
    def equationChanged(self):
        self.obj.Equation = self.parameter_widget.Equation.text()

