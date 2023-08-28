# ***************************************************************************
# *   Copyright (c) 2013 Joachim Zettler                                    *
# *   Copyright (c) 2013 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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

__title__ = "Result import for Calculix frd file format"
__author__ = "Juergen Riegel , Michael Hindley, Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package importCcxFrdResults
#  \ingroup FEM
#  \brief FreeCAD Calculix FRD Reader for FEM workbench

import os

import FreeCAD
from FreeCAD import Console


# ********* generic FreeCAD import and export methods *********
pyopen = open


def open(filename):
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
    importFrd(filename)


# ********* module specific methods *********
def importFrd(
    filename,
    analysis=None,
    result_name_prefix="",
    result_analysis_type=""
):
    import ObjectsFem
    from . import importToolsFem

    if analysis:
        doc = analysis.Document
    else:
        doc = FreeCAD.ActiveDocument

    m = read_frd_result(filename)
    result_mesh_object = None
    res_obj = None

    if len(m["Nodes"]) > 0:
        mesh = importToolsFem.make_femmesh(m)
        res_mesh_is_compacted = False
        nodenumbers_for_compacted_mesh = []

        number_of_increments = len(m["Results"])
        Console.PrintLog(
            "Increments: " + str(number_of_increments) + "\n"
        )
        if len(m["Results"]) > 0:
            for result_set in m["Results"]:
                if "number" in result_set:
                    eigenmode_number = result_set["number"]
                else:
                    eigenmode_number = 0
                step_time = result_set["time"]
                step_time = round(step_time, 2)
                if eigenmode_number > 0:
                    results_name = (
                        "{}EigenMode_{}_Results"
                        .format(result_name_prefix, eigenmode_number)
                    )
                elif number_of_increments > 1:
                    if result_analysis_type == "buckling":
                        results_name = (
                            "{}BucklingFactor_{}_Results"
                            .format(result_name_prefix, step_time)
                        )
                    else:
                        results_name = (
                            "{}Time_{}_Results"
                            .format(result_name_prefix, step_time)
                        )
                else:
                    results_name = (
                        "{}Results"
                        .format(result_name_prefix)
                    )

                res_obj = ObjectsFem.makeResultMechanical(doc, results_name)
                # create result mesh
                result_mesh_object = ObjectsFem.makeMeshResult(doc, results_name + "_Mesh")
                result_mesh_object.FemMesh = mesh
                res_obj.Mesh = result_mesh_object
                res_obj = importToolsFem.fill_femresult_mechanical(res_obj, result_set)
                if analysis:
                    # need to be here, becasause later on, the analysis objs are needed
                    # see fill of principal stresses
                    analysis.addObject(res_obj)

                # more result object calculations
                from femresult import resulttools
                from femtools import femutils
                if not res_obj.MassFlowRate:
                    # information 1:
                    # only compact result if not Flow 1D results
                    # compact result object, workaround for bug 2873
                    # https://www.freecad.org/tracker/view.php?id=2873
                    # information 2:
                    # if the result data has multiple result sets there will be multiple result objs
                    # they all will use one mesh obj
                    # on the first res obj fill: the mesh obj will be compacted, thus
                    # it does not need to be compacted on further result sets
                    # but NodeNumbers need to be compacted for every result set (res object fill)
                    # example frd file: https://forum.freecad.org/viewtopic.php?t=32649#p274291
                    if res_mesh_is_compacted is False:
                        # first result set, compact FemMesh and NodeNumbers
                        res_obj = resulttools.compact_result(res_obj)
                        res_mesh_is_compacted = True
                        nodenumbers_for_compacted_mesh = res_obj.NodeNumbers
                    else:
                        # all other result sets, do not compact FemMesh, only set NodeNumbers
                        res_obj.NodeNumbers = nodenumbers_for_compacted_mesh

                # fill DisplacementLengths
                res_obj = resulttools.add_disp_apps(res_obj)
                # fill vonMises
                res_obj = resulttools.add_von_mises(res_obj)
                # fill principal stress
                # if material reinforced object use add additional values to the res_obj
                if res_obj.getParentGroup():
                    has_reinforced_mat = False
                    for obj in res_obj.getParentGroup().Group:
                        if femutils.is_of_type(obj, "Fem::MaterialReinforced"):
                            has_reinforced_mat = True
                            Console.PrintLog(
                                "Reinforced material object detected, "
                                "reinforced principal stresses and standard principal "
                                "stresses will be added.\n"
                            )
                            resulttools.add_principal_stress_reinforced(res_obj)
                            break
                    if has_reinforced_mat is False:
                        Console.PrintLog(
                            "No reinforced material object detected, "
                            "standard principal stresses will be added.\n"
                        )
                        # fill PrincipalMax, PrincipalMed, PrincipalMin, MaxShear
                        res_obj = resulttools.add_principal_stress_std(res_obj)
                else:
                    Console.PrintLog(
                        "No Analysis detected, standard principal stresses will be added.\n"
                    )
                    # if a pure frd file was opened no analysis and thus no parent group
                    # fill PrincipalMax, PrincipalMed, PrincipalMin, MaxShear
                    res_obj = resulttools.add_principal_stress_std(res_obj)
                # fill Stats
                res_obj = resulttools.fill_femresult_stats(res_obj)

                # create a results pipeline if not already existing
                pipeline_name = "Pipeline_" + results_name
                pipeline_obj = doc.getObject(pipeline_name)
                if pipeline_obj is None:
                    pipeline_obj = ObjectsFem.makePostVtkResult(doc, res_obj, results_name)
                    pipeline_visibility = True
                    if analysis:
                        analysis.addObject(pipeline_obj)
                else:
                    if FreeCAD.GuiUp:
                        # store pipeline visibility because pipeline_obj.load makes the
                        # pipeline always visible
                        pipeline_visibility = pipeline_obj.ViewObject.Visibility
                    pipeline_obj.load(res_obj)
                # update the pipeline
                pipeline_obj.recomputeChildren()
                pipeline_obj.recompute()
                if FreeCAD.GuiUp:
                    pipeline_obj.ViewObject.updateColorBars()
                    # make results mesh invisible, will be made visible
                    # later in task_solver_ccxtools.py
                    res_obj.Mesh.ViewObject.Visibility = False
                    # restore pipeline visibility
                    pipeline_obj.ViewObject.Visibility = pipeline_visibility

        else:
            error_message = (
                "Nodes, but no results found in frd file. "
                "It means there only is a mesh but no results in frd file. "
                "Usually this happens for: \n"
                "- analysis type 'NOANALYSIS'\n"
                "- if CalculiX returned no results "
                "(happens on nonpositive jacobian determinant in at least one element)\n"
                "- just no frd results where requestet in input file "
                "(neither 'node file' nor 'el file' in output section')\n"
            )
            Console.PrintWarning(error_message)

        # create a result obj, even if we have no results but a result mesh in frd file
        # see error message above for more information
        if not res_obj:
            if result_name_prefix:
                results_name = "{}_Results".format(result_name_prefix)
            else:
                results_name = "Results"
            res_obj = ObjectsFem.makeResultMechanical(doc, results_name)
            res_obj.Mesh = result_mesh_object
            # TODO, node numbers in result obj could be set
            if analysis:
                analysis.addObject(res_obj)

        if FreeCAD.GuiUp:
            if analysis:
                import FemGui
                FemGui.setActiveAnalysis(analysis)
            doc.recompute()

    else:
        Console.PrintError(
            "Problem on frd file import. No nodes found in frd file.\n"
        )
        # None will be returned
        # or would it be better to raise an exception if there are not even nodes in frd file?

    return res_obj


