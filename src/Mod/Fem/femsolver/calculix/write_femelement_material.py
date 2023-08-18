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
__url__ = "https://www.freecad.org"


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
        if ccxwriter.member.cons_centrif:
            return True
        if ccxwriter.member.cons_selfweight:
            return True
        return False

    f.write("\n{}\n".format(59 * "*"))
    f.write("** Materials\n")
    f.write("** see information about units at file end\n")
    for femobj in ccxwriter.member.mats_linear:
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

            for nlfemobj in ccxwriter.member.mats_nonlinear:
                # femobj --> dict, FreeCAD document object is nlfemobj["Object"]
                nl_mat_obj = nlfemobj["Object"]
                if nl_mat_obj.LinearBaseMaterial == mat_obj:
                    if nl_mat_obj.MaterialModelNonlinearity == "simple hardening":
                        f.write("*PLASTIC\n")
                        for yield_point in nl_mat_obj.YieldPoints:
                            f.write("{}\n".format(yield_point))
                f.write("\n")
