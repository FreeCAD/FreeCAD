# ***************************************************************************
# *   Copyright (c) 2016 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM mesh group task panel for the document object"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package task_mesh_group
#  \ingroup FEM
#  \brief task panel for mesh group object

from PySide import QtCore

import FreeCAD
import FreeCADGui

from femguiutils import selection_widgets
from . import base_femtaskpanel


class _TaskPanel(base_femtaskpanel._BaseTaskPanel):
    """
    The TaskPanel for editing References property of MeshGroup objects
    """

    def __init__(self, obj):
        super().__init__(obj)

        # parameter widget
        self.parameterWidget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/MeshGroup.ui"
        )
        QtCore.QObject.connect(
            self.parameterWidget.rb_name,
            QtCore.SIGNAL("toggled(bool)"),
            self.choose_exportidentifier_name,
        )
        QtCore.QObject.connect(
            self.parameterWidget.rb_label,
            QtCore.SIGNAL("toggled(bool)"),
            self.choose_exportidentifier_label,
        )
        self.init_parameter_widget()

        # geometry selection widget
        # start with Solid in list!
        # only one shape type is allowed
        self.selectionWidget = selection_widgets.GeometryElementsSelection(
            obj.References, ["Solid", "Face", "Edge", "Vertex"], False, False
        )

        # form made from param and selection widget
        self.form = [self.parameterWidget, self.selectionWidget]

    def accept(self):
        self.obj.UseLabel = self.use_label
        self.obj.References = self.selectionWidget.references
        self.selectionWidget.finish_selection()
        return super().accept()

    def reject(self):
        self.selectionWidget.finish_selection()
        return super().reject()

    def init_parameter_widget(self):
        self.use_label = self.obj.UseLabel
        self.parameterWidget.rb_name.setChecked(not self.use_label)
        self.parameterWidget.rb_label.setChecked(self.use_label)

    def choose_exportidentifier_name(self, state):
        self.use_label = not state

    def choose_exportidentifier_label(self, state):
        self.use_label = state
