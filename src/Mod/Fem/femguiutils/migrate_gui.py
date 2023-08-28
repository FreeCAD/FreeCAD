# ***************************************************************************
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
""" Class and methods to migrate old FEM Gui objects

see module end as well as forum topic
https://forum.freecad.org/viewtopic.php?&t=46218
"""

__title__ = "FEM GUI that migrates old scripted objects"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"


class FemMigrateGui(object):

    def find_module(self, fullname, path):

        if fullname == "femguiobjects":
            return self
        if fullname == "femguiobjects._ViewProviderFemConstraintBodyHeatSource":
            return self
        if fullname == "femguiobjects._ViewProviderFemConstraintElectrostaticPotential":
            return self
        if fullname == "femguiobjects._ViewProviderFemConstraintFlowVelocity":
            return self
        if fullname == "femguiobjects._ViewProviderFemConstraintInitialFlowVelocity":
            return self
        if fullname == "femguiobjects._ViewProviderFemConstraintSelfWeight":
            return self
        if fullname == "femguiobjects._ViewProviderFemConstraintTie":
            return self
        if fullname == "femguiobjects._ViewProviderFemElementFluid1D":
            return self
        if fullname == "femguiobjects._ViewProviderFemElementGeometry1D":
            return self
        if fullname == "femguiobjects._ViewProviderFemElementGeometry2D":
            return self
        if fullname == "femguiobjects._ViewProviderFemElementRotation1D":
            return self
        if fullname == "femguiobjects._ViewProviderFemMaterial":
            return self
        if fullname == "femguiobjects._ViewProviderFemMaterialMechanicalNonlinear":
            return self
        if fullname == "femguiobjects._ViewProviderFemMaterialReinforced":
            return self
        if fullname == "femguiobjects._ViewProviderFemMeshBoundaryLayer":
            return self
        if fullname == "femguiobjects._ViewProviderFemMeshGmsh":
            return self
        if fullname == "femguiobjects._ViewProviderFemMeshGroup":
            return self
        if fullname == "femguiobjects._ViewProviderFemMeshRegion":
            return self
        if fullname == "femguiobjects._ViewProviderFemMeshResult":
            return self
        if fullname == "femguiobjects._ViewProviderFemResultMechanical":
            return self
        if fullname == "femguiobjects._ViewProviderFemSolverCalculix":
            return self

        if fullname == "PyGui":
            return self
        if fullname == "PyGui._ViewProviderFemConstraintBodyHeatSource":
            return self
        if fullname == "PyGui._ViewProviderFemConstraintElectrostaticPotential":
            return self
        if fullname == "PyGui._ViewProviderFemConstraintFlowVelocity":
            return self
        if fullname == "PyGui._ViewProviderFemConstraintSelfWeight":
            return self
        if fullname == "PyGui._ViewProviderFemElementFluid1D":
            return self
        if fullname == "PyGui._ViewProviderFemElementGeometry1D":
            return self
        if fullname == "PyGui._ViewProviderFemElementGeometry2D":
            return self
        if fullname == "PyGui._ViewProviderFemElementRotation1D":
            return self
        if fullname == "PyGui._ViewProviderFemMaterial":
            return self
        if fullname == "PyGui._ViewProviderFemMaterialMechanicalNonlinear":
            return self
        if fullname == "PyGui._ViewProviderFemMeshBoundaryLayer":
            return self
        if fullname == "PyGui._ViewProviderFemMeshGmsh":
            return self
        if fullname == "PyGui._ViewProviderFemMeshGroup":
            return self
        if fullname == "PyGui._ViewProviderFemMeshRegion":
            return self
        if fullname == "PyGui._ViewProviderFemMeshResult":
            return self
        if fullname == "PyGui._ViewProviderFemResultMechanical":
            return self
        if fullname == "PyGui._ViewProviderFemSolverCalculix":
            return self
        if fullname == "PyGui._ViewProviderFemSolverZ88":
            return self

        if fullname == "PyGui._ViewProviderFemBeamSection":
            return self
        if fullname == "PyGui._ViewProviderFemFluidSection":
            return self
        if fullname == "PyGui._ViewProviderFemShellThickness":
            return self

        if fullname == "_ViewProviderFemBeamSection":
            return self
        if fullname == "_ViewProviderFemConstraintSelfWeight":
            return self
        if fullname == "_ViewProviderFemMaterial":
            return self
        if fullname == "_ViewProviderFemMaterialMechanicalNonlinear":
            return self
        if fullname == "_ViewProviderFemMeshGmsh":
            return self
        if fullname == "_ViewProviderFemMeshGroup":
            return self
        if fullname == "_ViewProviderFemMeshRegion":
            return self
        if fullname == "_ViewProviderFemResultMechanical":
            return self
        if fullname == "_ViewProviderFemShellThickness":
            return self
        if fullname == "_ViewProviderFemSolverCalculix":
            return self
        if fullname == "_ViewProviderFemSolverZ88":
            return self

        if fullname == "_ViewProviderFemMechanicalResult":
            return self
        if fullname == "ViewProviderFemResult":
            return self
        if fullname == "_ViewProviderMechanicalMaterial":
            return self

        return None

    def create_module(self, spec):
        return None

    def exec_module(self, module):
        return self.load_module(module)

    def load_module(self, module):

        if module.__name__ == "femguiobjects":
            module.__path__ = "femguiobjects"
        if module.__name__ == "femguiobjects._ViewProviderFemConstraintBodyHeatSource":
            import femviewprovider.view_constraint_bodyheatsource
            module.ViewProxy = \
                femviewprovider.view_constraint_bodyheatsource.VPConstraintBodyHeatSource
        if module.__name__ == "femguiobjects._ViewProviderFemConstraintElectrostaticPotential":
            import femviewprovider.view_constraint_electrostaticpotential
            module.ViewProxy = \
                femviewprovider.view_constraint_electrostaticpotential.VPConstraintElectroStaticPotential
        if module.__name__ == "femguiobjects._ViewProviderFemConstraintFlowVelocity":
            import femviewprovider.view_constraint_flowvelocity
            module.ViewProxy = \
                femviewprovider.view_constraint_flowvelocity.VPConstraintFlowVelocity
        if module.__name__ == "femguiobjects._ViewProviderFemConstraintInitialFlowVelocity":
            import femviewprovider.view_constraint_initialflowvelocity
            module.ViewProxy = \
                femviewprovider.view_constraint_initialflowvelocity.VPConstraintInitialFlowVelocity
        if module.__name__ == "femguiobjects._ViewProviderFemConstraintSelfWeight":
            import femviewprovider.view_constraint_selfweight
            module._ViewProviderFemConstraintSelfWeight = \
                femviewprovider.view_constraint_selfweight.VPConstraintSelfWeight
        if module.__name__ == "femguiobjects._ViewProviderFemConstraintTie":
            import femviewprovider.view_constraint_tie
            module._ViewProviderFemConstraintTie = \
                femviewprovider.view_constraint_tie.VPConstraintTie
        if module.__name__ == "femguiobjects._ViewProviderFemElementFluid1D":
            import femviewprovider.view_element_fluid1D
            module._ViewProviderFemElementFluid1D = \
                femviewprovider.view_element_fluid1D.VPElementFluid1D
        if module.__name__ == "femguiobjects._ViewProviderFemElementGeometry1D":
            import femviewprovider.view_element_geometry1D
            module._ViewProviderFemElementGeometry1D = \
                femviewprovider.view_element_geometry1D.VPElementGeometry1D
        if module.__name__ == "femguiobjects._ViewProviderFemElementGeometry2D":
            import femviewprovider.view_element_geometry2D
            module._ViewProviderFemElementGeometry2D = \
                femviewprovider.view_element_geometry2D.VPElementGeometry2D
        if module.__name__ == "femguiobjects._ViewProviderFemElementRotation1D":
            import femviewprovider.view_element_rotation1D
            module._ViewProviderFemElementRotation1D = \
                femviewprovider.view_element_rotation1D.VPElementRotation1D
        if module.__name__ == "femguiobjects._ViewProviderFemMaterial":
            import femviewprovider.view_material_common
            module._ViewProviderFemMaterial = femviewprovider.view_material_common.VPMaterialCommon
        if module.__name__ == "femguiobjects._ViewProviderFemMaterialMechanicalNonlinear":
            import femviewprovider.view_material_mechanicalnonlinear
            module._ViewProviderFemMaterialMechanicalNonlinear = \
                femviewprovider.view_material_mechanicalnonlinear.VPMaterialMechanicalNonlinear
        if module.__name__ == "femguiobjects._ViewProviderFemMaterialReinforced":
            import femviewprovider.view_material_reinforced
            module._ViewProviderFemMaterialReinforced = \
                femviewprovider.view_material_reinforced.VPMaterialReinforced
        if module.__name__ == "femguiobjects._ViewProviderFemMeshBoundaryLayer":
            import femviewprovider.view_mesh_boundarylayer
            module._ViewProviderFemMeshBoundaryLayer = \
                femviewprovider.view_mesh_boundarylayer.VPMeshBoundaryLayer
        if module.__name__ == "femguiobjects._ViewProviderFemMeshGmsh":
            import femviewprovider.view_mesh_gmsh
            module._ViewProviderFemMeshGmsh = femviewprovider.view_mesh_gmsh.VPMeshGmsh
        if module.__name__ == "femguiobjects._ViewProviderFemMeshGroup":
            import femviewprovider.view_mesh_group
            module._ViewProviderFemMeshGroup = femviewprovider.view_mesh_group.VPMeshGroup
        if module.__name__ == "femguiobjects._ViewProviderFemMeshRegion":
            import femviewprovider.view_mesh_region
            module._ViewProviderFemMeshRegion = femviewprovider.view_mesh_region.VPMeshRegion
        if module.__name__ == "femguiobjects._ViewProviderFemMeshResult":
            import femviewprovider.view_mesh_result
            module._ViewProviderFemMeshResult = femviewprovider.view_mesh_result.VPFemMeshResult
        if module.__name__ == "femguiobjects._ViewProviderFemResultMechanical":
            import femviewprovider.view_result_mechanical
            module._ViewProviderFemResultMechanical = \
                femviewprovider.view_result_mechanical.VPResultMechanical
        if module.__name__ == "femguiobjects._ViewProviderFemSolverCalculix":
            import femviewprovider.view_solver_ccxtools
            module._ViewProviderFemSolverCalculix = \
                femviewprovider.view_solver_ccxtools.VPSolverCcxTools

        if module.__name__ == "PyGui":
            module.__path__ = "PyGui"
        if module.__name__ == "PyGui._ViewProviderFemConstraintBodyHeatSource":
            import femviewprovider.view_constraint_bodyheatsource
            module.ViewProxy = \
                femviewprovider.view_constraint_bodyheatsource.VPConstraintBodyHeatSource
        if module.__name__ == "PyGui._ViewProviderFemConstraintElectrostaticPotential":
            import femviewprovider.view_constraint_electrostaticpotential
            module.ViewProxy = \
                femviewprovider.view_constraint_electrostaticpotential.VPConstraintElectroStaticPotential
        if module.__name__ == "PyGui._ViewProviderFemConstraintFlowVelocity":
            import femviewprovider.view_constraint_flowvelocity
            module.ViewProxy = \
                femviewprovider.view_constraint_flowvelocity.VPConstraintFlowVelocity
        if module.__name__ == "PyGui._ViewProviderFemConstraintInitialFlowVelocity":
            import femviewprovider.view_constraint_initialflowvelocity
            module.ViewProxy = \
                femviewprovider.view_constraint_initialflowvelocity.VPConstraintInitialFlowVelocity
        if module.__name__ == "PyGui._ViewProviderFemConstraintSelfWeight":
            import femviewprovider.view_constraint_selfweight
            module._ViewProviderFemConstraintSelfWeight = \
                femviewprovider.view_constraint_selfweight.VPConstraintSelfWeight
        if module.__name__ == "PyGui._ViewProviderFemElementFluid1D":
            import femviewprovider.view_element_fluid1D
            module._ViewProviderFemElementFluid1D = \
                femviewprovider.view_element_fluid1D.VPElementFluid1D
        if module.__name__ == "PyGui._ViewProviderFemElementGeometry1D":
            import femviewprovider.view_element_geometry1D
            module._ViewProviderFemElementGeometry1D = \
                femviewprovider.view_element_geometry1D.VPElementGeometry1D
        if module.__name__ == "PyGui._ViewProviderFemElementGeometry2D":
            import femviewprovider.view_element_geometry2D
            module._ViewProviderFemElementGeometry2D = \
                femviewprovider.view_element_geometry2D.VPElementGeometry2D
        if module.__name__ == "PyGui._ViewProviderFemElementRotation1D":
            import femviewprovider.view_element_rotation1D
            module._ViewProviderFemElementRotation1D = \
                femviewprovider.view_element_rotation1D.VPElementRotation1D
        if module.__name__ == "PyGui._ViewProviderFemMaterial":
            import femviewprovider.view_material_common
            module._ViewProviderFemMaterial = \
                femviewprovider.view_material_common.VPMaterialCommon
        if module.__name__ == "PyGui._ViewProviderFemMaterialMechanicalNonlinear":
            import femviewprovider.view_material_mechanicalnonlinear
            module._ViewProviderFemMaterialMechanicalNonlinear = \
                femviewprovider.view_material_mechanicalnonlinear.VPMaterialMechanicalNonlinear
        if module.__name__ == "PyGui._ViewProviderFemMeshBoundaryLayer":
            import femviewprovider.view_mesh_boundarylayer
            module._ViewProviderFemMeshBoundaryLayer = \
                femviewprovider.view_mesh_boundarylayer.VPMeshBoundaryLayer
        if module.__name__ == "PyGui._ViewProviderFemMeshGmsh":
            import femviewprovider.view_mesh_gmsh
            module._ViewProviderFemMeshGmsh = femviewprovider.view_mesh_gmsh.VPMeshGmsh
        if module.__name__ == "PyGui._ViewProviderFemMeshGroup":
            import femviewprovider.view_mesh_group
            module._ViewProviderFemMeshGroup = femviewprovider.view_mesh_group.VPMeshGroup
        if module.__name__ == "PyGui._ViewProviderFemMeshRegion":
            import femviewprovider.view_mesh_region
            module._ViewProviderFemMeshRegion = femviewprovider.view_mesh_region.VPMeshRegion
        if module.__name__ == "PyGui._ViewProviderFemMeshResult":
            import femviewprovider.view_mesh_result
            module._ViewProviderFemMeshResult = femviewprovider.view_mesh_result.VPFemMeshResult
        if module.__name__ == "PyGui._ViewProviderFemResultMechanical":
            import femviewprovider.view_result_mechanical
            module._ViewProviderFemResultMechanical = \
                femviewprovider.view_result_mechanical.VPResultMechanical
        if module.__name__ == "PyGui._ViewProviderFemSolverCalculix":
            import femviewprovider.view_solver_ccxtools
            module._ViewProviderFemSolverCalculix = \
                femviewprovider.view_solver_ccxtools.VPSolverCcxTools
        if module.__name__ == "PyGui._ViewProviderFemSolverZ88":
            import femsolver.z88.solver
            module._ViewProviderFemSolverZ88 = femsolver.z88.solver.ViewProxy

        if module.__name__ == "PyGui._ViewProviderFemBeamSection":
            import femviewprovider.view_element_geometry1D
            module._ViewProviderFemBeamSection = \
                femviewprovider.view_element_geometry1D.VPElementGeometry1D
        if module.__name__ == "PyGui._ViewProviderFemFluidSection":
            import femviewprovider.view_element_fluid1D
            module._ViewProviderFemFluidSection = \
                femviewprovider.view_element_fluid1D.VPElementFluid1D
        if module.__name__ == "PyGui._ViewProviderFemShellThickness":
            import femviewprovider.view_element_geometry2D
            module._ViewProviderFemShellThickness = \
                femviewprovider.view_element_geometry2D.VPElementGeometry2D

        if module.__name__ == "_ViewProviderFemBeamSection":
            import femviewprovider.view_element_geometry1D
            module._ViewProviderFemBeamSection = \
                femviewprovider.view_element_geometry1D.VPElementGeometry1D
        if module.__name__ == "_ViewProviderFemConstraintSelfWeight":
            import femviewprovider.view_constraint_selfweight
            module._ViewProviderFemConstraintSelfWeight = \
                femviewprovider.view_constraint_selfweight.VPConstraintSelfWeight
        if module.__name__ == "_ViewProviderFemMaterial":
            import femviewprovider.view_material_common
            module._ViewProviderFemMaterial = \
                femviewprovider.view_material_common.VPMaterialCommon
        if module.__name__ == "_ViewProviderFemMaterialMechanicalNonlinear":
            import femviewprovider.view_material_mechanicalnonlinear
            module._ViewProviderFemMaterialMechanicalNonlinear = \
                femviewprovider.view_material_mechanicalnonlinear.VPMaterialMechanicalNonlinear
        if module.__name__ == "_ViewProviderFemMeshGmsh":
            import femviewprovider.view_mesh_gmsh
            module._ViewProviderFemMeshGmsh = femviewprovider.view_mesh_gmsh.VPMeshGmsh
        if module.__name__ == "_ViewProviderFemMeshGroup":
            import femviewprovider.view_mesh_group
            module._ViewProviderFemMeshGroup = femviewprovider.view_mesh_group.VPMeshGroup
        if module.__name__ == "_ViewProviderFemMeshRegion":
            import femviewprovider.view_mesh_region
            module._ViewProviderFemMeshRegion = femviewprovider.view_mesh_region.VPMeshRegion
        if module.__name__ == "_ViewProviderFemResultMechanical":
            import femviewprovider.view_result_mechanical
            module._ViewProviderFemResultMechanical = \
                femviewprovider.view_result_mechanical.VPResultMechanical
        if module.__name__ == "_ViewProviderFemShellThickness":
            import femviewprovider.view_element_geometry2D
            module._ViewProviderFemShellThickness = \
                femviewprovider.view_element_geometry2D.VPElementGeometry2D
        if module.__name__ == "_ViewProviderFemSolverCalculix":
            import femviewprovider.view_solver_ccxtools
            module._ViewProviderFemSolverCalculix = \
                femviewprovider.view_solver_ccxtools.VPSolverCcxTools
        if module.__name__ == "_ViewProviderFemSolverZ88":
            import femsolver.z88.solver
            module._ViewProviderFemSolverZ88 = femsolver.z88.solver.ViewProxy

        if module.__name__ == "_ViewProviderFemMechanicalResult":
            import femviewprovider.view_result_mechanical
            module._ViewProviderFemMechanicalResult = \
                femviewprovider.view_result_mechanical.VPResultMechanical
        if module.__name__ == "ViewProviderFemResult":
            import femviewprovider.view_result_mechanical
            module.ViewProviderFemResult = \
                femviewprovider.view_result_mechanical.VPResultMechanical
        if module.__name__ == "_ViewProviderMechanicalMaterial":
            import femviewprovider.view_material_common
            module._ViewProviderMechanicalMaterial = \
                femviewprovider.view_material_common.VPMaterialCommon

        return None


