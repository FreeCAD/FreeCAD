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

__title__ = "FreeCAD FEM import tools"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package importToolsFem
#  \ingroup FEM
#  \brief FreeCAD FEM import tools

import FreeCAD
from math import pow, sqrt
import numpy as np


def get_FemMeshObjectMeshGroups(fem_mesh_obj):
    """
        Get mesh groups from mesh. This also throws no exception if there
        is no Groups property at all (e.g. Netgen meshes).
    """
    fem_mesh = fem_mesh_obj.FemMesh
    try:
        gmshgroups = fem_mesh.Groups
    except:
        gmshgroups = ()

    return gmshgroups


def get_FemMeshObjectOrder(fem_mesh_obj):
    """
        Gets element order. Element order counting based on number of nodes on
        edges. Edge with 2 nodes -> linear elements, Edge with 3 nodes ->
        quadratic elements, and so on. No edges in mesh -> not determined.
        (Is this possible? Seems to be a very degenerate case.)
        If there are edges with different number of nodes appearing, return
        list of orders.
    """
    presumable_order = None

    edges = fem_mesh_obj.FemMesh.Edges

    if edges != ():
        edges_length_set = list({len(fem_mesh_obj.FemMesh.getElementNodes(e)) for e in edges})
        # only need set to eliminate double entries

        if len(edges_length_set) == 1:
            presumable_order = edges_length_set[0] - 1
        else:
            presumable_order = [el - 1 for el in edges_length_set]
    else:
        print("Found no edges in mesh: Element order determination does not work without them.")

    return presumable_order


def get_FemMeshObjectDimension(fem_mesh_obj):
    """ Count all entities in an abstract sense, to distinguish which dimension the mesh is
        (i.e. linemesh, facemesh, volumemesh)
    """
    dim = None

    if fem_mesh_obj.FemMesh.Nodes != ():
        dim = 0
    if fem_mesh_obj.FemMesh.Edges != ():
        dim = 1
    if fem_mesh_obj.FemMesh.Faces != ():
        dim = 2
    if fem_mesh_obj.FemMesh.Volumes != ():
        dim = 3

    return dim


def get_FemMeshObjectElementTypes(fem_mesh_obj, remove_zero_element_entries=True):
    """
        Spit out all elements in the mesh with their appropriate dimension.
    """
    FreeCAD_element_names_dims = {
        "Node": 0, "Edge": 1, "Hexa": 3, "Polygon": 2, "Polyhedron": 3,
        "Prism": 3, "Pyramid": 3, "Quadrangle": 2, "Tetra": 3, "Triangle": 2}

    eval_dict = locals()  # to access local variables from eval
    elements_list_with_zero = [(eval("fem_mesh_obj.FemMesh." + s + "Count", eval_dict), s, d) for (s, d) in FreeCAD_element_names_dims.items()]
    # ugly but necessary
    if remove_zero_element_entries:
        elements_list = [(num, s, d) for (num, s, d) in elements_list_with_zero if num > 0]
    else:
        elements_list = elements_list_with_zero

    return elements_list


def get_MaxDimElementFromList(elem_list):
    """
        Gets element with the maximal dimension in the mesh to determine cells.
    """
    elem_list.sort(key=lambda t: t[2])
    return elem_list[-1]


