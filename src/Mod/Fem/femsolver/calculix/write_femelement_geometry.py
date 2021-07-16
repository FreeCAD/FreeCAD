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

__title__ = "FreeCAD FEM calculix write inpfile femelement geometry"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecadweb.org"


def write_femelement_geometry(f, ccxwriter):

    # floats read from ccx should use {:.13G}, see comment in writer module

    f.write("\n{}\n".format(59 * "*"))
    f.write("** Sections\n")
    for ccx_elset in ccxwriter.mat_geo_sets:
        if ccx_elset["ccx_elset"]:
            elsetdef = "ELSET={}, ".format(ccx_elset["ccx_elset_name"])
            material = "MATERIAL={}".format(ccx_elset["mat_obj_name"])

            if "beamsection_obj"in ccx_elset:  # beam mesh
                beamsec_obj = ccx_elset["beamsection_obj"]
                normal = ccx_elset["beam_normal"]
                if beamsec_obj.SectionType == "Rectangular":
                    height = beamsec_obj.RectHeight.getValueAs("mm").Value
                    width = beamsec_obj.RectWidth.getValueAs("mm").Value
                    section_type = ", SECTION=RECT"
                    section_geo = "{:.13G},{:.13G}\n".format(height, width)
                    section_def = "*BEAM SECTION, {}{}{}\n".format(
                        elsetdef,
                        material,
                        section_type
                    )
                elif beamsec_obj.SectionType == "Circular":
                    radius = 0.5 * beamsec_obj.CircDiameter.getValueAs("mm").Value
                    section_type = ", SECTION=CIRC"
                    section_geo = "{:.13G}\n".format(radius)
                    section_def = "*BEAM SECTION, {}{}{}\n".format(
                        elsetdef,
                        material,
                        section_type
                    )
                elif beamsec_obj.SectionType == "Pipe":
                    radius = 0.5 * beamsec_obj.PipeDiameter.getValueAs("mm").Value
                    thickness = beamsec_obj.PipeThickness.getValueAs("mm").Value
                    section_type = ", SECTION=PIPE"
                    section_geo = "{:.13G},{:.13G}\n".format(radius, thickness)
                    section_def = "*BEAM GENERAL SECTION, {}{}{}\n".format(
                        elsetdef,
                        material,
                        section_type
                    )
                # see forum topic for output formatting of rotation
                # https://forum.freecadweb.org/viewtopic.php?f=18&t=46133&p=405142#p405142
                section_nor = "{:.13G}, {:.13G}, {:.13G}\n".format(
                    normal[0],
                    normal[1],
                    normal[2]
                )
                f.write(section_def)
                f.write(section_geo)
                f.write(section_nor)
            elif "fluidsection_obj"in ccx_elset:  # fluid mesh
                fluidsec_obj = ccx_elset["fluidsection_obj"]
                if fluidsec_obj.SectionType == "Liquid":
                    section_type = fluidsec_obj.LiquidSectionType
                    if (section_type == "PIPE INLET") or (section_type == "PIPE OUTLET"):
                        section_type = "PIPE INOUT"
                    section_def = "*FLUID SECTION, {}TYPE={}, {}\n".format(
                        elsetdef,
                        section_type,
                        material
                    )
                    section_geo = liquid_section_def(fluidsec_obj, section_type)
                """
                # deactivate as it would result in section_def and section_geo not defined
                # deactivated in the App and Gui object and thus in the task panel as well
                elif fluidsec_obj.SectionType == "Gas":
                    section_type = fluidsec_obj.GasSectionType
                elif fluidsec_obj.SectionType == "Open Channel":
                    section_type = fluidsec_obj.ChannelSectionType
                """
                f.write(section_def)
                f.write(section_geo)
            elif "shellthickness_obj"in ccx_elset:  # shell mesh
                shellth_obj = ccx_elset["shellthickness_obj"]
                section_def = "*SHELL SECTION, {}{}\n".format(elsetdef, material)
                thickness = shellth_obj.Thickness.getValueAs("mm").Value
                section_geo = "{:.13G}\n".format(thickness)
                f.write(section_def)
                f.write(section_geo)
            else:  # solid mesh
                section_def = "*SOLID SECTION, {}{}\n".format(elsetdef, material)
                f.write(section_def)


