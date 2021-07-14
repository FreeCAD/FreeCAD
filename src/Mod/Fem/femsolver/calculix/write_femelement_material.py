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

__title__ = "FreeCAD FEM calculix write inpfile materials"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecadweb.org"


import FreeCAD


def write_femelement_material(f, ccxwriter):

    # floats read from ccx should use {:.13G}, see comment in writer module
    # see unit comment in writer module

    def is_density_needed():
        if ccxwriter.analysis_type == "frequency":
            return True
        if (
            ccxwriter.analysis_type == "thermomech"
            and not ccxwriter.solver_obj.ThermoMechSteadyState
        ):
            return True
        if ccxwriter.centrif_objects:
            return True
        if ccxwriter.selfweight_objects:
            return True
        return False

    f.write("\n{}\n".format(59 * "*"))
    f.write("** Materials\n")
    f.write("** see information about units at file end\n")
    for femobj in ccxwriter.material_objects:
        # femobj --> dict, FreeCAD document object is femobj["Object"]
        mat_obj = femobj["Object"]
        mat_info_name = mat_obj.Material["Name"]
        mat_name = mat_obj.Name
        mat_label = mat_obj.Label

        # get material properties of solid material, Currently in SI units: M/kg/s/Kelvin
        if mat_obj.Category == "Solid":
            YM = FreeCAD.Units.Quantity(mat_obj.Material["YoungsModulus"])
            YM_in_MPa = YM.getValueAs("MPa").Value
            PR = float(mat_obj.Material["PoissonRatio"])
        if is_density_needed() is True:
            density = FreeCAD.Units.Quantity(mat_obj.Material["Density"])
            density_in_tonne_per_mm3 = density.getValueAs("t/mm^3").Value
        if ccxwriter.analysis_type == "thermomech":
            TC = FreeCAD.Units.Quantity(mat_obj.Material["ThermalConductivity"])
            # SvdW: Add factor to force units to results base units
            # of t/mm/s/K - W/m/K results in no factor needed
            TC_in_WmK = TC.getValueAs("W/m/K").Value
            SH = FreeCAD.Units.Quantity(mat_obj.Material["SpecificHeat"])
            # SvdW: Add factor to force units to results base units of t/mm/s/K
            # FIXME: why not get it directly in the units needed ?
            SH_in_JkgK = SH.getValueAs("J/kg/K").Value * 1e+06
            if mat_obj.Category == "Solid":
                TEC = FreeCAD.Units.Quantity(mat_obj.Material["ThermalExpansionCoefficient"])
                TEC_in_mmK = TEC.getValueAs("mm/mm/K").Value
            elif mat_obj.Category == "Fluid":
                DV = FreeCAD.Units.Quantity(mat_obj.Material["DynamicViscosity"])
                DV_in_tmms = DV.getValueAs("t/mm/s").Value

        # write material properties
        f.write("** FreeCAD material name: {}\n".format(mat_info_name))
        f.write("** {}\n".format(mat_label))
        f.write("*MATERIAL, NAME={}\n".format(mat_name))
        if mat_obj.Category == "Solid":
            f.write("*ELASTIC\n")
            f.write("{:.13G},{:.13G}\n".format(YM_in_MPa, PR))
        if is_density_needed() is True:
            f.write("*DENSITY\n")
            f.write("{:.13G}\n".format(density_in_tonne_per_mm3))
        if ccxwriter.analysis_type == "thermomech":
            if mat_obj.Category == "Solid":
                f.write("*CONDUCTIVITY\n")
                f.write("{:.13G}\n".format(TC_in_WmK))
                f.write("*EXPANSION\n")
                f.write("{:.13G}\n".format(TEC_in_mmK))
                f.write("*SPECIFIC HEAT\n")
                f.write("{:.13G}\n".format(SH_in_JkgK))
            elif mat_obj.Category == "Fluid":
                f.write("*FLUID CONSTANTS\n")
                f.write("{:.13G},{:.13G}\n".format(SH_in_JkgK, DV_in_tmms))

        # nonlinear material properties
        if ccxwriter.solver_obj.MaterialNonlinearity == "nonlinear":

            for nlfemobj in ccxwriter.material_nonlinear_objects:
                # femobj --> dict, FreeCAD document object is nlfemobj["Object"]
                nl_mat_obj = nlfemobj["Object"]
                if nl_mat_obj.LinearBaseMaterial == mat_obj:
                    if nl_mat_obj.MaterialModelNonlinearity == "simple hardening":
                        f.write("*PLASTIC\n")
                        if nl_mat_obj.YieldPoint1:
                            f.write("{}\n".format(nl_mat_obj.YieldPoint1))
                        if nl_mat_obj.YieldPoint2:
                            f.write("{}\n".format(nl_mat_obj.YieldPoint2))
                        if nl_mat_obj.YieldPoint3:
                            f.write("{}\n".format(nl_mat_obj.YieldPoint3))
                f.write("\n")


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
