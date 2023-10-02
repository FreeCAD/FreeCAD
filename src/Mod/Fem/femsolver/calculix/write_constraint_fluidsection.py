# ***************************************************************************
# *   Copyright (c) 2021 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM calculix constraint fluidsection"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"


import codecs
import os
from os.path import join

import FreeCAD

from femmesh import meshtools


# ********************************************************************************************
# handle elements for constraints fluidsection with Liquid Inlet or Outlet
# belongs to write_constraints_fluidsection, should be next method
# leave the constraints fluidsection code as the last constraint method in this module
# as it is none standard constraint method compared to all other constraints
def handle_fluidsection_liquid_inlet_outlet(inpfile, ccxwriter):

    if not ccxwriter.member.geos_fluidsection:
        return inpfile

    # Fluid sections:
    # fluidsection Liquid inlet outlet objs  requires special element definition
    # to fill ccxwriter.FluidInletoutlet_ele list the mat_geo_sets are needed
    # thus this has to be after the creation of mat_geo_sets
    # different pipe cross sections will generate mat_geo_sets

    ccxwriter.FluidInletoutlet_ele = []
    ccxwriter.fluid_inout_nodes_file = join(
        ccxwriter.dir_name,
        "{}_inout_nodes.txt".format(ccxwriter.mesh_name)
    )

    def get_fluidsection_inoutlet_obj_if_setdata(matgeoset):
        if (
            matgeoset["ccx_elset"]
            and not isinstance(matgeoset["ccx_elset"], str)
            and "fluidsection_obj" in matgeoset  # fluid mesh
        ):
            fluidsec_obj = matgeoset["fluidsection_obj"]
            if (
                fluidsec_obj.SectionType == "Liquid"
                and (
                    fluidsec_obj.LiquidSectionType == "PIPE INLET"
                    or fluidsec_obj.LiquidSectionType == "PIPE OUTLET"
                )
            ):
                return fluidsec_obj
        return None

    def is_fluidsection_inoutlet_setnames_possible(mat_geo_sets):
        for matgeoset in mat_geo_sets:
            if (
                matgeoset["ccx_elset"]
                and "fluidsection_obj" in matgeoset  # fluid mesh
            ):
                fluidsec_obj = matgeoset["fluidsection_obj"]
                if (
                    fluidsec_obj.SectionType == "Liquid"
                    and (
                        fluidsec_obj.LiquidSectionType == "PIPE INLET"
                        or fluidsec_obj.LiquidSectionType == "PIPE OUTLET"
                    )
                ):
                    return True
        return False

    # collect elementIDs for fluidsection Liquid inlet outlet objs
    # if they have element data (happens if not "eall")
    for matgeoset in ccxwriter.mat_geo_sets:
        fluidsec_obj = get_fluidsection_inoutlet_obj_if_setdata(matgeoset)
        if fluidsec_obj is None:
            continue
        elsetchanged = False
        counter = 0
        for elid in matgeoset["ccx_elset"]:
            counter = counter + 1
            if (elsetchanged is False) \
                    and (fluidsec_obj.LiquidSectionType == "PIPE INLET"):
                # 3rd index is to track which line nr the element is defined
                ccxwriter.FluidInletoutlet_ele.append(
                    [str(elid), fluidsec_obj.LiquidSectionType, 0]
                )
                elsetchanged = True
            elif (fluidsec_obj.LiquidSectionType == "PIPE OUTLET") \
                    and (counter == len(matgeoset["ccx_elset"])):
                # 3rd index is to track which line nr the element is defined
                ccxwriter.FluidInletoutlet_ele.append(
                    [str(elid), fluidsec_obj.LiquidSectionType, 0]
                )

    # create the correct element definition for fluidsection Liquid inlet outlet objs
    # at least one "fluidsection_obj" needs to be in mat_geo_sets and has the attributes
    # TODO: what if there are other objs in elsets?
    if is_fluidsection_inoutlet_setnames_possible(ccxwriter.mat_geo_sets) is not None:
        # it is not distinguished if split input file
        # for split input file the main file is just closed and reopend even if not needed
        inpfile.close()
        meshtools.use_correct_fluidinout_ele_def(
            ccxwriter.FluidInletoutlet_ele,
            ccxwriter.femmesh_file,
            ccxwriter.fluid_inout_nodes_file
        )
        inpfile = codecs.open(ccxwriter.file_name, "a", encoding="utf-8")

    return inpfile