# read a calculix result file and extract the nodes
# displacement vectors and stress values.
def read_frd_result(
    frd_input
):
    Console.PrintMessage(
        "Read ccx results from frd file: {}\n"
        .format(frd_input)
    )
    inout_nodes = []
    inout_nodes_file = frd_input.rsplit(".", 1)[0] + "_inout_nodes.txt"
    if os.path.exists(inout_nodes_file):
        Console.PrintMessage(
            "Read special 1DFlow nodes data form: {}\n".format(inout_nodes_file)
        )
        f = pyopen(inout_nodes_file, "r")
        lines = f.readlines()
        for line in lines:
            a = line.split(",")
            inout_nodes.append(a)
        f.close()
        Console.PrintMessage("{}\n".format(inout_nodes))
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
    mode_results["number"] = float("NaN")
    mode_results["time"] = float("NaN")
    mode_disp = {}
    mode_stress = {}
    mode_strain = {}
    mode_peeq = {}
    mode_temp = {}
    mode_massflow = {}
    mode_networkpressure = {}

    nodes_found = False
    elements_found = False
    mode_time_found = False
    mode_disp_found = False
    mode_stress_found = False
    mode_strain_found = False
    mode_peeq_found = False
    mode_temp_found = False
    mode_massflow_found = False
    mode_networkpressure_found = False
    end_of_section_found = False
    end_of_frd_data_found = False
    input_continues = False
    mode_eigen_changed = False
    mode_time_changed = False

    eigenmode = 0
    eigentemp = 0
    elem = -1
    elemType = 0
    timestep = 0
    timetemp = 0

    for line in frd_file:

        # Check if we found nodes section
        if line[4:6] == "2C":
            nodes_found = True
        if nodes_found and (line[1:3] == "-1"):
            # we found a nodes line, lets extract the node and coordinate data
            elem = int(line[4:13])
            nodes_x = float(line[13:25])
            nodes_y = float(line[25:37])
            nodes_z = float(line[37:49])
            nodes[elem] = FreeCAD.Vector(nodes_x, nodes_y, nodes_z)

        # Check if we found elements section
        if line[4:6] == "3C":
            elements_found = True
        if elements_found and (line[1:3] == "-1"):
            # we found a first element line, lets extract element number
            elem = int(line[4:13])
            elemType = int(line[14:18])
        if elements_found and (line[1:3] == "-2"):
            # we found a second element line, lets extract the elements
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
                # N6, N7, N8, N5, N2, N3, N4, N1, N14, N15
                # N16, N13, N10, N11, N12, N9, N18, N19, N20, N17
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
                """
                CalculiX uses a different node order in
                input file *.inp and result file *.frd for hexa20 (C3D20)
                according to Guido (the developer of ccx):
                see note in the first line of cgx manual part element types
                ccx (and thus the *.inp) follows the ABAQUS convention
                documented in the ccx-documentation
                cgx (and thus the *.frd) follows the FAM2 convention
                documented in the cgx-documentation
                FAM32 is from the company FEGS limited
                maybe this company does not exist any more
                elements_hexa20[elem] = (
                    nd6, nd7, nd8, nd5, nd2, nd3, nd4, nd1, nd14, nd15,
                    nd16, nd13, nd10, nd11, nd12, nd9, nd18, nd19, nd20, nd17
                )
                elements_hexa20[elem] = (
                    nd6, nd7, nd8, nd5, nd2, nd3, nd4, nd1, nd14, nd15,
                    nd16, nd13, nd18, nd19, nd20, nd17, nd10, nd11, nd12, nd9
                )
                hexa20 import works with the following frd file node assignment
                """
                elements_hexa20[elem] = (
                    nd8, nd5, nd6, nd7, nd4, nd1, nd2, nd3, nd20, nd17,
                    nd18, nd19, nd12, nd9, nd10, nd11, nd16, nd13, nd14, nd15
                )
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
                """
                CalculiX uses a different node order in
                input file *.inp and result file *.frd for penta15 (C3D15)
                see notes at hexa20
                elements_penta15[elem] = (
                    nd5, nd6, nd4, nd2, nd3, nd1, nd11, nd12, nd10, nd8,
                    nd9, nd7, nd14, nd15, nd13
                )  # order of the *.inp file
                """
                elements_penta15[elem] = (
                    nd5, nd6, nd4, nd2, nd3, nd1, nd14, nd15, nd13, nd8,
                    nd9, nd7, nd11, nd12, nd10
                )
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
                # CalculiX uses a different node order in
                # input file *.inp and result file *.frd for seg3 (B32)
                # see notes at hexa20
                # N1, N2 ,N3
                nd1 = int(line[3:13])
                nd2 = int(line[13:23])
                nd3 = int(line[23:33])
                if inout_nodes:
                    for i in range(len(inout_nodes)):
                        if nd1 == int(inout_nodes[i][1]):
                            # fluid inlet node numbering
                            elements_seg3[elem] = (int(inout_nodes[i][2]), nd3, nd1)
                        elif nd3 == int(inout_nodes[i][1]):
                            # fluid outlet node numbering
                            elements_seg3[elem] = (nd1, int(inout_nodes[i][2]), nd3)
                else:
                    # normal node numbering for D, B32 elements
                    elements_seg3[elem] = (nd1, nd2, nd3)

        # Check if we found new eigenmode line
        if line[5:10] == "PMODE":
            eigentemp = int(line[30:36])
            if eigentemp > eigenmode:
                eigenmode = eigentemp
                mode_eigen_changed = True

        # Check if we found new time step
        if line[4:10] == "1PSTEP":
            mode_time_found = True
        if mode_time_found and (line[2:7] == "100CL"):
            # we found the new time step line
            # !!! be careful here, there is timetemp and timestep!
            # TODO: use more differ names
            timetemp = float(line[13:25])
            if timetemp > timestep:
                timestep = timetemp
                mode_time_changed = True

        # Check if we found displacement section
        if line[5:9] == "DISP":
            mode_disp_found = True
        if mode_disp_found and (line[1:3] == "-1"):
            # we found a displacement line
            elem = int(line[4:13])
            mode_disp_x = float(line[13:25])
            mode_disp_y = float(line[25:37])
            mode_disp_z = float(line[37:49])
            mode_disp[elem] = FreeCAD.Vector(mode_disp_x, mode_disp_y, mode_disp_z)

        # Check if we found stress section
        if line[5:11] == "STRESS":
            mode_stress_found = True
        if mode_stress_found and (line[1:3] == "-1"):
            # we found a stress line
            elem = int(line[4:13])
            stress_1 = float(line[13:25])
            stress_2 = float(line[25:37])
            stress_3 = float(line[37:49])
            stress_4 = float(line[49:61])
            stress_5 = float(line[61:73])
            stress_6 = float(line[73:85])
            # CalculiX frd files: (Sxx, Syy, Szz, Sxy, Syz, Szx)
            # FreeCAD:            (Sxx, Syy, Szz, Sxy, Sxz, Syz)
            # thus exchange the last two entries
            mode_stress[elem] = (stress_1, stress_2, stress_3, stress_4, stress_6, stress_5)

        # Check if we found strain section
        if line[5:13] == "TOSTRAIN":
            mode_strain_found = True
        if mode_strain_found and (line[1:3] == "-1"):
            # we found a strain line in the frd file
            elem = int(line[4:13])
            strain_1 = float(line[13:25])
            strain_2 = float(line[25:37])
            strain_3 = float(line[37:49])
            strain_4 = float(line[49:61])
            strain_5 = float(line[61:73])
            strain_6 = float(line[73:85])
            # CalculiX frd files: (Exx, Eyy, Ezz, Exy, Eyz, Ezx)
            # FreeCAD:            (Exx, Eyy, Ezz, Exy, Exz, Eyz)
            # thus exchange the last two entries
            mode_strain[elem] = (strain_1, strain_2, strain_3, strain_4, strain_6, strain_5)

        # Check if we found an equivalent plastic strain section
        if line[5:7] == "PE":
            mode_peeq_found = True
        if mode_peeq_found and (line[1:3] == "-1"):
            # we found an equivalent plastic strain line
            elem = int(line[4:13])
            peeq = float(line[13:25])
            mode_peeq[elem] = (peeq)

        # Check if we found a temperature section
        if line[5:11] == "NDTEMP":
            mode_temp_found = True
        if mode_temp_found and (line[1:3] == "-1"):
            # we found a temperature line
            elem = int(line[4:13])
            temperature = float(line[13:25])
            mode_temp[elem] = (temperature)

        # Check if we found a mass flow section
        if line[5:11] == "MAFLOW":
            mode_massflow_found = True
        if mode_massflow_found and (line[1:3] == "-1"):
            # we found a mass flow line
            elem = int(line[4:13])
            massflow = float(line[13:25])
            mode_massflow[elem] = (massflow * 1000)  # convert units to kg/s from t/s
            if inout_nodes:
                for i in range(len(inout_nodes)):
                    if elem == int(inout_nodes[i][1]):
                        node = int(inout_nodes[i][2])
                        # convert units to kg/s from t/s
                        mode_massflow[node] = (massflow * 1000)

        # Check if we found a network pressure section
        if line[5:11] == "STPRES":
            mode_networkpressure_found = True
        if mode_networkpressure_found and (line[1:3] == "-1"):
            # we found a network pressure line
            elem = int(line[4:13])
            networkpressure = float(line[13:25])
            mode_networkpressure[elem] = (networkpressure)
            if inout_nodes:
                for i in range(len(inout_nodes)):
                    if elem == int(inout_nodes[i][1]):
                        node = int(inout_nodes[i][2])
                        mode_networkpressure[node] = (networkpressure)

        # Check if we found the end of a section
        if line[1:3] == "-3":
            end_of_section_found = True

            if nodes_found:
                nodes_found = False
                node_element_section = True

            if elements_found:
                elements_found = False
                node_element_section = True

            if mode_disp_found:
                mode_results["disp"] = mode_disp
                mode_disp = {}
                mode_disp_found = False
                node_element_section = False

            if mode_stress_found:
                mode_results["stress"] = mode_stress
                mode_stress = {}
                mode_stress_found = False
                node_element_section = False

            if mode_strain_found:
                mode_results["strain"] = mode_strain

                mode_strain = {}
                mode_strain_found = False
                node_element_section = False

            if mode_peeq_found:
                mode_results["peeq"] = mode_peeq
                mode_peeq = {}
                mode_peeq_found = False
                node_element_section = False

            if mode_temp_found:
                mode_results["temp"] = mode_temp
                mode_temp = {}
                mode_temp_found = False
                node_element_section = False

            if mode_massflow_found:
                mode_results["mflow"] = mode_massflow
                mode_massflow = {}
                mode_massflow_found = False
                node_element_section = False

            if mode_networkpressure_found:
                mode_results["npressure"] = mode_networkpressure
                mode_networkpressure_found = False
                mode_networkpressure = {}
                node_element_section = False

            """
            print("---- End of Section --> Mode_Results may be changed ----")
            for key in sorted(mode_results):
                if key is "number" or key is "time":
                    print(key + " --> " + str(mode_results[key]))
                else:
                    print(key + " --> " + str(len(mode_results[key])))
            print("----Mode_Results----\n")
            """

        # Check if we found the end of frd data
        if line[1:5] == "9999":
            end_of_frd_data_found = True

        if (mode_eigen_changed or mode_time_changed or end_of_frd_data_found) \
                and end_of_section_found \
                and not node_element_section:

            """
            print("\n\n----Append mode_results to results")
            print(line)
            for key in sorted(mode_results):
                if key is "number" or key is "time":
                    print(key + " --> " + str(mode_results[key]))
                else:
                    print(key + " --> " + str(len(mode_results[key])))
            print("----Append Mode_Results----\n")
            """

            # append mode_results to results and reset mode_result
            results.append(mode_results)
            mode_results = {}
            # https://forum.freecad.org/viewtopic.php?f=18&t=32649&start=10#p274686
            mode_results["number"] = float("NaN")
            mode_results["time"] = float("NaN")
            end_of_section_found = False

        # on changed --> write changed values in mode_result
        # will be the first to do on an empty mode_result
        if mode_eigen_changed:
            mode_results["number"] = eigenmode
            mode_eigen_changed = False

        if mode_time_changed:
            mode_results["time"] = timestep
            # mode_results["time"] = 0  # Don't return time if static  # Why?
            mode_time_found = False
            mode_time_changed = False

        # here we are in the indent of loop for every line in frd file
        # do not add a print here :-)

    # close frd file if loop over all lines is finished
    frd_file.close()

    """
    # debug prints and checks with the read data
    print("\n\n----RESULTS values begin----")
    print(len(results))
    # print("\n")
    # print(results)
    print("----RESULTS values end----\n\n")
    """

    if not inout_nodes:
        if results:
            if "mflow" in results[0] or "npressure" in results[0]:
                Console.PrintError(
                    "We have mflow or npressure, but no inout_nodes file.\n"
                )
    if not nodes:
        Console.PrintError("FEM: No nodes found in Frd file.\n")

    return {
        "Nodes": nodes,
        "Seg2Elem": elements_seg2,
        "Seg3Elem": elements_seg3,
        "Tria3Elem": elements_tria3,
        "Tria6Elem": elements_tria6,
        "Quad4Elem": elements_quad4,
        "Quad8Elem": elements_quad8,
        "Tetra4Elem": elements_tetra4,
        "Tetra10Elem": elements_tetra10,
        "Hexa8Elem": elements_hexa8,
        "Hexa20Elem": elements_hexa20,
        "Penta6Elem": elements_penta6,
        "Penta15Elem": elements_penta15,
        "Results": results
    }
