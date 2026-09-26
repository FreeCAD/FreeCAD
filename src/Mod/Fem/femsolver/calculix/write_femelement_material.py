# SPDX-License-Identifier: LGPL-2.1-or-later

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
from femtools import constants


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

    units = ccxwriter.solver_obj.UnitSystem
    f.write(f"\n** Physical constants for {units} unit system\n")
    stefan_boltzmann = ccxwriter.get_coherent_value(constants.stefan_boltzmann())
    f.write(f"*PHYSICAL CONSTANTS, ABSOLUTE ZERO=0, STEFAN BOLTZMANN={stefan_boltzmann:.13G}\n")

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
            YM = ccxwriter.get_coherent_value(mat_obj.Material["YoungsModulus"])
            PR = ccxwriter.get_coherent_value(mat_obj.Material["PoissonRatio"])
        if is_density_needed() is True:
            density = ccxwriter.get_coherent_value(mat_obj.Material["Density"])
        if ccxwriter.analysis_type == "thermomech":
            TC = ccxwriter.get_coherent_value(mat_obj.Material["ThermalConductivity"])
            SH = ccxwriter.get_coherent_value(mat_obj.Material["SpecificHeat"])
            if mat_obj.Category == "Solid":
                TEC = ccxwriter.get_coherent_value(mat_obj.Material["ThermalExpansionCoefficient"])
                if "ThermalExpansionReferenceTemperature" in mat_obj.Material:
                    RT = ccxwriter.get_coherent_value(
                        mat_obj.Material["ThermalExpansionReferenceTemperature"]
                    )
                else:
                    RT = ccxwriter.get_coherent_value("0 K")
            elif mat_obj.Category == "Fluid":
                KV = ccxwriter.get_coherent_value(mat_obj.Material["KinematicViscosity"])
                DV = KV * density
        if ccxwriter.analysis_type == "static":
            if mat_obj.Category == "Solid":
                if "ThermalExpansionCoefficient" in mat_obj.Material:
                    TEC = ccxwriter.get_coherent_value(
                        mat_obj.Material["ThermalExpansionCoefficient"]
                    )
                else:
                    TEC = 0
                if "ThermalExpansionReferenceTemperature" in mat_obj.Material:
                    RT = ccxwriter.get_coherent_value(
                        mat_obj.Material["ThermalExpansionReferenceTemperature"]
                    )
                else:
                    RT = 0
        if (
            ccxwriter.analysis_type == "electromagnetic"
            and ccxwriter.solver_obj.ElectromagneticMode == "electrostatic"
        ):
            rel_perm = ccxwriter.get_coherent_value(mat_obj.Material["RelativePermittivity"])
            vacuum_perm = ccxwriter.get_coherent_value(constants.vacuum_permittivity())
            abs_perm = vacuum_perm * rel_perm
        # write material properties
        f.write(f"** FreeCAD material name: {mat_info_name}\n")
        f.write(f"** {mat_label}\n")
        f.write(f"*MATERIAL, NAME={mat_name}\n")
        if mat_obj.Category == "Solid":
            f.write("*ELASTIC\n")
            f.write(f"{YM:.13G},{PR:.13G}\n")
        if is_density_needed() is True:
            f.write("*DENSITY\n")
            f.write(f"{density:.13G}\n")
        if ccxwriter.analysis_type == "thermomech":
            if mat_obj.Category == "Solid":
                f.write("*CONDUCTIVITY\n")
                f.write(f"{TC:.13G}\n")
                f.write(f"*EXPANSION, ZERO={RT:.13G}\n")
                f.write(f"{TEC:.13G}\n")
                f.write("*SPECIFIC HEAT\n")
                f.write(f"{SH:.13G}\n")
            elif mat_obj.Category == "Fluid":
                f.write("*FLUID CONSTANTS\n")
                f.write(f"{SH:.13G},{DV:.13G}\n")
        if ccxwriter.analysis_type == "static":
            if mat_obj.Category == "Solid":
                f.write(f"*EXPANSION, ZERO={RT:.13G}\n")
                f.write(f"{TEC:.13G}\n")
        if (
            ccxwriter.analysis_type == "electromagnetic"
            and ccxwriter.solver_obj.ElectromagneticMode == "electrostatic"
        ):
            f.write("*CONDUCTIVITY\n")
            f.write(f"{abs_perm:.13G}\n")

        # nonlinear material properties
        nl_mat_obj = mat_obj.Nonlinear
        if ccxwriter.solver_obj.MaterialNonlinearity:
            if nl_mat_obj and not nl_mat_obj.Suppressed:
                match nl_mat_obj.MaterialModelNonlinearity:
                    case "isotropic hardening":
                        f.write("*PLASTIC\n")
                    case "kinematic hardening":
                        f.write("*PLASTIC, HARDENING=KINEMATIC\n")
                for yield_point in nl_mat_obj.YieldPoints:
                    f.write(f"{yield_point}\n")
                f.write("\n")
