# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 - Bernd Hahnebach <bernd@bimstatik.org>            *
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

__title__ = "Fem Tools for results"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## \addtogroup FEM
#  @{

import FreeCAD


## Removes all result objects from an analysis group
#  @param analysis
def purge_results(analysis):
    for m in analysis.Group:
        if (m.isDerivedFrom('Fem::FemResultObject')):
            if m.Mesh and hasattr(m.Mesh, "Proxy") and m.Mesh.Proxy.Type == "Fem::FemMeshResult":
                analysis.Document.removeObject(m.Mesh.Name)
            analysis.Document.removeObject(m.Name)
    FreeCAD.ActiveDocument.recompute()


## Resets result mesh deformation
#  @param result object
def reset_mesh_deformation(resultobj):
    if FreeCAD.GuiUp:
        if resultobj.Mesh:
            resultobj.Mesh.ViewObject.applyDisplacement(0.0)


## Resets result mesh color
#  @param result object
def reset_mesh_color(resultobj):
    if FreeCAD.GuiUp:
        if resultobj.Mesh:
            resultobj.Mesh.ViewObject.NodeColor = {}
            resultobj.Mesh.ViewObject.ElementColor = {}
            node_numbers = resultobj.Mesh.FemMesh.Nodes.keys()
            zero_values = [0] * len(node_numbers)
            resultobj.Mesh.ViewObject.setNodeColorByScalars(node_numbers, zero_values)


def show_displacement(resultobj, displacement_factor=0.0):
    if FreeCAD.GuiUp:
        if resultobj.Mesh.ViewObject.Visibility is False:
            resultobj.Mesh.ViewObject.Visibility = True
        resultobj.Mesh.ViewObject.setNodeDisplacementByVectors(resultobj.NodeNumbers, resultobj.DisplacementVectors)
        resultobj.Mesh.ViewObject.applyDisplacement(displacement_factor)


## Sets mesh color using selected type of results (Sabs by default)
#  @param self The python object self
#  @param result_type Type of FEM result, allowed are:
#  - U1, U2, U3 - deformation
#  - Uabs - absolute deformation
#  - Sabs - Von Mises stress
#  @param limit cutoff value. All values over the limit are treated as equal to the limit. Useful for filtering out hotspots.
def show_result(resultobj, result_type="Sabs", limit=None):
    if result_type == "None":
        reset_mesh_color(resultobj.Mesh)
        return
    if resultobj:
        if result_type == "Sabs":
            values = resultobj.StressValues
        elif result_type == "Uabs":
            values = resultobj.DisplacementLengths
        # TODO: the result object does have more result types to show, implement them
        else:
            match = {"U1": 0, "U2": 1, "U3": 2}
            d = zip(*resultobj.DisplacementVectors)
            values = list(d[match[result_type]])
        show_color_by_scalar_with_cutoff(resultobj, values, limit)
    else:
        print('Error, No result object given.')


## Sets mesh color using list of values. Internally used by show_result function.
#  @param self The python object self
#  @param values list of values
#  @param limit cutoff value. All values over the limit are treated as equal to the limit. Useful for filtering out hotspots.
def show_color_by_scalar_with_cutoff(resultobj, values, limit=None):
    if limit:
        filtered_values = []
        for v in values:
            if v > limit:
                filtered_values.append(limit)
            else:
                filtered_values.append(v)
    else:
        filtered_values = values
    if FreeCAD.GuiUp:
        if resultobj.Mesh.ViewObject.Visibility is False:
            resultobj.Mesh.ViewObject.Visibility = True
        resultobj.Mesh.ViewObject.setNodeColorByScalars(resultobj.NodeNumbers, filtered_values)


## Returns minimum, average and maximum value for provided result type
#  @param result object
#  @param result_type Type of FEM result, allowed are:
#  - U1, U2, U3 - deformation
#  - Uabs - absolute deformation
#  - Sabs - Von Mises stress
#  - Prin1 - Principal stress 1
#  - Prin2 - Principal stress 2
#  - Prin3 - Principal stress 3
#  - MaxSear - maximum shear stress
#  - Peeq - peeq strain
#  - Temp - Temperature
#  - MFlow - MassFlowRate
#  - NPress - NetworkPressure
#  - None - always return (0.0, 0.0, 0.0)
def get_stats(resultobj, result_type):
    m = resultobj
    stats = (0.0, 0.0, 0.0)
    match_table = {
        "U1": (m.Stats[0], m.Stats[1], m.Stats[2]),
        "U2": (m.Stats[3], m.Stats[4], m.Stats[5]),
        "U3": (m.Stats[6], m.Stats[7], m.Stats[8]),
        "Uabs": (m.Stats[9], m.Stats[10], m.Stats[11]),
        "Sabs": (m.Stats[12], m.Stats[13], m.Stats[14]),
        "MaxPrin": (m.Stats[15], m.Stats[16], m.Stats[17]),
        "MidPrin": (m.Stats[18], m.Stats[19], m.Stats[20]),
        "MinPrin": (m.Stats[21], m.Stats[22], m.Stats[23]),
        "MaxShear": (m.Stats[24], m.Stats[25], m.Stats[26]),
        "Peeq": (m.Stats[27], m.Stats[28], m.Stats[29]),
        "Temp": (m.Stats[30], m.Stats[31], m.Stats[32]),
        "MFlow": (m.Stats[33], m.Stats[34], m.Stats[35]),
        "NPress": (m.Stats[36], m.Stats[37], m.Stats[38]),
        "None": (0.0, 0.0, 0.0)
    }
    stats = match_table[result_type]
    return stats


def compact_result(res_obj):
    '''
    compacts result.Mesh and appropriate result.NodeNumbers
    '''
    # as workaround for https://www.freecadweb.org/tracker/view.php?id=2873

    # get compact mesh data
    from femmesh.meshtools import compact_mesh as cm
    compact_femmesh_data = cm(res_obj.Mesh.FemMesh)
    compact_femmesh = compact_femmesh_data[0]
    node_map = compact_femmesh_data[1]
    # elem_map = compact_femmesh_data[2]  # FreeCAD result obj does not support elem results ATM

    # set result mesh
    res_obj.Mesh.FemMesh = compact_femmesh

    # set result node numbers
    new_node_numbers = []
    for old_node_id in res_obj.NodeNumbers:
        new_node_numbers.append(node_map[old_node_id])
    res_obj.NodeNumbers = new_node_numbers

    return res_obj

##  @}
