# ***************************************************************************
# *   Copyright (c) 2019 Bernd Hahnebach <bernd@bimstatik.org>              *
# *   Copyright (c) 2024 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
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

__title__ = "FreeCAD FEM material reinforced task panel for the document object"
__author__ = "Bernd Hahnebach, Mario Passaglia"
__url__ = "https://www.freecad.org"

## @package task_material_reinforced
#  \ingroup FEM
#  \brief task panel for reinforced material object

from PySide import QtCore
from PySide import QtGui

import FreeCAD
import FreeCADGui
import Materials
import MatGui

from . import base_femtaskpanel


class _TaskPanel(base_femtaskpanel._BaseTaskPanel):
    """
    The editmode TaskPanel for MaterialReinforced objects
    """

    def __init__(self, obj):
        super().__init__(obj)

        self.material = self.obj.Material
        self.uuid = self.obj.UUID
        self.reinf = self.obj.Reinforcement
        self.reinf_uuid = self.obj.ReinforcementUUID
        self.material_manager = Materials.MaterialManager()

        # parameter widget
        self.parameter_widget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/MaterialReinforcement.ui"
        )

        self.material_tree = MatGui.MaterialTreeWidget(self.parameter_widget.wgt_material_tree)
        self.material_tree.expanded = False
        self.material_tree.IncludeEmptyFolders = False
        self.material_tree.IncludeEmptyLibraries = False

        self.reinf_tree = MatGui.MaterialTreeWidget(self.parameter_widget.wgt_reinf_tree)
        self.reinf_tree.expanded = False
        self.reinf_tree.IncludeEmptyFolders = False
        self.reinf_tree.IncludeEmptyLibraries = False

        self.form = self.parameter_widget

        QtCore.QObject.connect(
            self.parameter_widget.wgt_material_tree,
            QtCore.SIGNAL("onMaterial(QString)"),
            self.set_material,
        )

        QtCore.QObject.connect(
            self.parameter_widget.wgt_reinf_tree,
            QtCore.SIGNAL("onMaterial(QString)"),
            self.set_reinforcement,
        )

        self.material_tree.UUID = self.uuid
        self.reinf_tree.UUID = self.reinf_uuid

    def accept(self):
        self.obj.Material = self.material
        self.obj.UUID = self.uuid
        self.obj.Reinforcement = self.reinf
        self.obj.ReinforcementUUID = self.reinf_uuid

        return super().accept()

    def set_material(self, value):
        if not value:
            return
        mat = self.material_manager.getMaterial(value)
        self.material = mat.Properties
        self.uuid = mat.UUID
        self.parameter_widget.lbl_material_descr.setText(self.material["Description"])

    def set_reinforcement(self, value):
        if not value:
            return
        mat = self.material_manager.getMaterial(value)
        self.reinf = mat.Properties
        self.reinf_uuid = mat.UUID
        self.parameter_widget.lbl_reinf_descr.setText(self.reinf["Description"])
