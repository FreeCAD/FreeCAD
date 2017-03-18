# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2013 - Joachim Zettler                                  *
# *   Copyright (c) 2013 - Juergen Riegel <FreeCAD@juergen-riegel.net>      *
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

__title__ = "FreeCAD Calculix library"
__author__ = "Juergen Riegel , Michael Hindley, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package importCcxFrdResults
#  \ingroup FEM
#  \brief FreeCAD Calculix FRD Reader for FEM workbench

import FreeCAD
import os


########## generic FreeCAD import and export methods ##########
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
    importFrd(filename)


########## module specific methods ##########
def importFrd(filename, analysis=None, result_name_prefix=None):
    import importToolsFem
    import ObjectsFem
    if result_name_prefix is None:
        result_name_prefix = ''
    m = readResult(filename)
    mesh_object = None
    if(len(m['Nodes']) > 0):
        if analysis is None:
            analysis_name = os.path.splitext(os.path.basename(filename))[0]
            analysis_object = ObjectsFem.makeAnalysis('Analysis')
            analysis_object.Label = analysis_name
        else:
            analysis_object = analysis  # see if statement few lines later, if not analysis -> no FemMesh object is created !

        if 'Nodes' in m:
            positions = []
            for k, v in m['Nodes'].items():
                positions.append(v)
            p_x_max, p_y_max, p_z_max = map(max, zip(*positions))
            p_x_min, p_y_min, p_z_min = map(min, zip(*positions))

            x_span = abs(p_x_max - p_x_min)
            y_span = abs(p_y_max - p_y_min)
            z_span = abs(p_z_max - p_z_min)
            span = max(x_span, y_span, z_span)

        if (not analysis):
            mesh = importToolsFem.make_femmesh(m)

            if len(m['Nodes']) > 0:
                mesh_object = FreeCAD.ActiveDocument.addObject('Fem::FemMeshObject', 'ResultMesh')
                mesh_object.FemMesh = mesh
                analysis_object.Member = analysis_object.Member + [mesh_object]

        number_of_increments = len(m['Results'])
        for result_set in m['Results']:
            eigenmode_number = result_set['number']
            step_time = result_set['time']
            step_time = round(step_time, 2)
            if eigenmode_number > 0:
                results_name = result_name_prefix + 'mode_' + str(eigenmode_number) + '_results'
            elif number_of_increments > 1:
                results_name = result_name_prefix + 'time_' + str(step_time) + '_results'
            else:
                results_name = result_name_prefix + 'results'

            results = ObjectsFem.makeResultMechanical(results_name)
            for m in analysis_object.Member:  # TODO analysis could have multiple mesh objects in the future
                if m.isDerivedFrom("Fem::FemMeshObject"):
                    results.Mesh = m
                    break
            results = importToolsFem.fill_femresult_mechanical(results, result_set, span)
            analysis_object.Member = analysis_object.Member + [results]

        if(FreeCAD.GuiUp):
            import FemGui
            FemGui.setActiveAnalysis(analysis_object)