def make_femmesh(mesh_data):
    ''' makes an FreeCAD FEM Mesh object from FEM Mesh data
    '''
    import Fem
    mesh = Fem.FemMesh()
    m = mesh_data
    if ('Nodes' in m) and (len(m['Nodes']) > 0):
        FreeCAD.Console.PrintLog("Found: nodes\n")
        if (
            ('Seg2Elem' in m)
            or ('Seg3Elem' in m)
            or ('Tria3Elem' in m)
            or ('Tria6Elem' in m)
            or ('Quad4Elem' in m)
            or ('Quad8Elem' in m)
            or ('Tetra4Elem' in m)
            or ('Tetra10Elem' in m)
            or ('Penta6Elem' in m)
            or ('Penta15Elem' in m)
            or ('Hexa8Elem' in m)
            or ('Hexa20Elem' in m)
        ):

            nds = m['Nodes']
            FreeCAD.Console.PrintLog("Found: elements\n")
            for i in nds:
                n = nds[i]
                mesh.addNode(n[0], n[1], n[2], i)
            elms_hexa8 = m['Hexa8Elem']
            for i in elms_hexa8:
                e = elms_hexa8[i]
                mesh.addVolume([e[0], e[1], e[2], e[3], e[4], e[5], e[6], e[7]], i)
            elms_penta6 = m['Penta6Elem']
            for i in elms_penta6:
                e = elms_penta6[i]
                mesh.addVolume([e[0], e[1], e[2], e[3], e[4], e[5]], i)
            elms_tetra4 = m['Tetra4Elem']
            for i in elms_tetra4:
                e = elms_tetra4[i]
                mesh.addVolume([e[0], e[1], e[2], e[3]], i)
            elms_tetra10 = m['Tetra10Elem']
            for i in elms_tetra10:
                e = elms_tetra10[i]
                mesh.addVolume([e[0], e[1], e[2], e[3], e[4], e[5], e[6], e[7], e[8], e[9]], i)
            elms_penta15 = m['Penta15Elem']
            for i in elms_penta15:
                e = elms_penta15[i]
                mesh.addVolume([e[0], e[1], e[2], e[3], e[4], e[5], e[6], e[7], e[8], e[9],
                                e[10], e[11], e[12], e[13], e[14]], i)
            elms_hexa20 = m['Hexa20Elem']
            for i in elms_hexa20:
                e = elms_hexa20[i]
                mesh.addVolume([e[0], e[1], e[2], e[3], e[4], e[5], e[6], e[7], e[8], e[9],
                                e[10], e[11], e[12], e[13], e[14], e[15], e[16], e[17], e[18], e[19]], i)
            elms_tria3 = m['Tria3Elem']
            for i in elms_tria3:
                e = elms_tria3[i]
                mesh.addFace([e[0], e[1], e[2]], i)
            elms_tria6 = m['Tria6Elem']
            for i in elms_tria6:
                e = elms_tria6[i]
                mesh.addFace([e[0], e[1], e[2], e[3], e[4], e[5]], i)
            elms_quad4 = m['Quad4Elem']
            for i in elms_quad4:
                e = elms_quad4[i]
                mesh.addFace([e[0], e[1], e[2], e[3]], i)
            elms_quad8 = m['Quad8Elem']
            for i in elms_quad8:
                e = elms_quad8[i]
                mesh.addFace([e[0], e[1], e[2], e[3], e[4], e[5], e[6], e[7]], i)
            elms_seg2 = m['Seg2Elem']
            for i in elms_seg2:
                e = elms_seg2[i]
                mesh.addEdge([e[0], e[1]], i)
            elms_seg3 = m['Seg3Elem']
            for i in elms_seg3:
                e = elms_seg3[i]
                mesh.addEdge([e[0], e[1], e[2]], i)
            FreeCAD.Console.PrintLog("imported mesh: {} nodes, {} HEXA8, {} PENTA6, {} TETRA4, {} TETRA10, {} PENTA15".format(
                len(nds), len(elms_hexa8), len(elms_penta6), len(elms_tetra4), len(elms_tetra10), len(elms_penta15)
            ))
            FreeCAD.Console.PrintLog("imported mesh: {} HEXA20, {} TRIA3, {} TRIA6, {} QUAD4, {} QUAD8, {} SEG2, {} SEG3".format(
                len(elms_hexa20), len(elms_tria3), len(elms_tria6), len(elms_quad4), len(elms_quad8), len(elms_seg2), len(elms_seg3)
            ))
        else:
            FreeCAD.Console.PrintError("No Elements found!\n")
    else:
        FreeCAD.Console.PrintError("No Nodes found!\n")
    return mesh


