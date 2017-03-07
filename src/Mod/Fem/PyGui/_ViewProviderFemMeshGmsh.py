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

__title__ = "_ViewProviderFemMeshGmsh"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package ViewProviderFemMeshGmsh
#  \ingroup FEM

import FreeCAD
import FreeCADGui
import FemGui


class _ViewProviderFemMeshGmsh:
    "A View Provider for the FemMeshGmsh object"
    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/fem-femmesh-from-shape.svg"

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

    def updateData(self, obj, prop):
        return

    def onChanged(self, vobj, prop):
        return

    def setEdit(self, vobj, mode):
        self.ViewObject.show()  # show the mesh on edit if it is hided
        import PyGui._TaskPanelFemMeshGmsh
        taskd = PyGui._TaskPanelFemMeshGmsh._TaskPanelFemMeshGmsh(self.Object)
        taskd.obj = vobj.Object
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self, vobj, mode):
        FreeCADGui.Control.closeDialog()
        return

    def doubleClicked(self, vobj):
        FreeCADGui.activateWorkbench('FemWorkbench')
        # Group meshing is only active on active analysis, we should make sure the analysis the mesh belongs too is active
        gui_doc = FreeCADGui.getDocument(vobj.Object.Document)
        if not gui_doc.getInEdit():
            # may be go the other way around and just activate the analysis the user has doubleClicked on ?!
            # not a fast one, we need to iterate over all member of all analysis to know to which analyis the object belongs too!!!
            # first check if there is an analysis in the active document
            found_an_analysis = False
            for o in gui_doc.Document.Objects:
                if o.isDerivedFrom('Fem::FemAnalysisPython'):
                        found_an_analysis = True
                        break
            if found_an_analysis:
                if FemGui.getActiveAnalysis() is not None:
                    if FemGui.getActiveAnalysis().Document is FreeCAD.ActiveDocument:
                        if self.Object in FemGui.getActiveAnalysis().Member:
                            if not gui_doc.getInEdit():
                                gui_doc.setEdit(vobj.Object.Name)
                            else:
                                FreeCAD.Console.PrintError('Activate the analysis this GMSH FEM mesh object belongs too!\n')
                        else:
                            print('GMSH FEM mesh object does not belong to the active analysis.')
                            found_mesh_analysis = False
                            for o in gui_doc.Document.Objects:
                                if o.isDerivedFrom('Fem::FemAnalysisPython'):
                                    for m in o.Member:
                                        if m == self.Object:
                                            found_mesh_analysis = True
                                            FemGui.setActiveAnalysis(o)
                                            print('The analysis the GMSH FEM mesh object belongs too was found and activated: ' + o.Name)
                                            gui_doc.setEdit(vobj.Object.Name)
                                            break
                            if not found_mesh_analysis:
                                print('GMSH FEM mesh object does not belong to an analysis. Analysis group meshing will be deactivated.')
                                gui_doc.setEdit(vobj.Object.Name)
                    else:
                        FreeCAD.Console.PrintError('Active analysis is not in active document.')
                else:
                    print('No active analysis in active document, we gone have a look if the GMSH FEM mesh object belongs to a non active analysis.')
                    found_mesh_analysis = False
                    for o in gui_doc.Document.Objects:
                        if o.isDerivedFrom('Fem::FemAnalysisPython'):
                            for m in o.Member:
                                if m == self.Object:
                                    found_mesh_analysis = True
                                    FemGui.setActiveAnalysis(o)
                                    print('The analysis the GMSH FEM mesh object belongs too was found and activated: ' + o.Name)
                                    gui_doc.setEdit(vobj.Object.Name)
                                    break
                    if not found_mesh_analysis:
                        print('GMSH FEM mesh object does not belong to an analysis. Analysis group meshing will be deactivated.')
                        gui_doc.setEdit(vobj.Object.Name)
            else:
                print('No analysis in the active document.')
                gui_doc.setEdit(vobj.Object.Name)
        else:
            FreeCAD.Console.PrintError('Active Task Dialog found! Please close this one first!\n')
        return True

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def claimChildren(self):
        return (self.Object.MeshRegionList + self.Object.MeshGroupList)

    def onDelete(self, feature, subelements):
        try:
            for obj in self.claimChildren():
                obj.ViewObject.show()
        except Exception as err:
            FreeCAD.Console.PrintError("Error in onDelete: " + err.message)
        return True
