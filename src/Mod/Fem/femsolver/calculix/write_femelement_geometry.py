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
    f.write("\n{}\n".format(59 * "*"))
    f.write("** Sections\n")
    for ccx_elset in ccxwriter.ccx_elsets:
        if ccx_elset["ccx_elset"]:
            if "beamsection_obj"in ccx_elset:  # beam mesh
                beamsec_obj = ccx_elset["beamsection_obj"]
                elsetdef = "ELSET=" + ccx_elset["ccx_elset_name"] + ", "
                material = "MATERIAL=" + ccx_elset["mat_obj_name"]
                normal = ccx_elset["beam_normal"]
                if beamsec_obj.SectionType == "Rectangular":
                    height = beamsec_obj.RectHeight.getValueAs("mm")
                    width = beamsec_obj.RectWidth.getValueAs("mm")
                    section_type = ", SECTION=RECT"
                    section_geo = str(height) + ", " + str(width) + "\n"
                    section_def = "*BEAM SECTION, {}{}{}\n".format(
                        elsetdef,
                        material,
                        section_type
                    )
                elif beamsec_obj.SectionType == "Circular":
                    radius = 0.5 * beamsec_obj.CircDiameter.getValueAs("mm")
                    section_type = ", SECTION=CIRC"
                    section_geo = str(radius) + "\n"
                    section_def = "*BEAM SECTION, {}{}{}\n".format(
                        elsetdef,
                        material,
                        section_type
                    )
                elif beamsec_obj.SectionType == "Pipe":
                    radius = 0.5 * beamsec_obj.PipeDiameter.getValueAs("mm")
                    thickness = beamsec_obj.PipeThickness.getValueAs("mm")
                    section_type = ", SECTION=PIPE"
                    section_geo = str(radius) + ", " + str(thickness) + "\n"
                    section_def = "*BEAM GENERAL SECTION, {}{}{}\n".format(
                        elsetdef,
                        material,
                        section_type
                    )
                # see forum topic for output formatting of rotation
                # https://forum.freecadweb.org/viewtopic.php?f=18&t=46133&p=405142#p405142
                section_nor = "{:f}, {:f}, {:f}\n".format(
                    normal[0],
                    normal[1],
                    normal[2]
                )
                f.write(section_def)
                f.write(section_geo)
                f.write(section_nor)
            elif "fluidsection_obj"in ccx_elset:  # fluid mesh
                fluidsec_obj = ccx_elset["fluidsection_obj"]
                elsetdef = "ELSET=" + ccx_elset["ccx_elset_name"] + ", "
                material = "MATERIAL=" + ccx_elset["mat_obj_name"]
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
                elsetdef = "ELSET=" + ccx_elset["ccx_elset_name"] + ", "
                material = "MATERIAL=" + ccx_elset["mat_obj_name"]
                section_def = "*SHELL SECTION, " + elsetdef + material + "\n"
                section_geo = str(shellth_obj.Thickness.getValueAs("mm")) + "\n"
                f.write(section_def)
                f.write(section_geo)
            else:  # solid mesh
                elsetdef = "ELSET=" + ccx_elset["ccx_elset_name"] + ", "
                material = "MATERIAL=" + ccx_elset["mat_obj_name"]
                section_def = "*SOLID SECTION, " + elsetdef + material + "\n"
                f.write(section_def)


# ************************************************************************************************
# Helpers
def liquid_section_def(obj, section_type):
    if section_type == "PIPE MANNING":
        manning_area = str(obj.ManningArea.getValueAs("mm^2").Value)
        manning_radius = str(obj.ManningRadius.getValueAs("mm"))
        manning_coefficient = str(obj.ManningCoefficient)
        section_geo = manning_area + "," + manning_radius + "," + manning_coefficient + "\n"
        return section_geo
    elif section_type == "PIPE ENLARGEMENT":
        enlarge_area1 = str(obj.EnlargeArea1.getValueAs("mm^2").Value)
        enlarge_area2 = str(obj.EnlargeArea2.getValueAs("mm^2").Value)
        section_geo = enlarge_area1 + "," + enlarge_area2 + "\n"
        return section_geo
    elif section_type == "PIPE CONTRACTION":
        contract_area1 = str(obj.ContractArea1.getValueAs("mm^2").Value)
        contract_area2 = str(obj.ContractArea2.getValueAs("mm^2").Value)
        section_geo = contract_area1 + "," + contract_area2 + "\n"
        return section_geo
    elif section_type == "PIPE ENTRANCE":
        entrance_pipe_area = str(obj.EntrancePipeArea.getValueAs("mm^2").Value)
        entrance_area = str(obj.EntranceArea.getValueAs("mm^2").Value)
        section_geo = entrance_pipe_area + "," + entrance_area + "\n"
        return section_geo
    elif section_type == "PIPE DIAPHRAGM":
        diaphragm_pipe_area = str(obj.DiaphragmPipeArea.getValueAs("mm^2").Value)
        diaphragm_area = str(obj.DiaphragmArea.getValueAs("mm^2").Value)
        section_geo = diaphragm_pipe_area + "," + diaphragm_area + "\n"
        return section_geo
    elif section_type == "PIPE BEND":
        bend_pipe_area = str(obj.BendPipeArea.getValueAs("mm^2").Value)
        bend_radius_diameter = str(obj.BendRadiusDiameter)
        bend_angle = str(obj.BendAngle)
        bend_loss_coefficient = str(obj.BendLossCoefficient)
        section_geo = ("{},{},{},{}\n".format(
            bend_pipe_area,
            bend_radius_diameter,
            bend_angle,
            bend_loss_coefficient
        ))
        return section_geo
    elif section_type == "PIPE GATE VALVE":
        gatevalve_pipe_area = str(obj.GateValvePipeArea.getValueAs("mm^2").Value)
        gatevalve_closing_coeff = str(obj.GateValveClosingCoeff)
        section_geo = gatevalve_pipe_area + "," + gatevalve_closing_coeff + "\n"
        return section_geo
    elif section_type == "PIPE WHITE-COLEBROOK":
        colebrooke_area = str(obj.ColebrookeArea.getValueAs("mm^2").Value)
        colebrooke_diameter = str(2 * obj.ColebrookeRadius.getValueAs("mm"))
        colebrooke_grain_diameter = str(obj.ColebrookeGrainDiameter.getValueAs("mm"))
        colebrooke_form_factor = str(obj.ColebrookeFormFactor)
        section_geo = ("{},{},{},{},{}\n".format(
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
            flow_rate = str(obj.PumpFlowRate[i])
            top = str(obj.PumpHeadLoss[i])
            section_geo = section_geo + flow_rate + "," + top + ","
        section_geo = section_geo + "\n"
        return section_geo
    else:
        return ""
