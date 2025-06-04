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

__title__ = "FreeCAD FEM mesh gmsh task panel for the document object"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package task_mesh_gmsh
#  \ingroup FEM
#  \brief task panel for mesh gmsh object

from PySide import QtCore

import FreeCAD
import FreeCADGui

from femmesh import gmshtools

from . import base_femlogtaskpanel


class _TaskPanel(base_femlogtaskpanel._BaseLogTaskPanel):
    """
    The TaskPanel for editing References property of
    MeshGmsh objects and creation of new FEM mesh
    """

    def __init__(self, obj):
        super().__init__(obj, gmshtools.GmshTools(obj))

        self.form = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/MeshGmsh.ui"
        )
        self.text_log = self.form.te_output
        self.text_time = self.form.l_time

        self.setup_connections()

    def setup_connections(self):
        super().setup_connections()

        QtCore.QObject.connect(
            self.form.qsb_max_size, QtCore.SIGNAL("valueChanged(Base::Quantity)"), self.max_changed
        )
        QtCore.QObject.connect(
            self.form.qsb_min_size, QtCore.SIGNAL("valueChanged(Base::Quantity)"), self.min_changed
        )
        QtCore.QObject.connect(
            self.form.cb_dimension, QtCore.SIGNAL("activated(int)"), self.choose_dimension
        )
        QtCore.QObject.connect(
            self.form.cb_order, QtCore.SIGNAL("activated(int)"), self.choose_order
        )
        self.form.cb_dimension.addItems(self.obj.getEnumerationsOfProperty("ElementDimension"))

        self.form.cb_order.addItems(self.obj.getEnumerationsOfProperty("ElementOrder"))
        QtCore.QObject.connect(
            self.form.pb_get_gmsh_version, QtCore.SIGNAL("clicked()"), self.get_version
        )

        self.get_object_params()
        self.set_widgets()

    def get_object_params(self):
        self.clmax = self.obj.CharacteristicLengthMax
        self.clmin = self.obj.CharacteristicLengthMin
        self.dimension = self.obj.ElementDimension
        self.order = self.obj.ElementOrder

    def set_object_params(self):
        self.obj.CharacteristicLengthMax = self.clmax
        self.obj.CharacteristicLengthMin = self.clmin
        self.obj.ElementDimension = self.dimension
        self.obj.ElementOrder = self.order

    def set_widgets(self):
        "fills the widgets"
        self.form.qsb_max_size.setProperty("value", self.clmax)
        FreeCADGui.ExpressionBinding(self.form.qsb_max_size).bind(
            self.obj, "CharacteristicLengthMax"
        )
        self.form.qsb_min_size.setProperty("value", self.clmin)
        FreeCADGui.ExpressionBinding(self.form.qsb_min_size).bind(
            self.obj, "CharacteristicLengthMin"
        )
        index_dimension = self.form.cb_dimension.findText(self.dimension)
        self.form.cb_dimension.setCurrentIndex(index_dimension)
        index_order = self.form.cb_order.findText(self.order)
        self.form.cb_order.setCurrentIndex(index_order)

    def max_changed(self, base_quantity_value):
        self.clmax = base_quantity_value

    def min_changed(self, base_quantity_value):
        self.clmin = base_quantity_value

    def choose_dimension(self, index):
        if index < 0:
            return
        self.form.cb_dimension.setCurrentIndex(index)
        self.dimension = self.form.cb_dimension.itemText(index)

    def choose_order(self, index):
        if index < 0:
            return
        self.form.cb_order.setCurrentIndex(index)
        self.order = self.form.cb_order.itemText(index)