def fill_femresult_mechanical(results, result_set):
    ''' fills a FreeCAD FEM mechanical result object with result data
    '''
    if 'number' in result_set:
        eigenmode_number = result_set['number']
    else:
        eigenmode_number = 0
    if 'time' in result_set:
        step_time = result_set['time']
        step_time = round(step_time, 2)

    if 'disp' in result_set:
        disp = result_set['disp']
        displacement = []
        for k, v in disp.items():
            displacement.append(v)

        x_max, y_max, z_max = map(max, zip(*displacement))
        if eigenmode_number > 0:
            span = get_span(results.Mesh.FemMesh.Nodes.items())
            max_disp = max(x_max, y_max, z_max)
            # Allow for max displacement to be 0.1% of the span
            # FIXME - add to Preferences
            max_allowed_disp = 0.001 * span
            scale = max_allowed_disp / max_disp
        else:
            scale = 1.0

        results.DisplacementVectors = list(map((lambda x: x * scale), disp.values()))
        results.NodeNumbers = list(disp.keys())
        results.DisplacementLengths = calculate_disp_abs(displacement)

        if 'stressv' in result_set:
            stressv = result_set['stressv']
            results.StressVectors = list(map((lambda x: x * scale), stressv.values()))

        if 'strainv' in result_set:
            strainv = result_set['strainv']
            results.StrainVectors = list(map((lambda x: x * scale), strainv.values()))

        if 'stress' in result_set:
            stress = result_set['stress']
            if len(stress) > 0:
                mstress = []
                prinstress1 = []
                prinstress2 = []
                prinstress3 = []
                shearstress = []
                for i in stress.values():
                    mstress.append(calculate_von_mises(i))
                    prin1, prin2, prin3, shear = calculate_principal_stress(i)
                    prinstress1.append(prin1)
                    prinstress2.append(prin2)
                    prinstress3.append(prin3)
                    shearstress.append(shear)
                if eigenmode_number > 0:
                    results.StressValues = list(map((lambda x: x * scale), mstress))
                    results.PrincipalMax = list(map((lambda x: x * scale), prinstress1))
                    results.PrincipalMed = list(map((lambda x: x * scale), prinstress2))
                    results.PrincipalMin = list(map((lambda x: x * scale), prinstress3))
                    results.MaxShear = list(map((lambda x: x * scale), shearstress))
                    results.Eigenmode = eigenmode_number
                else:
                    results.StressValues = mstress
                    results.PrincipalMax = prinstress1
                    results.PrincipalMed = prinstress2
                    results.PrincipalMin = prinstress3
                    results.MaxShear = shearstress
            stress_keys = list(stress.keys())
            if (results.NodeNumbers != 0 and results.NodeNumbers != stress_keys):
                print("Inconsistent FEM results: element number for Stress doesn't equal element number for Displacement {} != {}"
                      .format(results.NodeNumbers, len(results.StressValues)))
            results.NodeNumbers = stress_keys

        # Read Equivalent Plastic strain if they exist
        if 'peeq' in result_set:
            Peeq = result_set['peeq']
            if len(Peeq) > 0:
                if len(Peeq.values()) != len(disp.values()):
                    Pe = []
                    Pe_extra_nodes = Peeq.values()
                    nodes = len(disp.values())
                    for i in range(nodes):
                        Pe_value = Pe_extra_nodes[i]
                        Pe.append(Pe_value)
                    results.Peeq = Pe
                else:
                    results.Peeq = Peeq.values()

    # Read temperatures if they exist
    if 'temp' in result_set:
        Temperature = result_set['temp']
        if len(Temperature) > 0:
            if len(Temperature.values()) != len(disp.values()):
                Temp = []
                Temp_extra_nodes = Temperature.values()
                nodes = len(disp.values())
                for i in range(nodes):
                    Temp_value = Temp_extra_nodes[i]
                    Temp.append(Temp_value)
                results.Temperature = list(map((lambda x: x), Temp))
            else:
                results.Temperature = list(map((lambda x: x), Temperature.values()))
            results.Time = step_time

    # read MassFlow
    if 'mflow' in result_set:
        MassFlow = result_set['mflow']
        if len(MassFlow) > 0:
            results.MassFlowRate = list(map((lambda x: x), MassFlow.values()))
            results.Time = step_time
            results.NodeNumbers = list(MassFlow.keys())  # disp does not exist, results.NodeNumbers needs to be set

    # read NetworkPressure, disp does not exist, see MassFlow
    if 'npressure' in result_set:
        NetworkPressure = result_set['npressure']
        if len(NetworkPressure) > 0:
            results.NetworkPressure = list(map((lambda x: x), NetworkPressure.values()))
            results.Time = step_time

    # fill the stats list
    fill_femresult_stats(results)
    return results


