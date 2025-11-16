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

## @package task_mesh_restrict
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
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/MeshRestrict.ui"
        )
        self.parameter_widget.setWindowTitle("Restriction settings")
        self.parameter_widget.setWindowIcon(FreeCADGui.getIcon(":icons/FEM_MeshRestrict.svg"))
        self._init_parameter_widget()

        # geometry selection widget
        # only allow valid distance objects!
        self.selection_widget = selection_widgets.GeometryElementsSelection(
            obj.References, ["Face", "Edge", "Vertex", "Solid"], True, False
        )
        self.selection_widget.setWindowTitle("Reference geometries")
        self.selection_widget.setWindowIcon(FreeCADGui.getIcon(":icons/FEM_MeshRestrict.svg"))

        # form made from param and selection widget
        self.form = [self.parameter_widget, self.selection_widget]

    def _init_parameter_widget(self):

        ui = self.parameter_widget

        ui.Boundary.setChecked(self.obj.IncludeBoundary)
        ui.Boundary.toggled.connect(self.boundaryChanged)

        # option seems not to be supported
        #ui.Embedded.setChecked(self.obj.IncludeEmbedded)
        #ui.Embedded.toggled.connect(self.embeddedChanged)

        info = FreeCADGui.getIcon("info.svg")
        ui.Icon.setPixmap(info.pixmap(QtCore.QSize(32,32)))


    def accept(self):
        self.obj.References = self.selection_widget.references
        self.selection_widget.finish_selection()
        return super().accept()

    def reject(self):
        self.selection_widget.finish_selection()
        return super().reject()

    @QtCore.Slot(bool)
    def boundaryChanged(self, value):
        self.obj.IncludeBoundary = value

    #@QtCore.Slot(bool)
    #def embeddedChanged(self, value):
    #    self.obj.IncludeEmbedded = value



