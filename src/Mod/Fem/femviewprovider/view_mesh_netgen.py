# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2024 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

__title__ = "FreeCAD FEM mesh netgen ViewProvider for the document object"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"

## @package view_mesh_netgen
#  \ingroup FEM
#  \brief view provider for mesh netgen object

import FreeCAD
import FreeCADGui

import FemGui
from PySide import QtGui
from femtaskpanels import task_mesh_netgen
from femtools.femutils import is_of_type
from femviewprovider import view_base_femobject


class VPMeshNetgen(view_base_femobject.VPBaseFemObject):
    """
    A View Provider for the MeshNetgen object
    """

    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/FEM_MeshNetgenFromShape.svg"

    def setEdit(self, vobj, mode):
        # hide all FEM meshes and VTK FemPost* objects
        for obj in vobj.Object.Document.Objects:
            if obj.isDerivedFrom("Fem::FemMeshObject") or obj.isDerivedFrom("Fem::FemPostObject"):
                obj.ViewObject.hide()
        # show the mesh we like to edit
        self.ViewObject.show()
        # show task panel
        taskd = task_mesh_netgen._TaskPanel(self.Object)
        FreeCADGui.Control.showDialog(taskd)
        return True

    def doubleClicked(self, vobj):
        # Group meshing is only active on active analysis
        # we should make sure the analysis the mesh belongs too is active
        gui_doc = FreeCADGui.getDocument(vobj.Object.Document)
        if not gui_doc.getInEdit():
            # may be go the other way around and just activate the
            # analysis the user has doubleClicked on ?!
            # not a fast one, we need to iterate over all member of all
            # analysis to know to which analysis the object belongs too!!!
            # first check if there is an analysis in the active document
            found_an_analysis = False
            for o in gui_doc.Document.Objects:
                if o.isDerivedFrom("Fem::FemAnalysisPython"):
                    found_an_analysis = True
                    break
            if found_an_analysis:
                if FemGui.getActiveAnalysis() is not None:
                    if FemGui.getActiveAnalysis().Document is FreeCAD.ActiveDocument:
                        if self.Object in FemGui.getActiveAnalysis().Group:
                            if not gui_doc.getInEdit():
                                gui_doc.setEdit(vobj.Object.Name)
                            else:
                                FreeCAD.Console.PrintError(
                                    "Activate the analysis this Netgen FEM "
                                    "mesh object belongs too!\n"
                                )
                        else:
                            FreeCAD.Console.PrintMessage(
                                "Netgen FEM mesh object does not belong to the active analysis.\n"
                            )
                            found_mesh_analysis = False
                            for o in gui_doc.Document.Objects:
                                if o.isDerivedFrom("Fem::FemAnalysisPython"):
                                    for m in o.Group:
                                        if m == self.Object:
                                            found_mesh_analysis = True
                                            FemGui.setActiveAnalysis(o)
                                            FreeCAD.Console.PrintMessage(
                                                "The analysis the Netgen FEM mesh object "
                                                "belongs to was found and activated: {}\n".format(
                                                    o.Name
                                                )
                                            )
                                            gui_doc.setEdit(vobj.Object.Name)
                                            break
                            if not found_mesh_analysis:
                                FreeCAD.Console.PrintLog(
                                    "Netgen FEM mesh object does not belong to an analysis. "
                                    "Analysis group meshing can not be used.\n"
                                )
                                gui_doc.setEdit(vobj.Object.Name)
                    else:
                        FreeCAD.Console.PrintError("Active analysis is not in active document.\n")
                else:
                    FreeCAD.Console.PrintLog(
                        "No active analysis in active document, "
                        "we are going to have a look if the Netgen FEM mesh object "
                        "belongs to a non active analysis.\n"
                    )
                    found_mesh_analysis = False
                    for o in gui_doc.Document.Objects:
                        if o.isDerivedFrom("Fem::FemAnalysisPython"):
                            for m in o.Group:
                                if m == self.Object:
                                    found_mesh_analysis = True
                                    FemGui.setActiveAnalysis(o)
                                    FreeCAD.Console.PrintMessage(
                                        "The analysis the Netgen FEM mesh object "
                                        "belongs to was found and activated: {}\n".format(o.Name)
                                    )
                                    gui_doc.setEdit(vobj.Object.Name)
                                    break
                    if not found_mesh_analysis:
                        FreeCAD.Console.PrintLog(
                            "Netgen FEM mesh object does not belong to an analysis. "
                            "Analysis group meshing can not be used.\n"
                        )
                        gui_doc.setEdit(vobj.Object.Name)
            else:
                FreeCAD.Console.PrintLog("No analysis in the active document.\n")
                gui_doc.setEdit(vobj.Object.Name)
        else:
            from PySide.QtGui import QMessageBox

            message = "Active Task Dialog found! Please close this one before opening  a new one!"
            QMessageBox.critical(None, "Error in tree view", message)
            FreeCAD.Console.PrintError(message + "\n")
        return True

    def claimChildren(self):
        reg_childs = self.Object.MeshRegionList
        return reg_childs

    def dumps(self):
        return None

    def loads(self, state):
        return None
