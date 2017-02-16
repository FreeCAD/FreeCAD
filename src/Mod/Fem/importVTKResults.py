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

import os
import FreeCAD
import Fem


if open.__module__ == '__builtin__':
    pyopen = open  # because we'll redefine open below


def insert(filename, docname):
    "called when freecad wants to import a file"
    try:
        doc = FreeCAD.getDocument(docname)
    except NameError:
        doc = FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc
    importVTK(filename)


def open(filename):
    "called when freecad opens a file"
    docname = os.path.splitext(os.path.basename(filename))[0]
    insert(filename, docname)


def importVTK(filename, analysis=None, result_name_prefix=None):
    if result_name_prefix is None:
        result_name_prefix = ''
    if analysis is None:
        analysis_name = os.path.splitext(os.path.basename(filename))[0]
        import FemAnalysis
        analysis_object = FemAnalysis.makeFemAnalysis('Analysis')
        analysis_object.Label = analysis_name
    else:
        analysis_object = analysis

    # if properties can be added in FemVTKTools importCfdResult(), this file can be used for CFD workbench
    results_name = result_name_prefix + 'results'
    from FemResult import makeFemResult
    result_obj = makeFemResult(results_name)
    # result_obj = FreeCAD.ActiveDocument.addObject('Fem::FemResultObject', results_name)
    Fem.readResult(filename, result_obj.Name)  # readResult always creates a new femmesh named ResultMesh
    analysis_object.Member = analysis_object.Member + [result_obj]
    # FIXME move the ResultMesh in the analysis

    filenamebase = '.'.join(filename.split('.')[:-1])  # pattern: filebase_timestamp.vtk
    ts = filenamebase.split('_')[-1]
    try:
        time_step = float(ts)
    except:
        time_step = 0.0
    # Stats has been setup in C++ function FemVTKTools importCfdResult()


def export(objectslist, filename):
    "called when freecad exports a fem result object"
    if len(objectslist) != 1:
        FreeCAD.Console.PrintError("This exporter can only export one object at once\n")
        return
    obj = objectslist[0]
    if not obj.isDerivedFrom("Fem::FemResultObject"):
        FreeCAD.Console.PrintError("object selcted is not FemResultObject.\n")
        return
    Fem.writeResult(filename, obj)
