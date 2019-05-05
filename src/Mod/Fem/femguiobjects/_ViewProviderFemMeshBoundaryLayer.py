# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM mesh boundary layer ViewProvider for the document object"
__author__ = "Bernd Hahnebach, Qingfeng Xia"
__url__ = "http://www.freecadweb.org"

## @package ViewProviderFemMeshBoundaryLayer
#  \ingroup FEM
#  \brief FreeCAD FEM _ViewProviderFemMeshBoundaryLayer

import FreeCAD
import FreeCADGui
import FemGui  # needed to display the icons in TreeView
False if False else FemGui.__name__  # dummy usage of FemGui for flake8, just returns 'FemGui'

# for the panel
from PySide import QtCore
from . import FemSelectionWidgets


class _ViewProviderFemMeshBoundaryLayer:
    "A View Provider for the FemMeshBoundaryLayer object"

    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/fem-femmesh-boundary-layer.svg"

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
        taskd = _TaskPanelFemMeshBoundaryLayer(self.Object)
        taskd.obj = vobj.Object
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        return True

    def doubleClicked(self, vobj):
        guidoc = FreeCADGui.getDocument(vobj.Object.Document)
        # check if another VP is in edit mode, https://forum.freecadweb.org/viewtopic.php?t=13077#p104702
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


class _TaskPanelFemMeshBoundaryLayer:
    '''The TaskPanel for editing References property of FemMeshBoundaryLayer objects'''

    def __init__(self, obj):

        self.obj = obj

        # parameter widget
        self.parameterWidget = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/MeshBoundaryLayer.ui")
        QtCore.QObject.connect(self.parameterWidget.bl_number_of_layers, QtCore.SIGNAL("valueChanged(int)"), self.bl_number_of_layers_changed)
        QtCore.QObject.connect(self.parameterWidget.bl_min_thickness, QtCore.SIGNAL("valueChanged(Base::Quantity)"), self.bl_min_thickness_changed)
        QtCore.QObject.connect(self.parameterWidget.bl_growth_rate, QtCore.SIGNAL("valueChanged(double)"), self.bl_growth_rate_changed)  # becareful of signal signature for QDoubleSpinbox
        self.init_parameter_widget()

        # geometry selection widget
        self.selectionWidget = FemSelectionWidgets.GeometryElementsSelection(obj.References, ['Solid', 'Face', 'Edge', 'Vertex'])  # start with Solid in list!

        # form made from param and selection widget
        self.form = [self.parameterWidget, self.selectionWidget]

    def accept(self):
        self.set_mesh_boundarylayer_props()
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
        self.bl_min_thickness = self.obj.MinimumThickness
        self.bl_number_of_layers = self.obj.NumberOfLayers
        self.bl_growth_rate = self.obj.GrowthRate
        self.parameterWidget.bl_min_thickness.setText(self.bl_min_thickness.UserString)
        self.parameterWidget.bl_number_of_layers.setValue(self.bl_number_of_layers)
        self.parameterWidget.bl_growth_rate.setValue(self.bl_growth_rate)

    def set_mesh_boundarylayer_props(self):
        self.obj.MinimumThickness = self.bl_min_thickness
        self.obj.NumberOfLayers = self.bl_number_of_layers
        self.obj.GrowthRate = self.bl_growth_rate

    def bl_min_thickness_changed(self, base_quantity_value):
        self.bl_min_thickness = base_quantity_value

    def bl_number_of_layers_changed(self, value):
        self.bl_number_of_layers = value

    def bl_growth_rate_changed(self, value):
        self.bl_growth_rate = value
