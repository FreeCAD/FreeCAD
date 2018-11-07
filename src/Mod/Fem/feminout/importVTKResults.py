# ***************************************************************************
# *   (c) Qingfeng Xia 2017                       *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Lesser General Public License for more details.                   *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# *   Juergen Riegel 2002                                                   *
# ***************************************************************************/

__title__ = "FreeCAD Result import and export VTK file library"
__author__ = "Qingfeng Xia, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package importVTKResults
#  \ingroup FEM
#  \brief FreeCAD Result import and export VTK file library

import os
import FreeCAD
import Fem


# ********* generic FreeCAD import and export methods *********
if open.__module__ == '__builtin__':
    # because we'll redefine open below (Python2)
    pyopen = open
elif open.__module__ == 'io':
    # because we'll redefine open below (Python3)
    pyopen = open


def open(filename):
    "called when freecad opens a file"
    docname = os.path.splitext(os.path.basename(filename))[0]
    insert(filename, docname)


def insert(filename, docname):
    "called when freecad wants to import a file"
    try:
        doc = FreeCAD.getDocument(docname)
    except NameError:
        doc = FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc
    importVtk(filename)


def export(objectslist, filename):
    "called when freecad exports an object to vtk"
    if len(objectslist) > 1:  # the case of no selected obj is caught by FreeCAD already
        FreeCAD.Console.PrintError("This exporter can only export one object at once\n")
        return

    obj = objectslist[0]
    if obj.isDerivedFrom("Fem::FemPostPipeline"):
        FreeCAD.Console.PrintError('Export of a VTK post object to vtk is not yet implemented !\n')
        return
    elif obj.isDerivedFrom("Fem::FemMeshObject"):
        FreeCAD.Console.PrintError('Use export to FEM mesh formats to export a FEM mesh object to vtk!\n')
        return
    elif obj.isDerivedFrom("Fem::FemResultObject"):
        Fem.writeResult(filename, obj)
    else:
        FreeCAD.Console.PrintError('Selected object is not supported by export to VTK.\n')
        return


# ********* module specific methods *********
def importVtk(filename, object_name=None, object_type=None):
    if not object_type:
        vtkinout_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/InOutVtk")
        object_type = vtkinout_prefs.GetInt("ImportObject", 0)
    if not object_name:
        object_name = os.path.splitext(os.path.basename(filename))[0]
    if object_type == 0:
        # vtk result object
        importVtkVtkResult(filename, object_name)
    elif object_type == 1:
        # FEM mesh object
        importVtkFemMesh(filename, object_name)
    elif object_type == 2:
        # FreeCAD result object
        importVtkFCResult(filename, object_name)
    else:
        FreeCAD.Console.PrintError('Error, wrong parameter in VTK import pref: {}\n'.format(object_type))


def importVtkVtkResult(filename, resultname):
    vtk_result_obj = FreeCAD.ActiveDocument.addObject("Fem::FemPostPipeline", resultname)
    vtk_result_obj.read(filename)
    vtk_result_obj.touch()
    FreeCAD.ActiveDocument.recompute()


def importVtkFemMesh(filename, meshname):
    meshobj = FreeCAD.ActiveDocument.addObject("Fem::FemMeshObject", meshname)
    meshobj.FemMesh = Fem.read(filename)
    meshobj.touch()
    FreeCAD.ActiveDocument.recompute()


def importVtkFCResult(filename, resultname, analysis=None, result_name_prefix=None):
    # only fields from vtk are imported if they exactly named as the FreeCAD result properties
    # See _getFreeCADMechResultProperties() in FemVTKTools.cpp for the supported names

    import ObjectsFem
    from . import importToolsFem
    if result_name_prefix is None:
        result_name_prefix = ''
    if analysis:
        analysis_object = analysis

    results_name = result_name_prefix + 'results'
    result_obj = ObjectsFem.makeResultMechanical(FreeCAD.ActiveDocument, results_name)
    Fem.readResult(filename, result_obj.Name)  # readResult always creates a new femmesh named ResultMesh

    # workaround for the DisplacementLengths (They should have been calculated by Fem.readResult)
    if not result_obj.DisplacementLengths:
        result_obj.DisplacementLengths = importToolsFem.calculate_disp_abs(result_obj.DisplacementVectors)
        FreeCAD.Console.PrintMessage('Recalculated DisplacementLengths.\n')

    ''' seems unused at the moment
    filenamebase = '.'.join(filename.split('.')[:-1])  # pattern: filebase_timestamp.vtk
    ts = filenamebase.split('_')[-1]
    try:
        time_step = float(ts)
    except:
        time_step = 0.0
    '''

    if analysis:
        analysis_object.addObject(result_obj)
    result_obj.touch()
    FreeCAD.ActiveDocument.recompute()
