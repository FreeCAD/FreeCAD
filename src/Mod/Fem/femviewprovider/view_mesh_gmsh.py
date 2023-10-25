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

__title__ = "FreeCAD FEM mesh gmsh ViewProvider for the document object"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package view_mesh_gmsh
#  \ingroup FEM
#  \brief view provider for mesh gmsh object

import FreeCAD
import FreeCADGui

import FemGui
from PySide import QtGui
from femtaskpanels import task_mesh_gmsh
from femtools.femutils import is_of_type


# TODO use VPBaseFemObject from view_base_femobject
# class VPMeshGmsh(view_base_femobject.VPBaseFemObject):
class VPMeshGmsh:
    """
    A View Provider for the MeshGmsh object
    """

    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/FEM_MeshGmshFromShape.svg"

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

    def updateData(self, obj, prop):
        return

    def onChanged(self, vobj, prop):
        return

    def setEdit(self, vobj, mode):
        # hide all FEM meshes and VTK FemPost* objects
        for obj in vobj.Object.Document.Objects:
            if (
                obj.isDerivedFrom("Fem::FemMeshObject")
                or obj.isDerivedFrom("Fem::FemPostClipFilter")
                or obj.isDerivedFrom("Fem::FemPostContoursFilter")
                or obj.isDerivedFrom("Fem::FemPostCutFilter")
                or obj.isDerivedFrom("Fem::FemPostDataAlongLineFilter")
                or obj.isDerivedFrom("Fem::FemPostDataAtPointFilter")
                or obj.isDerivedFrom("Fem::FemPostPipeline")
                or obj.isDerivedFrom("Fem::FemPostPlaneFunction")
                or obj.isDerivedFrom("Fem::FemPostScalarClipFilter")
                or obj.isDerivedFrom("Fem::FemPostSphereFunction")
                or obj.isDerivedFrom("Fem::FemPostWarpVectorFilter")
            ):
                obj.ViewObject.hide()
        # show the mesh we like to edit
        self.ViewObject.show()
        # show task panel
        taskd = task_mesh_gmsh._TaskPanel(self.Object)
        # taskd.obj = vobj.Object
        FreeCADGui.Control.showDialog(taskd)
        return True

    """
    def setEdit(self, vobj, mode=0):
        view_base_femconstraint.VPBaseFemConstraint.setEdit(
            self,
            vobj,
            mode,
            _TaskPanel
        )
    """

    # overwrite unsetEdit
    def unsetEdit(self, vobj, mode):
        FreeCADGui.Control.closeDialog()
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
                                    "Activate the analysis this Gmsh FEM "
                                    "mesh object belongs too!\n"
                                )
                        else:
                            FreeCAD.Console.PrintMessage(
                                "Gmsh FEM mesh object does not belong to the active analysis.\n"
                            )
                            found_mesh_analysis = False
                            for o in gui_doc.Document.Objects:
                                if o.isDerivedFrom("Fem::FemAnalysisPython"):
                                    for m in o.Group:
                                        if m == self.Object:
                                            found_mesh_analysis = True
                                            FemGui.setActiveAnalysis(o)
                                            FreeCAD.Console.PrintMessage(
                                                "The analysis the Gmsh FEM mesh object "
                                                "belongs to was found and activated: {}\n"
                                                .format(o.Name)
                                            )
                                            gui_doc.setEdit(vobj.Object.Name)
                                            break
                            if not found_mesh_analysis:
                                FreeCAD.Console.PrintLog(
                                    "Gmsh FEM mesh object does not belong to an analysis. "
                                    "Analysis group meshing can not be used.\n"
                                )
                                gui_doc.setEdit(vobj.Object.Name)
                    else:
                        FreeCAD.Console.PrintError("Active analysis is not in active document.\n")
                else:
                    FreeCAD.Console.PrintLog(
                        "No active analysis in active document, "
                        "we are going to have a look if the Gmsh FEM mesh object "
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
                                        "The analysis the Gmsh FEM mesh object "
                                        "belongs to was found and activated: {}\n"
                                        .format(o.Name)
                                    )
                                    gui_doc.setEdit(vobj.Object.Name)
                                    break
                    if not found_mesh_analysis:
                        FreeCAD.Console.PrintLog(
                            "Gmsh FEM mesh object does not belong to an analysis. "
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

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def claimChildren(self):
        reg_childs = self.Object.MeshRegionList
        gro_childs = self.Object.MeshGroupList
        bou_childs = self.Object.MeshBoundaryLayerList
        return (reg_childs + gro_childs + bou_childs)

    def onDelete(self, feature, subelements):
        children = self.claimChildren()
        if len(children) > 0:
            # issue a warning
            message_text = (
                "The mesh contains submesh objects, therefore the\n"
                "following referencing objects might be lost:\n"
            )
            for obj in children:
                message_text += "\n" + obj.Label
            message_text += "\n\nAre you sure you want to continue?"
            reply = QtGui.QMessageBox.warning(
                None,
                "Object dependencies",
                message_text,
                QtGui.QMessageBox.Yes | QtGui.QMessageBox.No,
                QtGui.QMessageBox.No
            )
            if reply == QtGui.QMessageBox.Yes:
                return True
            else:
                return False
        return True

    def canDragObjects(self):
        return True

    def canDropObjects(self):
        return True

    def canDragObject(self, dragged_object):
        if (
            is_of_type(dragged_object, "Fem::MeshBoundaryLayer")
            or is_of_type(dragged_object, "Fem::MeshGroup")
            or is_of_type(dragged_object, "Fem::MeshRegion")
        ):
            return True
        else:
            return False

    def canDropObject(self, incoming_object):
        return True

    def dragObject(self, selfvp, dragged_object):
        if is_of_type(dragged_object, "Fem::MeshBoundaryLayer"):
            objs = self.Object.MeshBoundaryLayerList
            objs.remove(dragged_object)
            self.Object.MeshBoundaryLayerList = objs
        elif is_of_type(dragged_object, "Fem::MeshGroup"):
            objs = self.Object.MeshGroupList
            objs.remove(dragged_object)
            self.Object.MeshGroupList = objs
        elif is_of_type(dragged_object, "Fem::MeshRegion"):
            objs = self.Object.MeshRegionList
            objs.remove(dragged_object)
            self.Object.MeshRegionList = objs

    def dropObject(self, selfvp, incoming_object):
        if is_of_type(incoming_object, "Fem::MeshBoundaryLayer"):
            objs = self.Object.MeshBoundaryLayerList
            objs.append(incoming_object)
            self.Object.MeshBoundaryLayerList = objs
        elif is_of_type(incoming_object, "Fem::MeshGroup"):
            objs = self.Object.MeshGroupList
            objs.append(incoming_object)
            self.Object.MeshGroupList = objs
        elif is_of_type(incoming_object, "Fem::MeshRegion"):
            objs = self.Object.MeshRegionList
            objs.append(incoming_object)
            self.Object.MeshRegionList = objs
        incoming_object.Document.recompute()
