# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM element rotation 1D ViewProvider for the document object"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package ViewProviderFemElementRotation1D
#  \ingroup FEM
#  \brief FreeCAD FEM _ViewProviderFemElementRotation1D

import FreeCAD
import FreeCADGui
import FemGui  # needed to display the icons in TreeView
False if False else FemGui.__name__  # dummy usage of FemGui for flake8, just returns 'FemGui'

# for the panel
from PySide import QtCore
from . import FemSelectionWidgets


class _ViewProviderFemElementRotation1D:
    "A View Provider for the FemElementRotation1D object"
    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/fem-element-rotation-1d.svg"

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

    '''
    # do not activate the task panel, since rotation with reference shapes is not yet supported
    def setEdit(self, vobj, mode=0):
        # hide all meshes
        for o in FreeCAD.ActiveDocument.Objects:
            if o.isDerivedFrom("Fem::FemMeshObject"):
                o.ViewObject.hide()
        # show task panel
        taskd = _TaskPanelFemElementRotation1D(self.Object)
        taskd.obj = vobj.Object
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        return True
    '''

    def setEdit(self, vobj, mode=0):
        # avoid edit mode by return False, https://forum.freecadweb.org/viewtopic.php?t=12139&start=10#p161062
        return False

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


class _TaskPanelFemElementRotation1D:
    '''The TaskPanel for editing References property of FemElementRotation1D objects'''
    def __init__(self, obj):

        self.obj = obj

        # parameter widget
        self.parameterWidget = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/ElementRotation1D.ui")
        QtCore.QObject.connect(self.parameterWidget.if_rotation, QtCore.SIGNAL("valueChanged(Base::Quantity)"), self.rotation_changed)
        self.rotation = self.obj.Rotation
        self.parameterWidget.if_rotation.setText(self.rotation.UserString)

        # geometry selection widget
        self.selectionWidget = FemSelectionWidgets.GeometryElementsSelection(obj.References, ['Edge'])

        # form made from param and selection widget
        self.form = [self.parameterWidget, self.selectionWidget]

    def accept(self):
        self.obj.Rotation = self.rotation
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

    def rotation_changed(self, base_quantity_value):
        self.rotation = base_quantity_value
