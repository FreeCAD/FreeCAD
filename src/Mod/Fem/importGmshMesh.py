# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 - Qingfeng Xia <ox.ac.uk>             *
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
from __future__ import print_function

__title__ = "FreeCAD Gmsh supported format mesh export"
__author__ = "Qingfeng Xia, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package importGmshMesh
#  \ingroup FEM
#  \brief FreeCAD gmsh supported format mesh export from FemMeshGmsh object

import os
import os.path
import subprocess

import FreeCAD
import FemGui
import FemGmshTools


########## generic FreeCAD import and export methods ##########
# only export are supported, and boundary mesh might also be exported

def export(objectslist, fileString):
    "called when freecad exports a mesh file supprted by gmsh generation"
    if len(objectslist) != 1:
        FreeCAD.Console.PrintError("This exporter can only export one object.\n")
        return
    obj = objectslist[0]
    if not obj.isDerivedFrom("Fem::FemMeshObject"):
        FreeCAD.Console.PrintError("No FEM mesh object selected.\n")
        return
    #if not obj.Type == 'FemMeshGmsh':
    #    FreeCAD.Console.PrintError("Object selected is not a FemMeshGmsh type\n")
    #    return

    gmsh = FemGmshTools.FemGmshTools(obj, FemGui.getActiveAnalysis())
    if fileString != "":
        fileName, fileExtension = os.path.splitext(fileString)
        for k in FemGmshTools.FemGmshTools.output_format_suffix:
            if FemGmshTools.FemGmshTools.output_format_suffix[k] == fileExtension.lower():
                ret = gmsh.export_mesh(k, fileString)
                if not ret:
                    FreeCAD.Console.PrintError("Mesh is written to `{}` by Gmsh\n".format(ret))
                return
        FreeCAD.Console.PrintError("Export mesh format with suffix `{}` is not supported by Gmsh\n".format(fileExtension.lower()) )


def _run_command(comandlist):
    print("Run command: " + ' '.join(comandlist))
    error = None
    try:
        p = subprocess.Popen(comandlist, shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        output, error = p.communicate()
        #print(output)  # stdout is still cut at some point but the warnings are in stderr and thus printed :-)
        #print(error)
    except Exception as e:
        error = 'Error executing: {}\n'.format(' '.join(comandlist))
        print(e)
        FreeCAD.Console.PrintError(error)
    return error


def export_fenics_mesh(obj, meshfileString):
    if not meshfileString[-4:] == ".xml":
        error = "Error: only xml mesh is supported by gmsh conversion"
        FreeCAD.Console.PrintError(error)
        return error
    meshfileStem = meshfileString[:-4]  #.encode('ascii')  # FIXME

    gmsh = FemGmshTools.FemGmshTools(obj, FemGui.getActiveAnalysis())
    meshfile = gmsh.export_mesh(u"Gmsh MSH", meshfileStem + ".msh")
    if meshfile:
        msg = "Info: Mesh has been written to `{}` by Gmsh\n".format(meshfile)
        FreeCAD.Console.PrintMessage(msg)
        # once Fenics is installed, dolfin-convert should be in path
        comandlist = [u'dolfin-convert', u'-i gmsh', meshfileStem + u".msh", meshfileStem + u".xml"]
        # mixed str and unicode cause error in comandlist
        error = _run_command(comandlist)
        if not os.path.exists(meshfileStem+"_physical_region.xml"):
            FreeCAD.Console.PrintWarning("Mesh  boundary file `{}` not generated\n".format(meshfileStem+"_physical_region.xml"))
        if error:
            return error
    else:
        error = "Failed to write mesh file `{}` by Gmsh\n".format(meshfileString)
        FreeCAD.Console.PrintError(errror)
        return error


def show_fenics_mesh(fname):
    # boundary layer, quad element is not supported
    from dolfin import Mesh, MeshFunction, plot, interactive
    mesh = Mesh(fname+".xml")
    if os.path.exists( fname+"_physical_region.xml"):
        subdomains = MeshFunction("size_t", mesh, fname+"_physical_region.xml")
        plot(subdomains)
    if os.path.exists( fname+"_facet_region.xml"):
        boundaries = MeshFunction("size_t", mesh, fname+"_facet_region.xml")
        plot(boundaries)

    plot(mesh)
    interactive()


def export_foam_mesh(obj, meshfileString):
    # support only 3D
    gmsh = FemGmshTools.FemGmshTools(obj, FemGui.getActiveAnalysis())
    meshfile = gmsh.export_mesh(u"Gmsh MSH", meshfileString)
    if meshfile:
        msg = "Info: Mesh is not written to `{}` by Gmsh\n".format(meshfile)
        FreeCAD.Console.PrintMessage(msg)
        comandlist = ['gmshToFoam', '-case', foamCaseFolder, meshfileStem + ".msh"]
        return _run_command(comandlist)
    else:
        error = "Mesh is NOT written to `{}` by Gmsh\n".format(meshfileString)
        FreeCAD.Console.PrintError(errror)
        return error