"""
possible entries in the old files:
(the class name in the old file does not matter, we ever only had one class per module)

further renaming objects
module="femsolver.elmer.equations.fluxsolver"
see App migrate because object class and viewprovider class are in the same module

fourth big moving
renaming class and module names in femobjects
TODO add link to commit before the first commit
module="femguiobjects._ViewProviderFemConstraintBodyHeatSource"
module="femguiobjects._ViewProviderFemConstraintElectrostaticPotential"
module="femguiobjects._ViewProviderFemConstraintFlowVelocity"
module="femguiobjects._ViewProviderFemConstraintInitialFlowVelocity"
module="femguiobjects._ViewProviderFemConstraintSelfWeight"
module="femguiobjects._ViewProviderFemConstraintTie"
module="femguiobjects._ViewProviderFemElementFluid1D"
module="femguiobjects._ViewProviderFemElementGeometry1D"
module="femguiobjects._ViewProviderFemElementGeometry2D"
module="femguiobjects._ViewProviderFemElementRotation1D"
module="femguiobjects._ViewProviderFemMaterial"
module="femguiobjects._ViewProviderFemMaterialMechanicalNonlinear"
module="femguiobjects._ViewProviderFemMaterialReinforced"
module="femguiobjects._ViewProviderFemMeshBoundaryLayer"
module="femguiobjects._ViewProviderFemMeshGmsh"
module="femguiobjects._ViewProviderFemMeshGroup"
module="femguiobjects._ViewProviderFemMeshRegion"
module="femguiobjects._ViewProviderFemMeshResult"
module="femguiobjects._ViewProviderFemResultMechanical"
module="femguiobjects._ViewProviderFemSolverCalculix"

third big moving
from PyGui to femguiobjects, following the parent commit
https://github.com/FreeCAD/FreeCAD/tree/07ae0e56c4/src/Mod/Fem/PyGui
module="PyGui._ViewProviderFemConstraintBodyHeatSource"
module="PyGui._ViewProviderFemConstraintElectrostaticPotential"
module="PyGui._ViewProviderFemConstraintFlowVelocity"
module="PyGui._ViewProviderFemConstraintInitialFlowVelocity"
module="PyGui._ViewProviderFemConstraintSelfWeight"
module="PyGui._ViewProviderFemElementFluid1D"
module="PyGui._ViewProviderFemElementGeometry1D"
module="PyGui._ViewProviderFemElementGeometry2D"
module="PyGui._ViewProviderFemElementRotation1D"
module="PyGui._ViewProviderFemMaterial"
module="PyGui._ViewProviderFemMaterialMechanicalNonlinear"
module="PyGui._ViewProviderFemMeshBoundaryLayer"
module="PyGui._ViewProviderFemMeshGmsh"
module="PyGui._ViewProviderFemMeshGroup"
module="PyGui._ViewProviderFemMeshRegion"
module="PyGui._ViewProviderFemMeshResult"
module="PyGui._ViewProviderFemResultMechanical"
module="PyGui._ViewProviderFemSolverCalculix"
module="PyGui._ViewProviderFemSolverZ88"

renamed between the second and third big moveings
module="PyGui._ViewProviderFemBeamSection"
module="PyGui._ViewProviderFemFluidSection"
module="PyGui._ViewProviderFemShellThickness"

second big moveing
into PyObjects, following the parent commit
https://github.com/FreeCAD/FreeCAD/tree/7f884e8bff/src/Mod/Fem
module="_ViewProviderFemBeamSection"
module="_ViewProviderFemConstraintSelfWeight"
module="_ViewProviderFemMaterial"
module="_ViewProviderFemMaterialMechanicalNonlinear"
module="_ViewProviderFemMeshGmsh"
module="_ViewProviderFemMeshGroup"
module="_ViewProviderFemMeshRegion"
module="_ViewProviderFemResultMechanical"
module="_ViewProviderFemShellThickness"
module="_ViewProviderFemSolverCalculix"
module="_ViewProviderFemSolverZ88"

renamed between the first and second big moveings
module="_ViewProviderFemMechanicalResult"
module="ViewProviderFemResult"
module="_ViewProviderMechanicalMaterial"

first big moving
split modules from one module into make, obj class, vp class, command
new obj class module names had a _
following the parent commit of the first split commit
https://github.com/FreeCAD/FreeCAD/tree/c3328d6b4e/src/Mod/Fem
in this modules there where object class and viewprovider class together
# see migrate App
module="FemBeamSection"
module="FemShellThickness"
module="MechanicalAnalysis"
module="MechanicalMaterial"
"""