# ************************************************************************************************
# Helpers
def liquid_section_def(obj, section_type):
    if section_type == "PIPE MANNING":
        manning_area = obj.ManningArea.getValueAs("mm^2").Value
        manning_radius = obj.ManningRadius.getValueAs("mm").Value
        manning_coefficient = obj.ManningCoefficient
        section_geo = "{:.13G},{:.13G},{:.13G}\n".format(
            manning_area,
            manning_radius,
            manning_coefficient
        )
        return section_geo
    elif section_type == "PIPE ENLARGEMENT":
        enlarge_area1 = obj.EnlargeArea1.getValueAs("mm^2").Value
        enlarge_area2 = obj.EnlargeArea2.getValueAs("mm^2").Value
        section_geo = "{:.13G},{:.13G}\n".format(enlarge_area1, enlarge_area2)
        return section_geo
    elif section_type == "PIPE CONTRACTION":
        contract_area1 = obj.ContractArea1.getValueAs("mm^2").Value
        contract_area2 = obj.ContractArea2.getValueAs("mm^2").Value
        section_geo = "{:.13G},{:.13G}\n".format(contract_area1, contract_area2)
        return section_geo
    elif section_type == "PIPE ENTRANCE":
        entrance_pipe_area = obj.EntrancePipeArea.getValueAs("mm^2").Value
        entrance_area = obj.EntranceArea.getValueAs("mm^2").Value
        section_geo = "{:.13G},{:.13G}\n".format(entrance_pipe_area, entrance_area)
        return section_geo
    elif section_type == "PIPE DIAPHRAGM":
        diaphragm_pipe_area = obj.DiaphragmPipeArea.getValueAs("mm^2").Value
        diaphragm_area = obj.DiaphragmArea.getValueAs("mm^2").Value
        section_geo = "{:.13G},{:.13G}\n".format(diaphragm_pipe_area, diaphragm_area)
        return section_geo
    elif section_type == "PIPE BEND":
        bend_pipe_area = obj.BendPipeArea.getValueAs("mm^2").Value
        bend_radius_diameter = obj.BendRadiusDiameter
        bend_angle = obj.BendAngle
        bend_loss_coefficient = obj.BendLossCoefficient
        section_geo = ("{:.13G},{:.13G},{:.13G},{:.13G}\n".format(
            bend_pipe_area,
            bend_radius_diameter,
            bend_angle,
            bend_loss_coefficient
        ))
        return section_geo
    elif section_type == "PIPE GATE VALVE":
        gatevalve_pipe_area = obj.GateValvePipeArea.getValueAs("mm^2").Value
        gatevalve_closing_coeff = obj.GateValveClosingCoeff
        section_geo = "{:.13G},{:.13G}\n".format(gatevalve_pipe_area, gatevalve_closing_coeff)
        return section_geo
    elif section_type == "PIPE WHITE-COLEBROOK":
        colebrooke_area = obj.ColebrookeArea.getValueAs("mm^2").Value
        colebrooke_diameter = 2 * obj.ColebrookeRadius.getValueAs("mm")
        colebrooke_grain_diameter = obj.ColebrookeGrainDiameter.getValueAs("mm")
        colebrooke_form_factor = obj.ColebrookeFormFactor
        section_geo = ("{:.13G},{:.13G},{},{:.13G},{:.13G}\n".format(
            colebrooke_area,
            colebrooke_diameter,
            "-1",
            colebrooke_grain_diameter,
            colebrooke_form_factor
        ))
        return section_geo
    elif section_type == "LIQUID PUMP":
        section_geo = ""
        for i in range(len(obj.PumpFlowRate)):
            flow_rate = obj.PumpFlowRate[i]
            top = obj.PumpHeadLoss[i]
            section_geo = "{:.13G},{:.13G},\n".format(section_geo + flow_rate, top)
        section_geo = "{}\n".format(section_geo)
        return section_geo
    else:
        return ""