def fill_femresult_stats(results):
    '''
    fills a FreeCAD FEM mechanical result object with stats data
    results: FreeCAD FEM result object
    '''
    FreeCAD.Console.PrintLog('Calculate stats list for result obj: ' + results.Name + '\n')
    no_of_values = 1  # to avoid division by zero
    # set stats values to 0, they may not exist in result obj results
    x_min = y_min = z_min = x_max = y_max = z_max = x_avg = y_avg = z_avg = 0
    a_max = a_min = a_avg = s_max = s_min = s_avg = 0
    p1_min = p1_avg = p1_max = p2_min = p2_avg = p2_max = p3_min = p3_avg = p3_max = 0
    ms_min = ms_avg = ms_max = peeq_min = peeq_avg = peeq_max = 0
    temp_min = temp_avg = temp_max = mflow_min = mflow_avg = mflow_max = npress_min = npress_avg = npress_max = 0

    if results.DisplacementVectors:
        no_of_values = len(results.DisplacementVectors)
        x_max, y_max, z_max = map(max, zip(*results.DisplacementVectors))
        x_min, y_min, z_min = map(min, zip(*results.DisplacementVectors))
        sum_list = map(sum, zip(*results.DisplacementVectors))
        x_avg, y_avg, z_avg = [i / no_of_values for i in sum_list]
        a_min = min(results.DisplacementLengths)
        a_avg = sum(results.DisplacementLengths) / no_of_values
        a_max = max(results.DisplacementLengths)
    if results.StressValues:
        s_min = min(results.StressValues)
        s_avg = sum(results.StressValues) / no_of_values
        s_max = max(results.StressValues)
    if results.PrincipalMax:
        p1_min = min(results.PrincipalMax)
        p1_avg = sum(results.PrincipalMax) / no_of_values
        p1_max = max(results.PrincipalMax)
    if results.PrincipalMed:
        p2_min = min(results.PrincipalMed)
        p2_avg = sum(results.PrincipalMed) / no_of_values
        p2_max = max(results.PrincipalMed)
    if results.PrincipalMin:
        p3_min = min(results.PrincipalMin)
        p3_avg = sum(results.PrincipalMin) / no_of_values
        p3_max = max(results.PrincipalMin)
    if results.MaxShear:
        ms_min = min(results.MaxShear)
        ms_avg = sum(results.MaxShear) / no_of_values
        ms_max = max(results.MaxShear)
    if results.Peeq:
        peeq_min = min(results.Peeq)
        peeq_avg = sum(results.Peeq) / no_of_values
        peeq_max = max(results.Peeq)
    if results.Temperature:
        temp_min = min(results.Temperature)
        temp_avg = sum(results.Temperature) / no_of_values
        temp_max = max(results.Temperature)
    if results.MassFlowRate:
        no_of_values = len(results.MassFlowRate)  # DisplacementVectors is empty, no_of_values needs to be set
        mflow_min = min(results.MassFlowRate)
        mflow_avg = sum(results.MassFlowRate) / no_of_values
        mflow_max = max(results.MassFlowRate)
    if results.NetworkPressure:
        npress_min = min(results.NetworkPressure)
        npress_avg = sum(results.NetworkPressure) / no_of_values
        npress_max = max(results.NetworkPressure)

    results.Stats = [x_min, x_avg, x_max,
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
    # - module femresult/resulttools.py
    # - module femtest/testccxtools.py
    # TODO: all stats stuff should be reimplemented, ma be a dictionary would be far more robust than a list

    FreeCAD.Console.PrintLog('Stats list for result obj: ' + results.Name + ' calculated\n')
    return results


# helper
def calculate_von_mises(i):
    # Von mises stress (http://en.wikipedia.org/wiki/Von_Mises_yield_criterion)
    s11 = i[0]
    s22 = i[1]
    s33 = i[2]
    s12 = i[3]
    s23 = i[4]
    s31 = i[5]
    s11s22 = pow(s11 - s22, 2)
    s22s33 = pow(s22 - s33, 2)
    s33s11 = pow(s33 - s11, 2)
    s12s23s31 = 6 * (pow(s12, 2) + pow(s23, 2) + pow(s31, 2))
    vm_stress = sqrt(0.5 * (s11s22 + s22s33 + s33s11 + s12s23s31))
    return vm_stress


def calculate_principal_stress(i):
    sigma = np.array([[i[0], i[3], i[5]],
                      [i[3], i[1], i[4]],
                      [i[5], i[4], i[2]]])  # https://forum.freecadweb.org/viewtopic.php?f=18&t=24637&start=10#p240408

    try:  # it will fail if NaN is inside the array,
        # compute principal stresses
        eigvals = list(np.linalg.eigvalsh(sigma))
        eigvals.sort()
        eigvals.reverse()
        maxshear = (eigvals[0] - eigvals[2]) / 2.0
        return (eigvals[0], eigvals[1], eigvals[2], maxshear)
    except:
        return (float('NaN'), float('NaN'), float('NaN'), float('NaN'))


def calculate_disp_abs(displacements):
    disp_abs = []
    for d in displacements:
        disp_abs.append(sqrt(pow(d[0], 2) + pow(d[1], 2) + pow(d[2], 2)))
    return disp_abs

def get_span(node_items):
    positions = []  # list of node vectors
    for k, v in node_items:
        positions.append(v)
    p_x_max, p_y_max, p_z_max = map(max, zip(*positions))
    p_x_min, p_y_min, p_z_min = map(min, zip(*positions))
    x_span = abs(p_x_max - p_x_min)
    y_span = abs(p_y_max - p_y_min)
    z_span = abs(p_z_max - p_z_min)
    span = max(x_span, y_span, z_span)
    return span
