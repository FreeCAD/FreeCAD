# ***************************************************************************
# *   Copyright (c) 2015 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM element geometry 2D ViewProvider for the document object"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package ViewProviderFemElementGeometry2D
#  \ingroup FEM
#  \brief FreeCAD FEM _ViewProviderFemElementGeometry2D

import FreeCAD
import FreeCADGui

from PySide import QtCore

from . import FemSelectionWidgets
from . import ViewProviderFemConstraint


class _ViewProviderFemElementGeometry2D(ViewProviderFemConstraint.ViewProxy):
    """
    A View Provider for the FemElementGeometry2D object
    """

    def setEdit(self, vobj, mode=0):
        ViewProviderFemConstraint.ViewProxy.setEdit(
            self,
            vobj,
            mode,
            _TaskPanel
        )


class _TaskPanel:
    """
    The TaskPanel for editing References property of FemElementGeometry2D objects
    """

    def __init__(self, obj):

        self.obj = obj

        # parameter widget
        self.parameterWidget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/ElementGeometry2D.ui"
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_thickness,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.thickness_changed
        )
        self.init_parameter_widget()

        # geometry selection widget
        self.selectionWidget = FemSelectionWidgets.GeometryElementsSelection(
            obj.References,
            ["Face"]
        )

        # form made from param and selection widget
        self.form = [self.parameterWidget, self.selectionWidget]

    def accept(self):
        self.obj.Thickness = self.thickness
        self.obj.References = self.selectionWidget.references
        self.recompute_and_set_back_all()
        return True

    def reject(self):
        self.recompute_and_set_back_all()
        return True

    def recompute_and_set_back_all(self):
        doc = FreeCADGui.getDocument(self.obj.Document)
        doc.Document.recompute()
        self.selectionWidget.setback_listobj_visibility()
        if self.selectionWidget.sel_server:
            FreeCADGui.Selection.removeObserver(self.selectionWidget.sel_server)
        doc.resetEdit()

    def init_parameter_widget(self):
        self.thickness = self.obj.Thickness
        self.parameterWidget.if_thickness.setText(self.thickness.UserString)

    def thickness_changed(self, base_quantity_value):
        self.thickness = base_quantity_value
