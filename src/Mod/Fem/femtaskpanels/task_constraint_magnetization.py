# ***************************************************************************
# *   Copyright (c) 2023 Uwe Stöhr <uwestoehr@lyx.org>                      *
# *   Copyright (c) 2025 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
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

__title__ = "FreeCAD FEM constraint magnetization task panel for the document object"
__author__ = "Uwe Stöhr, Mario Passaglia"
__url__ = "https://www.freecad.org"

## @package task_constraint_magnetization
#  \ingroup FEM
#  \brief task panel for constraint magnetization object

from PySide import QtCore

import FreeCAD
import FreeCADGui

from femguiutils import selection_widgets

from femtools import membertools
from . import base_femtaskpanel


class _TaskPanel(base_femtaskpanel._BaseTaskPanel):

    def __init__(self, obj):
        super().__init__(obj)

        self.parameter_widget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/Magnetization.ui"
        )

        # geometry selection widget
        # magnetization is always a body force for 3D, therefore only allow solid
        self._selection_widget = selection_widgets.GeometryElementsSelection(
            obj.References, ["Solid", "Face"], True, False
        )

        # form made from param and selection widget
        self.form = [self.parameter_widget, self._selection_widget]

        analysis = obj.getParentGroup()
        self._mesh = None
        self._part = None
        if analysis is not None:
            self._mesh = membertools.get_single_member(analysis, "Fem::FemMeshObject")
        if self._mesh is not None:
            self._part = self._mesh.Shape
        self._partVisible = None
        self._meshVisible = None

        QtCore.QObject.connect(
            self.parameter_widget.ckb_magnetization_1,
            QtCore.SIGNAL("toggled(bool)"),
            self.magnetization_1_enabled_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_magnetization_re_1,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.magnetization_re_1_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.ckb_magnetization_2,
            QtCore.SIGNAL("toggled(bool)"),
            self.magnetization_2_enabled_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_magnetization_re_2,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.magnetization_re_2_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.ckb_magnetization_3,
            QtCore.SIGNAL("toggled(bool)"),
            self.magnetization_3_enabled_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_magnetization_re_3,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.magnetization_re_3_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_magnetization_im_1,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.magnetization_im_1_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_magnetization_im_2,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.magnetization_im_2_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_magnetization_im_3,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.magnetization_im_3_changed,
        )

        self.init_parameter_widget()

    def open(self):
        if self._mesh is not None and self._part is not None:
            self._meshVisible = self._mesh.ViewObject.isVisible()
            self._partVisible = self._part.ViewObject.isVisible()
            self._mesh.ViewObject.hide()
            self._part.ViewObject.show()

    def reject(self):
        self._restoreVisibility()
        self._selection_widget.finish_selection()
        return super().reject()

    def accept(self):
        if self.obj.References != self._selection_widget.references:
            self.obj.References = self._selection_widget.references
        self._set_params()
        self._selection_widget.finish_selection()
        self._restoreVisibility()
        return super().accept()

    def _restoreVisibility(self):
        if self._mesh is not None and self._part is not None:
            if self._meshVisible:
                self._mesh.ViewObject.show()
            else:
                self._mesh.ViewObject.hide()
            if self._partVisible:
                self._part.ViewObject.show()
            else:
                self._part.ViewObject.hide()

    def _get_params(self):
        self.magnetization_re_1 = self.obj.Magnetization_re_1
        self.magnetization_re_2 = self.obj.Magnetization_re_2
        self.magnetization_re_3 = self.obj.Magnetization_re_3
        self.magnetization_im_1 = self.obj.Magnetization_im_1
        self.magnetization_im_2 = self.obj.Magnetization_im_2
        self.magnetization_im_3 = self.obj.Magnetization_im_3

        self.magnetization_1_enabled = self.obj.EnableMagnetization_1
        self.magnetization_2_enabled = self.obj.EnableMagnetization_2
        self.magnetization_3_enabled = self.obj.EnableMagnetization_3

    def _set_params(self):
        self.obj.Magnetization_re_1 = self.magnetization_re_1
        self.obj.Magnetization_re_2 = self.magnetization_re_2
        self.obj.Magnetization_re_3 = self.magnetization_re_3
        self.obj.Magnetization_im_1 = self.magnetization_im_1
        self.obj.Magnetization_im_2 = self.magnetization_im_2
        self.obj.Magnetization_im_3 = self.magnetization_im_3

        self.obj.EnableMagnetization_1 = self.magnetization_1_enabled
        self.obj.EnableMagnetization_2 = self.magnetization_2_enabled
        self.obj.EnableMagnetization_3 = self.magnetization_3_enabled

    def init_parameter_widget(self):
        self._get_params()

        self.parameter_widget.qsb_magnetization_re_1.setProperty("value", self.magnetization_re_1)
        self.parameter_widget.qsb_magnetization_re_1.setEnabled(self.magnetization_1_enabled)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_magnetization_re_1).bind(
            self.obj, "Magnetization_re_1"
        )

        self.parameter_widget.qsb_magnetization_re_2.setProperty("value", self.magnetization_re_2)
        self.parameter_widget.qsb_magnetization_re_2.setEnabled(self.magnetization_2_enabled)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_magnetization_re_2).bind(
            self.obj, "Magnetization_re_2"
        )

        self.parameter_widget.qsb_magnetization_re_3.setProperty("value", self.magnetization_re_3)
        self.parameter_widget.qsb_magnetization_re_3.setEnabled(self.magnetization_3_enabled)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_magnetization_re_3).bind(
            self.obj, "Magnetization_re_3"
        )

        self.parameter_widget.qsb_magnetization_im_1.setProperty("value", self.magnetization_im_1)
        self.parameter_widget.qsb_magnetization_im_1.setEnabled(self.magnetization_1_enabled)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_magnetization_im_1).bind(
            self.obj, "Magnetization_im_1"
        )

        self.parameter_widget.qsb_magnetization_im_2.setProperty("value", self.magnetization_im_2)
        self.parameter_widget.qsb_magnetization_im_2.setEnabled(self.magnetization_2_enabled)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_magnetization_im_2).bind(
            self.obj, "Magnetization_im_2"
        )

        self.parameter_widget.qsb_magnetization_im_3.setProperty("value", self.magnetization_im_3)
        self.parameter_widget.qsb_magnetization_im_3.setEnabled(self.magnetization_3_enabled)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_magnetization_im_3).bind(
            self.obj, "Magnetization_im_3"
        )

        self.parameter_widget.ckb_magnetization_1.setChecked(self.magnetization_1_enabled)
        self.parameter_widget.ckb_magnetization_2.setChecked(self.magnetization_2_enabled)
        self.parameter_widget.ckb_magnetization_3.setChecked(self.magnetization_3_enabled)

    def magnetization_1_enabled_changed(self, value):
        self.magnetization_1_enabled = value
        self.parameter_widget.qsb_magnetization_re_1.setEnabled(value)
        self.parameter_widget.qsb_magnetization_im_1.setEnabled(value)

    def magnetization_2_enabled_changed(self, value):
        self.magnetization_2_enabled = value
        self.parameter_widget.qsb_magnetization_re_2.setEnabled(value)
        self.parameter_widget.qsb_magnetization_im_2.setEnabled(value)

    def magnetization_3_enabled_changed(self, value):
        self.magnetization_3_enabled = value
        self.parameter_widget.qsb_magnetization_re_3.setEnabled(value)
        self.parameter_widget.qsb_magnetization_im_3.setEnabled(value)

    def magnetization_re_1_changed(self, value):
        self.magnetization_re_1 = value

    def magnetization_re_2_changed(self, value):
        self.magnetization_re_2 = value

    def magnetization_re_3_changed(self, value):
        self.magnetization_re_3 = value

    def magnetization_im_1_changed(self, value):
        self.magnetization_im_1 = value

    def magnetization_im_2_changed(self, value):
        self.magnetization_im_2 = value

    def magnetization_im_3_changed(self, value):
        self.magnetization_im_3 = value
