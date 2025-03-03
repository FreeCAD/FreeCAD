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

__title__ = "FreeCAD FEM constraint current density task panel for the document object"
__author__ = "Uwe Stöhr, Mario Passaglia"
__url__ = "https://www.freecad.org"

## @package task_constraint_currentdensity
#  \ingroup FEM
#  \brief task panel for constraint current density object

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
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/CurrentDensity.ui"
        )

        # geometry selection widget
        # start with Solid in list!
        self._selectionWidget = selection_widgets.GeometryElementsSelection(
            obj.References, ["Solid", "Face"], True, False
        )

        # form made from param and selection widget
        self.form = [self.parameter_widget, self._selectionWidget]

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
            self.parameter_widget.cb_mode,
            QtCore.SIGNAL("currentIndexChanged(int)"),
            self.mode_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.ckb_current_density_1,
            QtCore.SIGNAL("toggled(bool)"),
            self.current_density_1_enabled_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_current_density_re_1,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.current_density_re_1_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.ckb_current_density_2,
            QtCore.SIGNAL("toggled(bool)"),
            self.current_density_2_enabled_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_current_density_re_2,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.current_density_re_2_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.ckb_current_density_3,
            QtCore.SIGNAL("toggled(bool)"),
            self.current_density_3_enabled_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_current_density_re_3,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.current_density_re_3_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_current_density_im_1,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.current_density_im_1_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_current_density_im_2,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.current_density_im_2_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_current_density_im_3,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.current_density_im_3_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_normal_current_density_re,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.normal_current_density_re_changed,
        )
        QtCore.QObject.connect(
            self.parameter_widget.qsb_normal_current_density_im,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.normal_current_density_im_changed,
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
        self._selectionWidget.finish_selection()
        return super().reject()

    def accept(self):
        if self.obj.References != self._selectionWidget.references:
            self.obj.References = self._selectionWidget.references
        self._set_params()
        self._selectionWidget.finish_selection()
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
        self.mode = self.obj.Mode

        self.current_density_re_1 = self.obj.CurrentDensity_re_1
        self.current_density_re_2 = self.obj.CurrentDensity_re_2
        self.current_density_re_3 = self.obj.CurrentDensity_re_3
        self.current_density_im_1 = self.obj.CurrentDensity_im_1
        self.current_density_im_2 = self.obj.CurrentDensity_im_2
        self.current_density_im_3 = self.obj.CurrentDensity_im_3

        self.current_density_1_enabled = self.obj.EnableCurrentDensity_1
        self.current_density_2_enabled = self.obj.EnableCurrentDensity_2
        self.current_density_3_enabled = self.obj.EnableCurrentDensity_3

        self.normal_current_density_re = self.obj.NormalCurrentDensity_re
        self.normal_current_density_im = self.obj.NormalCurrentDensity_im

    def _set_params(self):
        self.obj.Mode = self.mode

        self.obj.CurrentDensity_re_1 = self.current_density_re_1
        self.obj.CurrentDensity_re_2 = self.current_density_re_2
        self.obj.CurrentDensity_re_3 = self.current_density_re_3
        self.obj.CurrentDensity_im_1 = self.current_density_im_1
        self.obj.CurrentDensity_im_2 = self.current_density_im_2
        self.obj.CurrentDensity_im_3 = self.current_density_im_3

        self.obj.EnableCurrentDensity_1 = self.current_density_1_enabled
        self.obj.EnableCurrentDensity_2 = self.current_density_2_enabled
        self.obj.EnableCurrentDensity_3 = self.current_density_3_enabled

        self.obj.NormalCurrentDensity_re = self.normal_current_density_re
        self.obj.NormalCurrentDensity_im = self.normal_current_density_im

    def init_parameter_widget(self):
        self._get_params()

        # custom current density
        self.parameter_widget.qsb_current_density_re_1.setProperty(
            "value", self.current_density_re_1
        )
        self.parameter_widget.qsb_current_density_re_1.setEnabled(self.current_density_1_enabled)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_current_density_re_1).bind(
            self.obj, "CurrentDensity_re_1"
        )

        self.parameter_widget.qsb_current_density_re_2.setProperty(
            "value", self.current_density_re_2
        )
        self.parameter_widget.qsb_current_density_re_2.setEnabled(self.current_density_2_enabled)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_current_density_re_2).bind(
            self.obj, "CurrentDensity_re_2"
        )

        self.parameter_widget.qsb_current_density_re_3.setProperty(
            "value", self.current_density_re_3
        )
        self.parameter_widget.qsb_current_density_re_3.setEnabled(self.current_density_3_enabled)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_current_density_re_3).bind(
            self.obj, "CurrentDensity_re_3"
        )

        self.parameter_widget.qsb_current_density_im_1.setProperty(
            "value", self.current_density_im_1
        )
        self.parameter_widget.qsb_current_density_im_1.setEnabled(self.current_density_1_enabled)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_current_density_im_1).bind(
            self.obj, "CurrentDensity_im_1"
        )

        self.parameter_widget.qsb_current_density_im_2.setProperty(
            "value", self.current_density_im_2
        )
        self.parameter_widget.qsb_current_density_im_2.setEnabled(self.current_density_2_enabled)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_current_density_im_2).bind(
            self.obj, "CurrentDensity_im_2"
        )

        self.parameter_widget.qsb_current_density_im_3.setProperty(
            "value", self.current_density_im_3
        )
        self.parameter_widget.qsb_current_density_im_3.setEnabled(self.current_density_3_enabled)
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_current_density_im_3).bind(
            self.obj, "CurrentDensity_im_3"
        )

        self.parameter_widget.ckb_current_density_1.setChecked(self.current_density_1_enabled)
        self.parameter_widget.ckb_current_density_2.setChecked(self.current_density_2_enabled)
        self.parameter_widget.ckb_current_density_3.setChecked(self.current_density_3_enabled)

        self.parameter_widget.qsb_normal_current_density_re.setProperty(
            "value", self.normal_current_density_re
        )
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_normal_current_density_re).bind(
            self.obj, "NormalCurrentDensity_re"
        )
        self.parameter_widget.qsb_normal_current_density_im.setProperty(
            "value", self.normal_current_density_im
        )
        FreeCADGui.ExpressionBinding(self.parameter_widget.qsb_normal_current_density_im).bind(
            self.obj, "NormalCurrentDensity_im"
        )

        self.mode_enum = self.obj.getEnumerationsOfProperty("Mode")
        index = self.mode_enum.index(self.mode)
        self.parameter_widget.cb_mode.addItems(self.mode_enum)
        self.parameter_widget.cb_mode.setCurrentIndex(index)

    def current_density_1_enabled_changed(self, value):
        self.current_density_1_enabled = value
        self.parameter_widget.qsb_current_density_re_1.setEnabled(value)
        self.parameter_widget.qsb_current_density_im_1.setEnabled(value)

    def current_density_2_enabled_changed(self, value):
        self.current_density_2_enabled = value
        self.parameter_widget.qsb_current_density_re_2.setEnabled(value)
        self.parameter_widget.qsb_current_density_im_2.setEnabled(value)

    def current_density_3_enabled_changed(self, value):
        self.current_density_3_enabled = value
        self.parameter_widget.qsb_current_density_re_3.setEnabled(value)
        self.parameter_widget.qsb_current_density_im_3.setEnabled(value)

    def current_density_re_1_changed(self, value):
        self.current_density_re_1 = value

    def current_density_re_2_changed(self, value):
        self.current_density_re_2 = value

    def current_density_re_3_changed(self, value):
        self.current_density_re_3 = value

    def current_density_im_1_changed(self, value):
        self.current_density_im_1 = value

    def current_density_im_2_changed(self, value):
        self.current_density_im_2 = value

    def current_density_im_3_changed(self, value):
        self.current_density_im_3 = value

    def normal_current_density_re_changed(self, value):
        self.normal_current_density_re = value

    def normal_current_density_im_changed(self, value):
        self.normal_current_density_im = value

    def mode_changed(self, index):
        self.mode = self.mode_enum[index]
        if self.mode == "Custom":
            self.parameter_widget.gb_custom.setEnabled(True)
            self.parameter_widget.gb_normal.setEnabled(False)
        elif self.mode == "Normal":
            self.parameter_widget.gb_custom.setEnabled(False)
            self.parameter_widget.gb_normal.setEnabled(True)
