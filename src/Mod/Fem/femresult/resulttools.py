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
from math import sqrt


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
#  - see def get_all_stats() for dict keys description
#  - None - always return (0.0, 0.0, 0.0)
def get_stats(res_obj, result_type):
    match_table = get_all_stats(res_obj)
    match_table["None"] = (0.0, 0.0, 0.0)
    stats = (0.0, 0.0, 0.0)
    stats = match_table[result_type]
    return stats


## Returns all stats for provided result type
#  @param result object
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
def get_all_stats(res_obj):
    m = res_obj.Stats
    stats_dict = {
        "U1": (m[0], m[1], m[2]),
        "U2": (m[3], m[4], m[5]),
        "U3": (m[6], m[7], m[8]),
        "Uabs": (m[9], m[10], m[11]),
        "Sabs": (m[12], m[13], m[14]),
        "MaxPrin": (m[15], m[16], m[17]),
        "MidPrin": (m[18], m[19], m[20]),
        "MinPrin": (m[21], m[22], m[23]),
        "MaxShear": (m[24], m[25], m[26]),
        "Peeq": (m[27], m[28], m[29]),
        "Temp": (m[30], m[31], m[32]),
        "MFlow": (m[33], m[34], m[35]),
        "NPress": (m[36], m[37], m[38])
    }
    return stats_dict


def fill_femresult_stats(res_obj):
    '''
    fills a FreeCAD FEM mechanical result object with stats data
    res_obj: FreeCAD FEM result object
    '''
    FreeCAD.Console.PrintLog('Calculate stats list for result obj: ' + res_obj.Name + '\n')
    no_of_values = 1  # to avoid division by zero
    # set stats values to 0, they may not exist in res_obj
    x_min = y_min = z_min = x_max = y_max = z_max = x_avg = y_avg = z_avg = 0
    a_max = a_min = a_avg = s_max = s_min = s_avg = 0
    p1_min = p1_avg = p1_max = p2_min = p2_avg = p2_max = p3_min = p3_avg = p3_max = 0
    ms_min = ms_avg = ms_max = peeq_min = peeq_avg = peeq_max = 0
    temp_min = temp_avg = temp_max = mflow_min = mflow_avg = mflow_max = npress_min = npress_avg = npress_max = 0

    if res_obj.DisplacementVectors:
        no_of_values = len(res_obj.DisplacementVectors)
        x_max, y_max, z_max = map(max, zip(*res_obj.DisplacementVectors))
        x_min, y_min, z_min = map(min, zip(*res_obj.DisplacementVectors))
        sum_list = map(sum, zip(*res_obj.DisplacementVectors))
        x_avg, y_avg, z_avg = [i / no_of_values for i in sum_list]
        a_min = min(res_obj.DisplacementLengths)
        a_avg = sum(res_obj.DisplacementLengths) / no_of_values
        a_max = max(res_obj.DisplacementLengths)
    if res_obj.StressValues:
        s_min = min(res_obj.StressValues)
        s_avg = sum(res_obj.StressValues) / no_of_values
        s_max = max(res_obj.StressValues)
    if res_obj.PrincipalMax:
        p1_min = min(res_obj.PrincipalMax)
        p1_avg = sum(res_obj.PrincipalMax) / no_of_values
        p1_max = max(res_obj.PrincipalMax)
    if res_obj.PrincipalMed:
        p2_min = min(res_obj.PrincipalMed)
        p2_avg = sum(res_obj.PrincipalMed) / no_of_values
        p2_max = max(res_obj.PrincipalMed)
    if res_obj.PrincipalMin:
        p3_min = min(res_obj.PrincipalMin)
        p3_avg = sum(res_obj.PrincipalMin) / no_of_values
        p3_max = max(res_obj.PrincipalMin)
    if res_obj.MaxShear:
        ms_min = min(res_obj.MaxShear)
        ms_avg = sum(res_obj.MaxShear) / no_of_values
        ms_max = max(res_obj.MaxShear)
    if res_obj.Peeq:
        peeq_min = min(res_obj.Peeq)
        peeq_avg = sum(res_obj.Peeq) / no_of_values
        peeq_max = max(res_obj.Peeq)
    if res_obj.Temperature:
        temp_min = min(res_obj.Temperature)
        temp_avg = sum(res_obj.Temperature) / no_of_values
        temp_max = max(res_obj.Temperature)
    if res_obj.MassFlowRate:
        no_of_values = len(res_obj.MassFlowRate)  # DisplacementVectors is empty, no_of_values needs to be set
        mflow_min = min(res_obj.MassFlowRate)
        mflow_avg = sum(res_obj.MassFlowRate) / no_of_values
        mflow_max = max(res_obj.MassFlowRate)
    if res_obj.NetworkPressure:
        npress_min = min(res_obj.NetworkPressure)
        npress_avg = sum(res_obj.NetworkPressure) / no_of_values
        npress_max = max(res_obj.NetworkPressure)

    res_obj.Stats = [x_min, x_avg, x_max,
                     y_min, y_avg, y_max,
                     z_min, z_avg, z_max,
                     a_min, a_avg, a_max,
                     s_min, s_avg, s_max,
                     p1_min, p1_avg, p1_max,
                     p2_min, p2_avg, p2_max,
                     p3_min, p3_avg, p3_max,
                     ms_min, ms_avg, ms_max,
                     peeq_min, peeq_avg, peeq_max,
                     temp_min, temp_avg, temp_max,
                     mflow_min, mflow_avg, mflow_max,
                     npress_min, npress_avg, npress_max]
    # stat_types = ["U1", "U2", "U3", "Uabs", "Sabs", "MaxPrin", "MidPrin", "MinPrin", "MaxShear", "Peeq", "Temp", "MFlow", "NPress"]
    # len(stat_types) == 13*3 == 39
    # do not forget to adapt initialization of all Stats items in modules:
    # - module femobjects/_FemResultMechanical.py
    # do not forget to adapt the def get_stats in:
    # - get_stats in module femresult/resulttools.py
    # - module femtest/testccxtools.py
    # TODO: all stats stuff should be reimplemented, maybe a dictionary would be far more robust than a list

    FreeCAD.Console.PrintLog('Stats list for result obj: ' + res_obj.Name + ' calculated\n')
    return res_obj


def add_disp_apps(res_obj):
    res_obj.DisplacementLengths = calculate_disp_abs(res_obj.DisplacementVectors)
    FreeCAD.Console.PrintMessage('Added DisplacementLengths.\n')
    return res_obj


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


def calculate_disp_abs(displacements):
    disp_abs = []
    for d in displacements:
        disp_abs.append(sqrt(pow(d[0], 2) + pow(d[1], 2) + pow(d[2], 2)))
    return disp_abs

##  @}