# ********************************************************************************************
# TODO:
# split method into separate methods and move some part into base writer
# see also method handle_fluidsection_liquid_inlet_outlet
def write_constraints_fluidsection(f, ccxwriter):
    if not ccxwriter.member.geos_fluidsection:
        return
    if ccxwriter.analysis_type not in ["thermomech"]:
        return

    # write constraint to file
    f.write("\n***********************************************************\n")
    f.write("** FluidSection constraints\n")
    if os.path.exists(ccxwriter.fluid_inout_nodes_file):
        inout_nodes_file = open(ccxwriter.fluid_inout_nodes_file, "r")
        lines = inout_nodes_file.readlines()
        inout_nodes_file.close()
    else:
        FreeCAD.Console.PrintError(
            "1DFlow inout nodes file not found: {}\n"
            .format(ccxwriter.fluid_inout_nodes_file)
        )
    # get nodes
    ccxwriter.get_constraints_fluidsection_nodes()
    for femobj in ccxwriter.member.geos_fluidsection:
        # femobj --> dict, FreeCAD document object is femobj["Object"]
        fluidsection_obj = femobj["Object"]
        f.write("** " + fluidsection_obj.Label + "\n")
        if fluidsection_obj.SectionType == "Liquid":
            if fluidsection_obj.LiquidSectionType == "PIPE INLET":
                f.write("**Fluid Section Inlet \n")
                if fluidsection_obj.InletPressureActive is True:
                    f.write("*BOUNDARY \n")
                    for n in femobj["Nodes"]:
                        for line in lines:
                            b = line.split(",")
                            if int(b[0]) == n and b[3] == "PIPE INLET\n":
                                # degree of freedom 2 is for defining pressure
                                f.write("{},{},{},{}\n".format(
                                    b[0],
                                    "2",
                                    "2",
                                    fluidsection_obj.InletPressure
                                ))
                if fluidsection_obj.InletFlowRateActive is True:
                    f.write("*BOUNDARY,MASS FLOW \n")
                    for n in femobj["Nodes"]:
                        for line in lines:
                            b = line.split(",")
                            if int(b[0]) == n and b[3] == "PIPE INLET\n":
                                # degree of freedom 1 is for defining flow rate
                                # factor applied to convert unit from kg/s to t/s
                                f.write("{},{},{},{}\n".format(
                                    b[1],
                                    "1",
                                    "1",
                                    fluidsection_obj.InletFlowRate * 0.001
                                ))
            elif fluidsection_obj.LiquidSectionType == "PIPE OUTLET":
                f.write("**Fluid Section Outlet \n")
                if fluidsection_obj.OutletPressureActive is True:
                    f.write("*BOUNDARY \n")
                    for n in femobj["Nodes"]:
                        for line in lines:
                            b = line.split(",")
                            if int(b[0]) == n and b[3] == "PIPE OUTLET\n":
                                # degree of freedom 2 is for defining pressure
                                f.write("{},{},{},{}\n".format(
                                    b[0],
                                    "2",
                                    "2",
                                    fluidsection_obj.OutletPressure
                                ))
                if fluidsection_obj.OutletFlowRateActive is True:
                    f.write("*BOUNDARY,MASS FLOW \n")
                    for n in femobj["Nodes"]:
                        for line in lines:
                            b = line.split(",")
                            if int(b[0]) == n and b[3] == "PIPE OUTLET\n":
                                # degree of freedom 1 is for defining flow rate
                                # factor applied to convert unit from kg/s to t/s
                                f.write("{},{},{},{}\n".format(
                                    b[1],
                                    "1",
                                    "1",
                                    fluidsection_obj.OutletFlowRate * 0.001
                                ))