# read a calculix result file and extract the nodes, displacement vectores and stress values.
def readResult(frd_input):
    inout_nodes_exist = False
    if os.path.exists("inout_nodes.txt"):
        inout_nodes = []
        f = pyopen("inout_nodes.txt", "r")
        lines = f.readlines()
        for line in lines:
            a = line.split(',')
            inout_nodes.append(a)
        if len(inout_nodes) > 0:
            inout_nodes_exist = True
        f.close()
        os.remove("inout_nodes.txt")
    frd_file = pyopen(frd_input, "r")
    nodes = {}
    elements_hexa8 = {}
    elements_penta6 = {}
    elements_tetra4 = {}
    elements_tetra10 = {}
    elements_penta15 = {}
    elements_hexa20 = {}
    elements_tria3 = {}
    elements_tria6 = {}
    elements_quad4 = {}
    elements_quad8 = {}
    elements_seg2 = {}
    elements_seg3 = {}
    results = []
    mode_results = {}
    mode_disp = {}
    mode_stress = {}
    mode_stressv = {}
    mode_strain = {}
    mode_peeq = {}
    mode_temp = {}
    mode_massflow = {}
    mode_networkpressure = {}

    mode_disp_found = False
    nodes_found = False
    mode_stress_found = False
    mode_strain_found = False
    mode_peeq_found = False
    mode_temp_found = False
    mode_massflow_found = False
    mode_networkpressure_found = False
    mode_time_found = False
    elements_found = False
    input_continues = False
    eigenmode = 0
    elem = -1
    elemType = 0
    timestep = 0
    timetemp = 0

    for line in frd_file:
        # Check if we found nodes section
        if line[4:6] == "2C":
            nodes_found = True
        # first lets extract the node and coordinate information from the results file
        if nodes_found and (line[1:3] == "-1"):
            elem = int(line[4:13])
            nodes_x = float(line[13:25])
            nodes_y = float(line[25:37])
            nodes_z = float(line[37:49])
            nodes[elem] = FreeCAD.Vector(nodes_x, nodes_y, nodes_z)
        # Check if we found nodes section
        if line[4:6] == "3C":
            elements_found = True
        # first lets extract element number
        if elements_found and (line[1:3] == "-1"):
            elem = int(line[4:13])
            elemType = int(line[14:18])
        # then import elements
        if elements_found and (line[1:3] == "-2"):
            # node order fits with node order in writeAbaqus() in FemMesh.cpp
            if elemType == 1:
                # C3D8 CalculiX --> hexa8 FreeCAD
                # N6, N7, N8, N5, N2, N3, N4, N1
                nd1 = int(line[3:13])
                nd2 = int(line[13:23])
                nd3 = int(line[23:33])
                nd4 = int(line[33:43])
                nd5 = int(line[43:53])
                nd6 = int(line[53:63])
                nd7 = int(line[63:73])
                nd8 = int(line[73:83])
                elements_hexa8[elem] = (nd6, nd7, nd8, nd5, nd2, nd3, nd4, nd1)
            elif elemType == 2:
                # C3D6 Calculix --> penta6 FreeCAD
                # N5, N6, N4, N2, N3, N1
                nd1 = int(line[3:13])
                nd2 = int(line[13:23])
                nd3 = int(line[23:33])
                nd4 = int(line[33:43])
                nd5 = int(line[43:53])
                nd6 = int(line[53:63])
                elements_penta6[elem] = (nd5, nd6, nd4, nd2, nd3, nd1)
            elif elemType == 3:
                # C3D4 Calculix --> tetra4 FreeCAD
                # N2, N1, N3, N4
                nd1 = int(line[3:13])
                nd2 = int(line[13:23])
                nd3 = int(line[23:33])
                nd4 = int(line[33:43])
                elements_tetra4[elem] = (nd2, nd1, nd3, nd4)
            elif elemType == 4 and input_continues is False:
                # first line
                # C3D20 Calculix --> hexa20 FreeCAD
                # N6, N7, N8, N5, N2, N3, N4, N1, N14, N15, N16, N13, N10, N11, N12, N9, N18, N19, N20, N17
                nd1 = int(line[3:13])
                nd2 = int(line[13:23])
                nd3 = int(line[23:33])
                nd4 = int(line[33:43])
                nd5 = int(line[43:53])
                nd6 = int(line[53:63])
                nd7 = int(line[63:73])
                nd8 = int(line[73:83])
                nd9 = int(line[83:93])
                nd10 = int(line[93:103])
                input_continues = True
            elif elemType == 4 and input_continues is True:
                # second line
                nd11 = int(line[3:13])
                nd12 = int(line[13:23])
                nd13 = int(line[23:33])
                nd14 = int(line[33:43])
                nd15 = int(line[43:53])
                nd16 = int(line[53:63])
                nd17 = int(line[63:73])
                nd18 = int(line[73:83])
                nd19 = int(line[83:93])
                nd20 = int(line[93:103])
                input_continues = False
                # CalculiX uses a different node order in input file *.inp and result file *.frd for hexa20 (C3D20)
                # according to Guido (the developer of ccx)
                # ccx (and thus the *.inp) follows the ABAQUS convention (documented in the ccx-documentation)
                # cgx (and thus the *.frd) follows the FAM2 convention (documented in the cgx-documentation)
                # FAM32 is from the company FEGS limited, maybe this company does not exist any more)
                # elements_hexa20[elem] = (nd6, nd7, nd8, nd5, nd2, nd3, nd4, nd1, nd14, nd15,
                #                          nd16, nd13, nd10, nd11, nd12, nd9, nd18, nd19, nd20, nd17)
                # elements_hexa20[elem] = (nd6, nd7, nd8, nd5, nd2, nd3, nd4, nd1, nd14, nd15,
                #                          nd16, nd13, nd18, nd19, nd20, nd17, nd10, nd11, nd12, nd9)
                # hexa20 import works with the following frd file node assignment
                elements_hexa20[elem] = (nd8, nd5, nd6, nd7, nd4, nd1, nd2, nd3, nd20, nd17,
                                         nd18, nd19, nd12, nd9, nd10, nd11, nd16, nd13, nd14, nd15)
                # print elements_hexa20[elem]
            elif elemType == 5 and input_continues is False:
                # first line
                # C3D15 Calculix --> penta15 FreeCAD
                # N5, N6, N4, N2, N3, N1, N11, N12, N10, N8, N9, N7, N14, N15, N13
                nd1 = int(line[3:13])
                nd2 = int(line[13:23])
                nd3 = int(line[23:33])
                nd4 = int(line[33:43])
                nd5 = int(line[43:53])
                nd6 = int(line[53:63])
                nd7 = int(line[63:73])
                nd8 = int(line[73:83])
                nd9 = int(line[83:93])
                nd10 = int(line[93:103])
                input_continues = True
            elif elemType == 5 and input_continues is True:
                # second line
                nd11 = int(line[3:13])
                nd12 = int(line[13:23])
                nd13 = int(line[23:33])
                nd14 = int(line[33:43])
                nd15 = int(line[43:53])
                input_continues = False
                # CalculiX uses a different node order in input file *.inp and result file *.frd for penta15 (C3D15)
                # elements_penta15[elem] = (nd5, nd6, nd4, nd2, nd3, nd1, nd11, nd12, nd10, nd8,
                #                           nd9, nd7, nd14, nd15, nd13)  # order of the *.inp file
                elements_penta15[elem] = (nd5, nd6, nd4, nd2, nd3, nd1, nd14, nd15, nd13, nd8,
                                          nd9, nd7, nd11, nd12, nd10)
            elif elemType == 6:
                # C3D10 Calculix --> tetra10 FreeCAD
                # N2, N1, N3, N4, N5, N7, N6, N9, N8, N10
                nd1 = int(line[3:13])
                nd2 = int(line[13:23])
                nd3 = int(line[23:33])
                nd4 = int(line[33:43])
                nd5 = int(line[43:53])
                nd6 = int(line[53:63])
                nd7 = int(line[63:73])
                nd8 = int(line[73:83])
                nd9 = int(line[83:93])
                nd10 = int(line[93:103])
                elements_tetra10[elem] = (nd2, nd1, nd3, nd4, nd5, nd7, nd6, nd9, nd8, nd10)
            elif elemType == 7:
                # S3 Calculix --> tria3 FreeCAD
                # N1, N2, N3
                nd1 = int(line[3:13])
                nd2 = int(line[13:23])
                nd3 = int(line[23:33])
                elements_tria3[elem] = (nd1, nd2, nd3)
            elif elemType == 8:
                # S6 CalculiX --> tria6 FreeCAD
                # N1, N2, N3, N4, N5, N6
                nd1 = int(line[3:13])
                nd2 = int(line[13:23])
                nd3 = int(line[23:33])
                nd4 = int(line[33:43])
                nd5 = int(line[43:53])
                nd6 = int(line[53:63])
                elements_tria6[elem] = (nd1, nd2, nd3, nd4, nd5, nd6)
            elif elemType == 9:
                # S4 CalculiX --> quad4 FreeCAD
                # N1, N2, N3, N4
                nd1 = int(line[3:13])
                nd2 = int(line[13:23])
                nd3 = int(line[23:33])
                nd4 = int(line[33:43])
                elements_quad4[elem] = (nd1, nd2, nd3, nd4)
            elif elemType == 10:
                # S8 CalculiX --> quad8 FreeCAD
                # N1, N2, N3, N4, N5, N6, N7, N8
                nd1 = int(line[3:13])
                nd2 = int(line[13:23])
                nd3 = int(line[23:33])
                nd4 = int(line[33:43])
                nd5 = int(line[43:53])
                nd6 = int(line[53:63])
                nd7 = int(line[63:73])
                nd8 = int(line[73:83])
                elements_quad8[elem] = (nd1, nd2, nd3, nd4, nd5, nd6, nd7, nd8)
            elif elemType == 11:
                # B31 CalculiX --> seg2 FreeCAD
                # N1, N2
                nd1 = int(line[3:13])
                nd2 = int(line[13:23])
                elements_seg2[elem] = (nd1, nd2)
            elif elemType == 12:
                # B32 CalculiX --> seg3 FreeCAD
                # Also D element element number
                # N1, N3 ,N2 Order in outpufile is 1,3,2
                nd1 = int(line[3:13])
                nd3 = int(line[13:23])
                nd2 = int(line[23:33])
                if inout_nodes_exist:
                    for i in range(len(inout_nodes)):
                        if nd1 == int(inout_nodes[i][1]):
                            elements_seg3[elem] = (int(inout_nodes[i][2]), nd3, nd1)  # fluid inlet node numbering
                        elif nd3 == int(inout_nodes[i][1]):
                            elements_seg3[elem] = (nd1, int(inout_nodes[i][2]), nd3)  # fluid outlet node numbering
                else:
                    elements_seg3[elem] = (nd1, nd2, nd3)  # normal node numbering for D, B32 elements

        # Check if we found new eigenmode
        if line[5:10] == "PMODE":
            eigenmode = int(line[30:36])
        # Check if we found displacement section
        if line[5:9] == "DISP":
            mode_disp_found = True
        # we found a displacement line in the frd file
        if mode_disp_found and (line[1:3] == "-1"):
            elem = int(line[4:13])
            mode_disp_x = float(line[13:25])
            mode_disp_y = float(line[25:37])
            mode_disp_z = float(line[37:49])
            mode_disp[elem] = FreeCAD.Vector(mode_disp_x, mode_disp_y, mode_disp_z)
        if line[5:11] == "STRESS":
            mode_stress_found = True
        # we found a stress line in the frd file
        if mode_stress_found and (line[1:3] == "-1"):
            elem = int(line[4:13])
            stress_1 = float(line[13:25])
            stress_2 = float(line[25:37])
            stress_3 = float(line[37:49])
            stress_4 = float(line[49:61])
            stress_5 = float(line[61:73])
            stress_6 = float(line[73:85])
            mode_stress[elem] = (stress_1, stress_2, stress_3, stress_4, stress_5, stress_6)
            mode_stressv[elem] = FreeCAD.Vector(stress_1, stress_2, stress_3)
        if line[5:13] == "TOSTRAIN":
            mode_strain_found = True
        # we found a strain line in the frd file
        if mode_strain_found and (line[1:3] == "-1"):
            elem = int(line[4:13])
            strain_1 = float(line[13:25])
            strain_2 = float(line[25:37])
            strain_3 = float(line[37:49])
