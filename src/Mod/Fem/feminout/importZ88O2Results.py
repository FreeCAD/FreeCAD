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

__title__ = "FreeCAD Z88 Disp Reader"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package importZ88O2Results
#  \ingroup FEM
#  \brief FreeCAD Z88 Disp Reader for FEM workbench

import FreeCAD
import os


Debug = False


# ********* generic FreeCAD import and export methods *********
if open.__module__ == '__builtin__':
    # because we'll redefine open below (Python2)
    pyopen = open
elif open.__module__ == 'io':
    # because we'll redefine open below (Python3)
    pyopen = open


def open(
    filename
):
    "called when freecad opens a file"
    docname = os.path.splitext(os.path.basename(filename))[0]
    insert(filename, docname)


def insert(
    filename,
    docname
):
    "called when freecad wants to import a file"
    try:
        doc = FreeCAD.getDocument(docname)
    except NameError:
        doc = FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc
    import_z88_disp(filename)


# ********* module specific methods *********
def import_z88_disp(
    filename,
    analysis=None,
    result_name_prefix=None
):
    '''insert a FreeCAD FEM mechanical result object in the ActiveDocument
    pure usage:
    import feminout.importZ88O2Results as importZ88O2Results
    disp_file = '/pathtofile/z88o2.txt'
    importZ88O2Results.import_z88_disp(disp_file)

    the z888i1.txt FEMMesh file needs to be in the same directory as z88o2.txt
    # ahh, make a new document first ;-)
    '''
    from . import importZ88Mesh
    from . import importToolsFem
    import ObjectsFem
    if result_name_prefix is None:
        result_name_prefix = ''
    disp_read = read_z88_disp(filename)
    result_mesh_object = None
    if len(disp_read['Nodes']) > 0:
        if analysis:
            analysis_object = analysis

        # read result mesh
        if filename.endswith('z88o2.txt'):
            mesh_file = filename.replace('o2', 'i1')
            mesh_data = importZ88Mesh.read_z88_mesh(mesh_file)
            femmesh = importToolsFem.make_femmesh(mesh_data)
            result_mesh_object = ObjectsFem.makeMeshResult(
                FreeCAD.ActiveDocument,
                'Result_mesh'
            )
            result_mesh_object.FemMesh = femmesh
        else:
            FreeCAD.Console.PrintError('Z88 mesh file z88i1.txt not found!')

        # create result obj
        for result_set in disp_read['Results']:
            results_name = result_name_prefix + 'results'

            res_obj = ObjectsFem.makeResultMechanical(FreeCAD.ActiveDocument, results_name)
            res_obj.Mesh = result_mesh_object
            res_obj = importToolsFem.fill_femresult_mechanical(res_obj, result_set)
            if analysis:
                analysis_object.addObject(res_obj)

        if FreeCAD.GuiUp:
            if analysis:
                import FemGui
                FemGui.setActiveAnalysis(analysis_object)
            FreeCAD.ActiveDocument.recompute()

    else:
        FreeCAD.Console.PrintError(
            'Problem on Z88 result file import. No nodes found in Z88 result file.\n'
        )
    return res_obj


def read_z88_disp(
    z88_disp_input
):
    '''
    read a z88 disp file and extract the nodes and elements
    z88 Displacement output file is z88o2.txt
    works with Z88OS14
    '''
    nodes = {}
    mode_disp = {}
    mode_results = {}
    results = []

    z88_disp_file = pyopen(z88_disp_input, "r")

    for no, line in enumerate(z88_disp_file):
        lno = no + 1
        linelist = line.split()

        if lno >= 6:
            # disp line
            # print(linelist)
            node_no = int(linelist[0])
            mode_disp_x = float(linelist[1])
            mode_disp_y = float(linelist[2])
            if len(linelist) > 3:
                mode_disp_z = float(linelist[3])
            else:
                mode_disp_z = 0.0
            mode_disp[node_no] = FreeCAD.Vector(mode_disp_x, mode_disp_y, mode_disp_z)
            nodes[node_no] = node_no

    mode_results['disp'] = mode_disp
    results.append(mode_results)

    if Debug:
        for r in results[0]['disp']:
            print(r, ' --> ', results[0]['disp'][r])

    z88_disp_file.close()
    return {'Nodes': nodes, 'Results': results}
