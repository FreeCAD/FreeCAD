# ***************************************************************************
# *   Copyright (c) 2020 Przemo Firszt <przemo@firszt.eu>                   *
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "Analysis Checks"
__author__ = "Przemo Firszt, Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

import FreeCAD

from FreeCAD import Units

from . import femutils
from femsolver.calculix.solver import ANALYSIS_TYPES


def check_member_for_solver_calculix(analysis, solver, mesh, member):

    message = ""

    # mesh
    if not mesh:
        message += "A single mesh object must be defined in the analysis.\n"
    if mesh:
        if (
            mesh.FemMesh.VolumeCount == 0
            and mesh.FemMesh.FaceCount > 0
            and not member.geos_shellthickness
        ):
            message += (
                "FEM mesh has no volume elements, "
                "either define shell thicknesses or "
                "provide a FEM mesh with volume elements.\n"
            )
        if (
            mesh.FemMesh.VolumeCount == 0
            and mesh.FemMesh.FaceCount == 0
            and mesh.FemMesh.EdgeCount > 0
            and not member.geos_beamsection
            and not member.geos_fluidsection
        ):
            message += (
                "FEM mesh has no volume and no shell elements, "
                "either define a beam/fluid section or provide "
                "a FEM mesh with volume elements.\n"
            )
        if (
            mesh.FemMesh.VolumeCount == 0
            and mesh.FemMesh.FaceCount == 0
            and mesh.FemMesh.EdgeCount == 0
        ):
            message += (
                "FEM mesh has neither volume nor shell or edge elements. "
                "Provide a FEM mesh with elements.\n"
            )

    # material linear and nonlinear
    if not member.mats_linear:
        message += "No material object defined in the analysis.\n"
    has_no_references = False
    for m in member.mats_linear:
        if len(m["Object"].References) == 0:
            if has_no_references is True:
                message += (
                    "More than one material has an empty references list "
                    "(Only one empty references list is allowed!).\n"
                )
            has_no_references = True
    mat_ref_shty = ""
    for m in member.mats_linear:
        ref_shty = femutils.get_refshape_type(m["Object"])
        if ref_shty == "Compound":
            ref_shty = "Solid"
        if not mat_ref_shty:
            mat_ref_shty = ref_shty
        if mat_ref_shty and ref_shty and ref_shty != mat_ref_shty:
            # mat_ref_shty could be empty in one material
            # only the not empty ones should have the same shape type
            message += (
                "Some material objects do not have the same reference shape type "
                "(all material objects must have the same reference shape type, "
                "at the moment).\n"
            )
    for m in member.mats_linear:
        mat_map = m["Object"].Material
        mat_obj = m["Object"]
        if mat_obj.Category == "Solid":
            if "YoungsModulus" in mat_map:
                # print(Units.Quantity(mat_map["YoungsModulus"]).Value)
                if not Units.Quantity(mat_map["YoungsModulus"]).Value:
                    message += "Value of YoungsModulus is set to 0.0.\n"
            else:
                message += "No YoungsModulus defined for at least one material.\n"
            if "PoissonRatio" not in mat_map:
                # PoissonRatio is allowed to be 0.0 (in ccx), but it should be set anyway.
                message += "No PoissonRatio defined for at least one material.\n"
        if solver.AnalysisType == "frequency" or member.cons_selfweight:
            if "Density" not in mat_map:
                message += "No Density defined for at least one material.\n"
        if solver.AnalysisType == "thermomech":
            if "ThermalConductivity" in mat_map:
                if not Units.Quantity(mat_map["ThermalConductivity"]).Value:
                    message += "Value of ThermalConductivity is set to 0.0.\n"
            else:
                message += (
                    "Thermomechanical analysis: No ThermalConductivity defined "
                    "for at least one material.\n"
                )
            if "ThermalExpansionCoefficient" not in mat_map and mat_obj.Category == "Solid":
                message += (
                    "Thermomechanical analysis: No ThermalExpansionCoefficient defined "
                    "for at least one material.\n"  # allowed to be 0.0 (in ccx)
                )
            if "SpecificHeat" not in mat_map:
                message += (
                    "Thermomechanical analysis: No SpecificHeat "
                    "defined for at least one material.\n"  # allowed to be 0.0 (in ccx)
                )
        if femutils.is_of_type(mat_obj, "Fem::MaterialReinforced"):
            # additional tests for reinforced materials,
            # they are needed for result calculation, not for ccx analysis
            mat_map_m = mat_obj.Material
            if "AngleOfFriction" in mat_map_m:
                # print(Units.Quantity(mat_map_m["AngleOfFriction"]).Value)
                if not Units.Quantity(mat_map_m["AngleOfFriction"]).Value:
                    message += (
                        "Value of AngleOfFriction is set to 0.0 "
                        "for the matrix of a reinforced material.\n"
                    )
            else:
                message += (
                    "No AngleOfFriction defined for the matrix "
                    "of at least one reinforced material.\n"
                )
            if "CompressiveStrength" in mat_map_m:
                # print(Units.Quantity(mat_map_m["CompressiveStrength"]).Value)
                if not Units.Quantity(mat_map_m["CompressiveStrength"]).Value:
                    message += (
                        "Value of CompressiveStrength is set to 0.0 "
                        "for the matrix of a reinforced material.\n"
                    )
            else:
                message += (
                    "No CompressiveStrength defined for the matrix "
                    "of at least one reinforced material.\n"
                )
            mat_map_r = mat_obj.Reinforcement
            if "YieldStrength" in mat_map_r:
                # print(Units.Quantity(mat_map_r["YieldStrength"]).Value)
                if not Units.Quantity(mat_map_r["YieldStrength"]).Value:
                    message += (
                        "Value of YieldStrength is set to 0.0 "
                        "for the reinforcement of a reinforced material.\n"
                    )
            else:
                message += (
                    "No YieldStrength defined for the reinforcement "
                    "of at least one reinforced material.\n"
                )
    if len(member.mats_linear) == 1:
        mobj = member.mats_linear[0]["Object"]
        if hasattr(mobj, "References") and mobj.References:
            FreeCAD.Console.PrintError(
                "Only one material object, but this one has a reference shape. "
                "The reference shape will be ignored.\n"
            )

    # which analysis needs which constraints
    # no check in the regard of loads existence (constraint force, pressure, self weight)
    # is done, because an analysis without loads at all is a valid analysis too
    if solver.AnalysisType == "static":
        if not (member.cons_fixed or member.cons_displacement or member.cons_rigidbody):
            message += "Static analysis: No mechanical boundary conditions defined.\n"
    if solver.AnalysisType == "thermomech":
        if not member.cons_initialtemperature:
            if not member.geos_fluidsection:
                message += "Thermomechanical analysis: No initial temperature defined.\n"

    # constraints
    # fixed
    if member.cons_fixed:
        for c in member.cons_fixed:
            if len(c["Object"].References) == 0:
                message += "{} has empty references.".format(c["Object"].Name)
    # displacement
    if member.cons_displacement:
        for di in member.cons_displacement:
            if len(di["Object"].References) == 0:
                message += "{} has empty references.".format(c["Object"].Name)
    # plane rotation
    if member.cons_planerotation:
        for c in member.cons_planerotation:
            if len(c["Object"].References) == 0:
                message += "{} has empty references.".format(c["Object"].Name)
    # contact
    if member.cons_contact:
        for c in member.cons_contact:
            if len(c["Object"].References) == 0:
                message += "{} has empty references.".format(c["Object"].Name)
    # tie
    if member.cons_tie:
        for c in member.cons_tie:
            items = 0
            for reference in c["Object"].References:
                items += len(reference[1])
            if items != 2:
                message += "{} doesn't reference exactly two needed faces.\n".format(
                    c["Object"].Name
                )
    # sectionprint
    if member.cons_sectionprint:
        for c in member.cons_sectionprint:
            items = 0
            for reference in c["Object"].References:
                items += len(reference[1])
            if items != 1:
                message += "{} doesn't reference exactly one needed face.\n".format(
                    c["Object"].Name
                )
    # transform
    if member.cons_transform:
        for c in member.cons_transform:
            if len(c["Object"].References) == 0:
                message += "{} has empty references.".format(c["Object"].Name)
    # pressure
    if member.cons_pressure:
        for c in member.cons_pressure:
            if len(c["Object"].References) == 0:
                message += "{} has empty references.".format(c["Object"].Name)
    # force
    if member.cons_force:
        for c in member.cons_force:
            if len(c["Object"].References) == 0:
                message += "{} has empty references.".format(c["Object"].Name)
    # temperature
    if member.cons_temperature:
        for c in member.cons_temperature:
            if len(c["Object"].References) == 0:
                message += "{} has empty references.".format(c["Object"].Name)
    # heat flux
    if member.cons_heatflux:
        for c in member.cons_heatflux:
            if len(c["Object"].References) == 0:
                message += "{} has empty references.".format(c["Object"].Name)

    # geometries
    # beam section
    if member.geos_beamsection:
        if member.geos_shellthickness:
            # this needs to be checked only once either here or in shell_thicknesses
            message += (
                "Beam sections and shell thicknesses in one analysis "
                "are not supported at the moment.\n"
            )
        if member.geos_fluidsection:
            # this needs to be checked only once either here or in shell_thicknesses
            message += (
                "Beam sections and fluid sections in one analysis "
                "are not supported at the moment.\n"
            )
        has_no_references = False
        for b in member.geos_beamsection:
            if len(b["Object"].References) == 0:
                if has_no_references is True:
                    message += (
                        "More than one beam section has an empty references "
                        "list (Only one empty references list is allowed!).\n"
                    )
                has_no_references = True
        if mesh:
            if mesh.FemMesh.FaceCount > 0 or mesh.FemMesh.VolumeCount > 0:
                message += "Beam sections defined but FEM mesh has volume or shell elements.\n"
            if mesh.FemMesh.EdgeCount == 0:
                message += "Beam sections defined but FEM mesh has no edge elements.\n"
            if not (hasattr(mesh, "Shape") or hasattr(mesh, "Part")):
                message += (
                    "Mesh without geometry link. "
                    "The mesh needs to know its geometry for the beam rotations.\n"
                )
        if len(member.geos_beamrotation) > 1:
            message += "Multiple beam rotations in one analysis are not supported at the moment.\n"
    # beam rotations
    if member.geos_beamrotation and not member.geos_beamsection:
        message += "Beam rotations in the analysis but no beam sections defined.\n"
    # shell thickness
    if member.geos_shellthickness:
        has_no_references = False
        for s in member.geos_shellthickness:
            if len(s["Object"].References) == 0:
                if has_no_references is True:
                    message += (
                        "More than one shell thickness has an empty references "
                        "list (Only one empty references list is allowed!).\n"
                    )
                has_no_references = True
        if mesh:
            if mesh.FemMesh.VolumeCount > 0:
                message += "Shell thicknesses defined but FEM mesh has volume elements.\n"
            if mesh.FemMesh.FaceCount == 0:
                message += "Shell thicknesses defined but FEM mesh has no shell elements.\n"
    # fluid section
    if member.geos_fluidsection:
        if not member.cons_selfweight:
            message += "A fluid network analysis requires self weight constraint to be applied\n"
        if solver.AnalysisType != "thermomech":
            message += "A fluid network analysis can only be done in a thermomech analysis\n"
        has_no_references = False
        for f in member.geos_fluidsection:
            if len(f["Object"].References) == 0:
                if has_no_references is True:
                    message += (
                        "More than one fluid section has an empty references list "
                        "(Only one empty references list is allowed!).\n"
                    )
                has_no_references = True
        if mesh:
            if mesh.FemMesh.FaceCount > 0 or mesh.FemMesh.VolumeCount > 0:
                message += "Fluid sections defined but FEM mesh has volume or shell elements.\n"
            if mesh.FemMesh.EdgeCount == 0:
                message += "Fluid sections defined but FEM mesh has no edge elements.\n"

    return message


##  @}
