# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 - Bernd Hahnebach <bernd@bimstatik.org>            *
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

__title__ = "_ViewProviderFemMeshGroup"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package ViewProviderFemMeshGroup
#  \ingroup FEM

import FreeCAD
import FreeCADGui
import FemGui  # needed to display the icons in TreeView
False if False else FemGui.__name__  # dummy usage of FemGui for flake8, just returns 'FemGui'

# for the panel
from PySide import QtCore
from . import FemSelectionWidgets


class _ViewProviderFemMeshGroup:
    "A View Provider for the FemMeshGroup object"
    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/fem-femmesh-from-shape.svg"

    def attach(self, vobj):
        from pivy import coin
        self.ViewObject = vobj
        self.Object = vobj.Object
        self.standard = coin.SoGroup()
        vobj.addDisplayMode(self.standard, "Standard")

    def getDisplayModes(self, obj):
        return ["Standard"]

    def getDefaultDisplayMode(self):
        return "Standard"

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
        taskd = _TaskPanelFemMeshGroup(self.Object)
        taskd.obj = vobj.Object
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        return

    def doubleClicked(self, vobj):
        guidoc = FreeCADGui.getDocument(vobj.Object.Document)
        # check if another VP is in edit mode, https://forum.freecadweb.org/viewtopic.php?t=13077#p104702
        if not guidoc.getInEdit():
            guidoc.setEdit(vobj.Object.Name)
        else:
            from PySide.QtGui import QMessageBox
            message = 'Active Task Dialog found! Please close this one before open a new one!'
            QMessageBox.critical(None, "Error in tree view", message)
            FreeCAD.Console.PrintError(message + '\n')
        return True

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None


class _TaskPanelFemMeshGroup:
    '''The TaskPanel for editing References property of FemMeshGroup objects'''

    def __init__(self, obj):

        # parameter widget
        self.obj = obj
        self.parameterWidget = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/MeshGroup.ui")
        QtCore.QObject.connect(self.parameterWidget.rb_name, QtCore.SIGNAL("toggled(bool)"), self.choose_exportidentifier_name)
        QtCore.QObject.connect(self.parameterWidget.rb_label, QtCore.SIGNAL("toggled(bool)"), self.choose_exportidentifier_label)
        self.init_parameter_widget()

        # geometry selection widget
        self.selectionWidget = FemSelectionWidgets.GeometryElementsSelection(obj.References, ['Solid', 'Face', 'Edge', 'Vertex'])  # start with Solid in list!

        # form made from param and selection widget
        self.form = [self.parameterWidget, self.selectionWidget]

    def accept(self):
        self.obj.UseLabel = self.use_label
        self.obj.References = self.selectionWidget.references
        FreeCAD.ActiveDocument.recompute()
        self.set_back_all()
        return True

    def reject(self):
        self.set_back_all()
        return True

    def set_back_all(self):
        self.selectionWidget.setback_listobj_visibility()
        if self.selectionWidget.sel_server:
            FreeCADGui.Selection.removeObserver(self.selectionWidget.sel_server)
        FreeCADGui.ActiveDocument.resetEdit()

    def init_parameter_widget(self):
        self.use_label = self.obj.UseLabel
        self.parameterWidget.rb_name.setChecked(not self.use_label)
        self.parameterWidget.rb_label.setChecked(self.use_label)

    def choose_exportidentifier_name(self, state):
        self.use_label = not state

    def choose_exportidentifier_label(self, state):
        self.use_label = state
