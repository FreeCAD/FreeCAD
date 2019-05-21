# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM element geometry 1D ViewProvider for the document object"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package ViewProviderFemElementGeometry1D
#  \ingroup FEM
#  \brief FreeCAD FEM _ViewProviderFemElementGeometry1D

import FreeCAD
import FreeCADGui
import FemGui  # needed to display the icons in TreeView
False if False else FemGui.__name__  # dummy usage of FemGui for flake8, just returns 'FemGui'

# for the panel
from femobjects import _FemElementGeometry1D
from PySide import QtCore
from . import FemSelectionWidgets


class _ViewProviderFemElementGeometry1D:
    "A View Provider for the FemElementGeometry1D object"

    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/fem-element-geometry-1d.svg"

    def attach(self, vobj):
        from pivy import coin
        self.ViewObject = vobj
        self.Object = vobj.Object
        self.standard = coin.SoGroup()
        vobj.addDisplayMode(self.standard, "Default")

    def getDisplayModes(self, obj):
        return ["Default"]

    def getDefaultDisplayMode(self):
        return "Default"

    def updateData(self, obj, prop):
        return

    def onChanged(self, vobj, prop):
        return

    def setEdit(self, vobj, mode=0):
        # hide all meshes
        for o in FreeCAD.ActiveDocument.Objects:
            if o.isDerivedFrom("Fem::FemMeshObject"):
                o.ViewObject.hide()
        # show task panel
        taskd = _TaskPanelFemElementGeometry1D(self.Object)
        taskd.obj = vobj.Object
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        return True

    def doubleClicked(self, vobj):
        guidoc = FreeCADGui.getDocument(vobj.Object.Document)
        # check if another VP is in edit mode
        # https://forum.freecadweb.org/viewtopic.php?t=13077#p104702
        if not guidoc.getInEdit():
            guidoc.setEdit(vobj.Object.Name)
        else:
            from PySide.QtGui import QMessageBox
            message = 'Active Task Dialog found! Please close this one before opening  a new one!'
            QMessageBox.critical(None, "Error in tree view", message)
            FreeCAD.Console.PrintError(message + '\n')
        return True

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None


class _TaskPanelFemElementGeometry1D:
    '''The TaskPanel for editing References property of FemElementGeometry1D objects'''

    def __init__(self, obj):

        self.obj = obj

        # parameter widget
        self.parameterWidget = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/ElementGeometry1D.ui"
        )
        QtCore.QObject.connect(
            self.parameterWidget.cb_crosssectiontype,
            QtCore.SIGNAL("activated(int)"),
            self.sectiontype_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_rec_height,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.rec_height_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_rec_width,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.rec_width_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_circ_diameter,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.circ_diameter_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_pipe_diameter,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.pipe_diameter_changed
        )
        QtCore.QObject.connect(
            self.parameterWidget.if_pipe_thickness,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.pipe_thickness_changed
        )

        # it is inside the class thus double _FemElementGeometry1D
        self.parameterWidget.cb_crosssectiontype.addItems(
            _FemElementGeometry1D._FemElementGeometry1D.known_beam_types
        )
        self.get_beamsection_props()
        self.updateParameterWidget()

        # geometry selection widget
        self.selectionWidget = FemSelectionWidgets.GeometryElementsSelection(
            obj.References,
            ['Edge']
        )

        # form made from param and selection widget
        self.form = [self.parameterWidget, self.selectionWidget]

    def accept(self):
        self.set_beamsection_props()
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

    def get_beamsection_props(self):
        self.SectionType = self.obj.SectionType
        self.RectHeight = self.obj.RectHeight
        self.RectWidth = self.obj.RectWidth
        self.CircDiameter = self.obj.CircDiameter
        self.PipeDiameter = self.obj.PipeDiameter
        self.PipeThickness = self.obj.PipeThickness

    def set_beamsection_props(self):
        self.obj.SectionType = self.SectionType
        self.obj.RectHeight = self.RectHeight
        self.obj.RectWidth = self.RectWidth
        self.obj.CircDiameter = self.CircDiameter
        self.obj.PipeDiameter = self.PipeDiameter
        self.obj.PipeThickness = self.PipeThickness

    def updateParameterWidget(self):
        'fills the widgets'
        index_crosssectiontype = self.parameterWidget.cb_crosssectiontype.findText(
            self.SectionType
        )
        self.parameterWidget.cb_crosssectiontype.setCurrentIndex(index_crosssectiontype)
        self.parameterWidget.if_rec_height.setText(self.RectHeight.UserString)
        self.parameterWidget.if_rec_width.setText(self.RectWidth.UserString)
        self.parameterWidget.if_circ_diameter.setText(self.CircDiameter.UserString)
        self.parameterWidget.if_pipe_diameter.setText(self.PipeDiameter.UserString)
        self.parameterWidget.if_pipe_thickness.setText(self.PipeThickness.UserString)

    def sectiontype_changed(self, index):
        if index < 0:
            return
        self.parameterWidget.cb_crosssectiontype.setCurrentIndex(index)
        # parameterWidget returns unicode
        self.SectionType = str(self.parameterWidget.cb_crosssectiontype.itemText(index))

    def rec_height_changed(self, base_quantity_value):
        self.RectHeight = base_quantity_value

    def rec_width_changed(self, base_quantity_value):
        self.RectWidth = base_quantity_value

    def circ_diameter_changed(self, base_quantity_value):
        self.CircDiameter = base_quantity_value

    def pipe_diameter_changed(self, base_quantity_value):
        self.PipeDiameter = base_quantity_value

    def pipe_thickness_changed(self, base_quantity_value):
        self.PipeThickness = base_quantity_value