#            strain_4 = float(line[49:61])  #Not used in vector
#            strain_5 = float(line[61:73])
#            strain_6 = float(line[73:85])
            mode_strain[elem] = FreeCAD.Vector(strain_1, strain_2, strain_3)

        if line[5:7] == "PE":
            mode_peeq_found = True
        # we found an equivalent plastic strain line in the frd file
        if mode_peeq_found and (line[1:3] == "-1"):
            elem = int(line[4:13])
            peeq = float(line[13:25])
            mode_peeq[elem] = (peeq)
        # Check if we found a time step
        if line[4:10] == "1PSTEP":
            mode_time_found = True
        if mode_time_found and (line[2:7] == "100CL"):
            timetemp = float(line[13:25])
            if timetemp > timestep:
                timestep = timetemp
        if line[5:11] == "NDTEMP":
            mode_temp_found = True
        # we found a temperatures line in the frd file
        if mode_temp_found and (line[1:3] == "-1"):
            elem = int(line[4:13])
            temperature = float(line[13:25])
            mode_temp[elem] = (temperature)
        if line[5:11] == "MAFLOW":
            mode_massflow_found = True
        # we found a mass flow line in the frd file
        if mode_massflow_found and (line[1:3] == "-1"):
            elem = int(line[4:13])
            massflow = float(line[13:25])
            mode_massflow[elem] = (massflow * 1000)  # convert units to kg/s from t/s
            if inout_nodes_exist:
                for i in range(len(inout_nodes)):
                    if elem == int(inout_nodes[i][1]):
                        node = int(inout_nodes[i][2])
                        mode_massflow[node] = (massflow * 1000)  # convert units to kg/s from t/s
        if line[5:11] == "STPRES":
            mode_networkpressure_found = True
        # we found a network pressure line in the frd file
        if mode_networkpressure_found and (line[1:3] == "-1"):
            elem = int(line[4:13])
            networkpressure = float(line[13:25])
            mode_networkpressure[elem] = (networkpressure)
            if inout_nodes_exist:
                for i in range(len(inout_nodes)):
                    if elem == int(inout_nodes[i][1]):
                        node = int(inout_nodes[i][2])
                        mode_networkpressure[node] = (networkpressure)
        # Check for the end of a section
        if line[1:3] == "-3":
            if mode_disp_found:
                mode_disp_found = False

            if mode_stress_found:
                mode_stress_found = False

            if mode_strain_found:
                mode_strain_found = False

            if mode_peeq_found:
                mode_peeq_found = False

            if mode_temp_found:
                mode_temp_found = False

            if mode_time_found:
                mode_time_found = False

            if mode_massflow_found:
                mode_massflow_found = False

            if mode_networkpressure_found:
                mode_networkpressure_found = False

            if mode_disp and mode_stress and mode_temp:
                mode_results = {}
                mode_results['number'] = eigenmode
                mode_results['disp'] = mode_disp
                mode_results['stress'] = mode_stress
                mode_results['stressv'] = mode_stressv
                mode_results['strainv'] = mode_strain
                mode_results['peeq'] = mode_peeq
                mode_results['temp'] = mode_temp
                mode_results['time'] = timestep
                results.append(mode_results)
                mode_disp = {}
                mode_stress = {}
                mode_stressv = {}
                mode_strain = {}
                mode_peeq = {}
                mode_temp = {}
                eigenmode = 0

            if mode_disp and mode_stress:
                mode_results = {}
                mode_results['number'] = eigenmode
                mode_results['disp'] = mode_disp
                mode_results['stress'] = mode_stress
                mode_results['stressv'] = mode_stressv
                mode_results['strainv'] = mode_strain
                mode_results['peeq'] = mode_peeq
                mode_results['time'] = 0  # Don't return time if static
                results.append(mode_results)
                mode_disp = {}
                mode_stress = {}
                mode_stressv = {}
                mode_strain = {}
                mode_peeq = {}
                eigenmode = 0

            if mode_massflow and mode_networkpressure:
                mode_results = {}
                mode_results['number'] = eigenmode
                mode_results['mflow'] = mode_massflow
                mode_results['npressure'] = mode_networkpressure
                mode_results['time'] = timestep
                results.append(mode_results)
                mode_massflow = {}
                mode_networkpressure = {}
                eigenmode = 0
            nodes_found = False
            elements_found = False

    frd_file.close()
    if not nodes:
        FreeCAD.Console.PrintError('FEM: No nodes found in Frd file.\n')
    return {'Nodes': nodes,
            'Hexa8Elem': elements_hexa8, 'Penta6Elem': elements_penta6, 'Tetra4Elem': elements_tetra4, 'Tetra10Elem': elements_tetra10,
            'Penta15Elem': elements_penta15, 'Hexa20Elem': elements_hexa20, 'Tria3Elem': elements_tria3, 'Tria6Elem': elements_tria6,
            'Quad4Elem': elements_quad4, 'Quad8Elem': elements_quad8, 'Seg2Elem': elements_seg2, 'Seg3Elem': elements_seg3,
            'Results': results}
