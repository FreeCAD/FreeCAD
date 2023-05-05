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
""" Class and methods to migrate old FEM App objects

see module end as well as forum topic
https://forum.freecad.org/viewtopic.php?&t=46218
"""

__title__ = "FEM class and methods that migrates old App objects"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

import FreeCAD


class FemMigrateApp(object):

    def find_module(self, fullname, path):

        if fullname == "femsolver.elmer.equations":
            return self
        if fullname == "femsolver.elmer.equations.fluxsolver":
            return self

        if fullname == "femobjects":
            return self
        if fullname == "femobjects._FemConstraintBodyHeatSource":
            return self
        if fullname == "femobjects._FemConstraintElectrostaticPotential":
            return self
        if fullname == "femobjects._FemConstraintFlowVelocity":
            return self
        if fullname == "femobjects._FemConstraintInitialFlowVelocity":
            return self
        if fullname == "femobjects._FemConstraintSelfWeight":
            return self
        if fullname == "femobjects._FemConstraintTie":
            return self
        if fullname == "femobjects._FemElementFluid1D":
            return self
        if fullname == "femobjects._FemElementGeometry1D":
            return self
        if fullname == "femobjects._FemElementGeometry2D":
            return self
        if fullname == "femobjects._FemElementRotation1D":
            return self
        if fullname == "femobjects._FemMaterial":
            return self
        if fullname == "femobjects._FemMaterialMechanicalNonlinear":
            return self
        if fullname == "femobjects._FemMaterialReinforced":
            return self
        if fullname == "femobjects._FemMeshBoundaryLayer":
            return self
        if fullname == "femobjects._FemMeshGmsh":
            return self
        if fullname == "femobjects._FemMeshGroup":
            return self
        if fullname == "femobjects._FemMeshRegion":
            return self
        if fullname == "femobjects._FemMeshResult":
            return self
        if fullname == "femobjects._FemResultMechanical":
            return self
        if fullname == "femobjects._FemSolverCalculix":
            return self

        if fullname == "PyObjects":
            return self
        if fullname == "PyObjects._FemConstraintBodyHeatSource":
            return self
        if fullname == "PyObjects._FemConstraintElectrostaticPotential":
            return self
        if fullname == "PyObjects._FemConstraintFlowVelocity":
            return self
        if fullname == "PyObjects._FemConstraintInitialFlowVelocity":
            return self
        if fullname == "PyObjects._FemConstraintSelfWeight":
            return self
        if fullname == "PyObjects._FemElementFluid1D":
            return self
        if fullname == "PyObjects._FemElementGeometry1D":
            return self
        if fullname == "PyObjects._FemElementGeometry2D":
            return self
        if fullname == "PyObjects._FemElementRotation1D":
            return self
        if fullname == "PyObjects._FemMaterial":
            return self
        if fullname == "PyObjects._FemMaterialMechanicalNonlinear":
            return self
        if fullname == "PyObjects._FemMeshBoundaryLayer":
            return self
        if fullname == "PyObjects._FemMeshGmsh":
            return self
        if fullname == "PyObjects._FemMeshGroup":
            return self
        if fullname == "PyObjects._FemMeshRegion":
            return self
        if fullname == "PyObjects._FemMeshResult":
            return self
        if fullname == "PyObjects._FemResultMechanical":
            return self
        if fullname == "PyObjects._FemSolverCalculix":
            return self
        if fullname == "PyObjects._FemSolverZ88":
            return self

        if fullname == "PyObjects._FemBeamSection":
            return self
        if fullname == "PyObjects._FemFluidSection":
            return self
        if fullname == "PyObjects._FemShellThickness":
            return self

        if fullname == "_FemBeamSection":
            return self
        if fullname == "_FemConstraintSelfWeight":
            return self
        if fullname == "_FemMaterial":
            return self
        if fullname == "_FemMaterialMechanicalNonlinear":
            return self
        if fullname == "_FemMeshGmsh":
            return self
        if fullname == "_FemMeshGroup":
            return self
        if fullname == "_FemMeshRegion":
            return self
        if fullname == "_FemResultMechanical":
            return self
        if fullname == "_FemShellThickness":
            return self
        if fullname == "_FemSolverCalculix":
            return self
        if fullname == "_FemSolverZ88":
            return self

        if fullname == "_FemMechanicalResult":
            return self
        if fullname == "FemResult":
            return self
        if fullname == "_MechanicalMaterial":
            return self

        if fullname == "FemBeamSection":
            return self
        if fullname == "FemShellThickness":
            return self
        if fullname == "MechanicalAnalysis":
            return self
        if fullname == "MechanicalMaterial":
            return self
        return None

    def create_module(self, spec):
        return None

    def exec_module(self, module):
        return self.load_module(module)

    def load_module(self, module):

        if module.__name__ == "femsolver.elmer.equations":
            return self
        if module.__name__ == "femsolver.elmer.equations.fluxsolver":
            import femsolver.elmer.equations.flux
            module.Proxy = femsolver.elmer.equations.flux.Proxy
            if FreeCAD.GuiUp:
                module.ViewProxy = femsolver.elmer.equations.flux.ViewProxy

        if module.__name__ == "femobjects":
            module.__path__ = "femobjects"
        if module.__name__ == "femobjects._FemConstraintBodyHeatSource":
            import femobjects.constraint_bodyheatsource
            module.Proxy = femobjects.constraint_bodyheatsource.ConstraintBodyHeatSource
        if module.__name__ == "femobjects._FemConstraintElectrostaticPotential":
            import femobjects.constraint_electrostaticpotential
            module.Proxy = \
                femobjects.constraint_electrostaticpotential.ConstraintElectrostaticPotential
        if module.__name__ == "femobjects._FemConstraintFlowVelocity":
            import femobjects.constraint_flowvelocity
            module.Proxy = femobjects.constraint_flowvelocity.ConstraintFlowVelocity
        if module.__name__ == "femobjects._FemConstraintInitialFlowVelocity":
            import femobjects.constraint_initialflowvelocity
            module.Proxy = femobjects.constraint_initialflowvelocity.ConstraintInitialFlowVelocity
        if module.__name__ == "femobjects._FemConstraintSelfWeight":
            import femobjects.constraint_selfweight
            module._FemConstraintSelfWeight = femobjects.constraint_selfweight.ConstraintSelfWeight
        if module.__name__ == "femobjects._FemConstraintTie":
            import femobjects.constraint_tie
            module._FemConstraintTie = femobjects.constraint_tie.ConstraintTie
        if module.__name__ == "femobjects._FemElementFluid1D":
            import femobjects.element_fluid1D
            module._FemElementFluid1D = femobjects.element_fluid1D.ElementFluid1D
        if module.__name__ == "femobjects._FemElementGeometry1D":
            import femobjects.element_geometry1D
            module._FemElementGeometry1D = femobjects.element_geometry1D.ElementGeometry1D
        if module.__name__ == "femobjects._FemElementGeometry2D":
            import femobjects.element_geometry2D
            module._FemElementGeometry2D = femobjects.element_geometry2D.ElementGeometry2D
        if module.__name__ == "femobjects._FemElementRotation1D":
            import femobjects.element_rotation1D
            module._FemElementRotation1D = femobjects.element_rotation1D.ElementRotation1D
        if module.__name__ == "femobjects._FemMaterial":
            import femobjects.material_common
            module._FemMaterial = femobjects.material_common.MaterialCommon
        if module.__name__ == "femobjects._FemMaterialMechanicalNonlinear":
            import femobjects.material_mechanicalnonlinear
            module._FemMaterialMechanicalNonlinear = \
                femobjects.material_mechanicalnonlinear.MaterialMechanicalNonlinear
        if module.__name__ == "femobjects._FemMaterialReinforced":
            import femobjects.material_reinforced
            module._FemMaterialReinforced = femobjects.material_reinforced.MaterialReinforced
        if module.__name__ == "femobjects._FemMeshBoundaryLayer":
            import femobjects.mesh_boundarylayer
            module._FemMeshBoundaryLayer = femobjects.mesh_boundarylayer.MeshBoundaryLayer
        if module.__name__ == "femobjects._FemMeshGmsh":
            import femobjects.mesh_gmsh
            module._FemMeshGmsh = femobjects.mesh_gmsh.MeshGmsh
        if module.__name__ == "femobjects._FemMeshGroup":
            import femobjects.mesh_group
            module._FemMeshGroup = femobjects.mesh_group.MeshGroup
        if module.__name__ == "femobjects._FemMeshRegion":
            import femobjects.mesh_region
            module._FemMeshRegion = femobjects.mesh_region.MeshRegion
        if module.__name__ == "femobjects._FemMeshResult":
            import femobjects.mesh_result
            module._FemMeshResult = femobjects.mesh_result.MeshResult
        if module.__name__ == "femobjects._FemResultMechanical":
            import femobjects.result_mechanical
            module._FemResultMechanical = femobjects.result_mechanical.ResultMechanical
        if module.__name__ == "femobjects._FemSolverCalculix":
            import femobjects.solver_ccxtools
            module._FemSolverCalculix = femobjects.solver_ccxtools.SolverCcxTools

        if module.__name__ == "PyObjects":
            module.__path__ = "PyObjects"
        if module.__name__ == "PyObjects._FemConstraintBodyHeatSource":
            import femobjects.constraint_bodyheatsource
            module.Proxy = femobjects.constraint_bodyheatsource.ConstraintBodyHeatSource
        if module.__name__ == "PyObjects._FemConstraintElectrostaticPotential":
            import femobjects.constraint_electrostaticpotential
            module.Proxy = \
                femobjects.constraint_electrostaticpotential.ConstraintElectrostaticPotential
        if module.__name__ == "PyObjects._FemConstraintFlowVelocity":
            import femobjects.constraint_flowvelocity
            module.Proxy = femobjects.constraint_flowvelocity.ConstraintFlowVelocity
        if module.__name__ == "PyObjects._FemConstraintInitialFlowVelocity":
            import femobjects.constraint_initialflowvelocity
            module.Proxy = femobjects.constraint_initialflowvelocity.ConstraintInitialFlowVelocity
        if module.__name__ == "PyObjects._FemConstraintSelfWeight":
            import femobjects.constraint_selfweight
            module._FemConstraintSelfWeight = femobjects.constraint_selfweight.ConstraintSelfWeight
        if module.__name__ == "PyObjects._FemElementFluid1D":
            import femobjects.element_fluid1D
            module._FemElementFluid1D = femobjects.element_fluid1D.ElementFluid1D
        if module.__name__ == "PyObjects._FemElementGeometry1D":
            import femobjects.element_geometry1D
            module._FemElementGeometry1D = femobjects.element_geometry1D.ElementGeometry1D
        if module.__name__ == "PyObjects._FemElementGeometry2D":
            import femobjects.element_geometry2D
            module._FemElementGeometry2D = femobjects.element_geometry2D.ElementGeometry2D
        if module.__name__ == "PyObjects._FemElementRotation1D":
            import femobjects.element_rotation1D
            module._FemElementRotation1D = femobjects.element_rotation1D.ElementRotation1D
        if module.__name__ == "PyObjects._FemMaterial":
            import femobjects.material_common
            module._FemMaterial = femobjects.material_common.MaterialCommon
        if module.__name__ == "PyObjects._FemMaterialMechanicalNonlinear":
            import femobjects.material_mechanicalnonlinear
            module._FemMaterialMechanicalNonlinear = \
                femobjects.material_mechanicalnonlinear.MaterialMechanicalNonlinear
        if module.__name__ == "PyObjects._FemMeshBoundaryLayer":
            import femobjects.mesh_boundarylayer
            module._FemMeshBoundaryLayer = femobjects.mesh_boundarylayer.MeshBoundaryLayer
        if module.__name__ == "PyObjects._FemMeshGmsh":
            import femobjects.mesh_gmsh
            module._FemMeshGmsh = femobjects.mesh_gmsh.MeshGmsh
        if module.__name__ == "PyObjects._FemMeshGroup":
            import femobjects.mesh_group
            module._FemMeshGroup = femobjects.mesh_group.MeshGroup
        if module.__name__ == "PyObjects._FemMeshRegion":
            import femobjects.mesh_region
            module._FemMeshRegion = femobjects.mesh_region.MeshRegion
        if module.__name__ == "PyObjects._FemMeshResult":
            import femobjects.mesh_result
            module._FemMeshResult = femobjects.mesh_result.MeshResult
        if module.__name__ == "PyObjects._FemResultMechanical":
            import femobjects.result_mechanical
            module._FemResultMechanical = femobjects.result_mechanical.ResultMechanical
        if module.__name__ == "PyObjects._FemSolverCalculix":
            import femobjects.solver_ccxtools
            module._FemSolverCalculix = femobjects.solver_ccxtools.SolverCcxTools
        if module.__name__ == "PyObjects._FemSolverZ88":
            import femsolver.z88.solver
            module._FemSolverZ88 = femsolver.z88.solver.Proxy

        if module.__name__ == "PyObjects._FemBeamSection":
            import femobjects.element_geometry1D
            module._FemBeamSection = femobjects.element_geometry1D.ElementGeometry1D
        if module.__name__ == "PyObjects._FemFluidSection":
            import femobjects.element_fluid1D
            module._FemFluidSection = femobjects.element_fluid1D.ElementFluid1D
        if module.__name__ == "PyObjects._FemShellThickness":
            import femobjects.element_geometry2D
            module._FemShellThickness = femobjects.element_geometry2D.ElementGeometry2D

        if module.__name__ == "_FemBeamSection":
            import femobjects.element_geometry1D
            module._FemBeamSection = femobjects.element_geometry1D.ElementGeometry1D
        if module.__name__ == "_FemConstraintSelfWeight":
            import femobjects.constraint_selfweight
            module._FemConstraintSelfWeight = femobjects.constraint_selfweight.ConstraintSelfWeight
        if module.__name__ == "_FemMaterial":
            import femobjects.material_common
            module._FemMaterial = femobjects.material_common.MaterialCommon
        if module.__name__ == "_FemMaterialMechanicalNonlinear":
            import femobjects.material_mechanicalnonlinear
            module._FemMaterialMechanicalNonlinear = \
                femobjects.material_mechanicalnonlinear.MaterialMechanicalNonlinear
        if module.__name__ == "_FemMeshGmsh":
            import femobjects.mesh_gmsh
            module._FemMeshGmsh = femobjects.mesh_gmsh.MeshGmsh
        if module.__name__ == "_FemMeshGroup":
            import femobjects.mesh_group
            module._FemMeshGroup = femobjects.mesh_group.MeshGroup
        if module.__name__ == "_FemMeshRegion":
            import femobjects.mesh_region
            module._FemMeshRegion = femobjects.mesh_region.MeshRegion
        if module.__name__ == "_FemResultMechanical":
            import femobjects.result_mechanical
            module._FemResultMechanical = femobjects.result_mechanical.ResultMechanical
        if module.__name__ == "_FemShellThickness":
            import femobjects.element_geometry2D
            module._FemShellThickness = femobjects.element_geometry2D.ElementGeometry2D
        if module.__name__ == "_FemSolverCalculix":
            import femobjects.solver_ccxtools
            module._FemSolverCalculix = femobjects.solver_ccxtools.SolverCcxTools
        if module.__name__ == "_FemSolverZ88":
            import femsolver.z88.solver
            module._FemSolverZ88 = femsolver.z88.solver.Proxy

        if module.__name__ == "_FemMechanicalResult":
            import femobjects.result_mechanical
            module._FemMechanicalResult = femobjects.result_mechanical.ResultMechanical
        if module.__name__ == "FemResult":
            import femobjects.result_mechanical
            module.FemResult = femobjects.result_mechanical.ResultMechanical
        if module.__name__ == "_MechanicalMaterial":
            import femobjects.material_common
            module._MechanicalMaterial = femobjects.material_common.MaterialCommon

        if module.__name__ == "FemBeamSection":
            import femobjects.element_geometry1D
            module._FemBeamSection = femobjects.element_geometry1D.ElementGeometry1D
            if FreeCAD.GuiUp:
                import femviewprovider.view_element_geometry1D
                module._ViewProviderFemBeamSection = \
                    femviewprovider.view_element_geometry1D.VPElementGeometry1D
        if module.__name__ == "FemShellThickness":
            import femobjects.element_geometry2D
            module._FemShellThickness = femobjects.element_geometry2D.ElementGeometry2D
            if FreeCAD.GuiUp:
                import femviewprovider.view_element_geometry2D
                module._ViewProviderFemShellThickness = \
                    femviewprovider.view_element_geometry2D.VPElementGeometry2D
        if module.__name__ == "MechanicalAnalysis":
            import femobjects.base_fempythonobject
            module._FemAnalysis = femobjects.base_fempythonobject.BaseFemPythonObject
            if FreeCAD.GuiUp:
                import femviewprovider.view_base_femobject
                module._ViewProviderFemAnalysis = \
                    femviewprovider.view_base_femobject.VPBaseFemObject
        if module.__name__ == "MechanicalMaterial":
            import femobjects.material_common
            module._MechanicalMaterial = femobjects.material_common.MaterialCommon
            if FreeCAD.GuiUp:
                import femviewprovider.view_material_common
                module._ViewProviderMechanicalMaterial = \
                    femviewprovider.view_material_common.VPMaterialCommon
        return None


"""
possible entries in the old files:
(the class name in the old file does not matter, we ever only had one class per module)

further renaming objects
module="femsolver.elmer.equations.fluxsolver"
in this modules object class and viewprovider class are in same module


fourth big moving
renaming class and module names in femobjects
TODO add link to commit before the first commit
module="femobjects._FemConstraintBodyHeatSource"
module="femobjects._FemConstraintElectrostaticPotential"
module="femobjects._FemConstraintFlowVelocity"
module="femobjects._FemConstraintInitialFlowVelocity"
module="femobjects._FemConstraintSelfWeight"
module="femobjects._FemConstraintTie"
module="femobjects._FemElementFluid1D"
module="femobjects._FemElementGeometry1D"
module="femobjects._FemElementGeometry2D"
module="femobjects._FemElementRotation1D"
module="femobjects._FemMaterial"
module="femobjects._FemMaterialMechanicalNonlinear"
module="femobjects._FemMaterialReinforced"
module="femobjects._FemMeshBoundaryLayer"
module="femobjects._FemMeshGmsh"
module="femobjects._FemMeshGroup"
module="femobjects._FemMeshRegion"
module="femobjects._FemMeshResult"
module="femobjects._FemResultMechanical"
module="femobjects._FemSolverCalculix"

third big moving
from PyObjects to femobjects, following the parent commit
https://github.com/FreeCAD/FreeCAD/tree/07ae0e56c4/src/Mod/Fem/PyObjects
module="PyObjects._FemConstraintBodyHeatSource"
module="PyObjects._FemConstraintElectrostaticPotential"
module="PyObjects._FemConstraintFlowVelocity"
module="PyObjects._FemConstraintInitialFlowVelocity"
module="PyObjects._FemConstraintSelfWeight"
module="PyObjects._FemElementFluid1D"
module="PyObjects._FemElementGeometry1D"
module="PyObjects._FemElementGeometry2D"
module="PyObjects._FemElementRotation1D"
module="PyObjects._FemMaterial"
module="PyObjects._FemMaterialMechanicalNonlinear"
module="PyObjects._FemMeshBoundaryLayer"
module="PyObjects._FemMeshGmsh"
module="PyObjects._FemMeshGroup"
module="PyObjects._FemMeshRegion"
module="PyObjects._FemMeshResult"
module="PyObjects._FemResultMechanical"
module="PyObjects._FemSolverCalculix"
module="PyObjects._FemSolverZ88"

renamed between the second and third big moveings
module="PyObjects._FemBeamSection"
module="PyObjects._FemFluidSection"
module="PyObjects._FemShellThickness"

second big moveing
into PyObjects, following the parent commit
https://github.com/FreeCAD/FreeCAD/tree/7f884e8bff/src/Mod/Fem
module="_FemBeamSection"
module="_FemConstraintSelfWeight"
module="_FemMaterial"
module="_FemMaterialMechanicalNonlinear."
module="_FemMeshGmsh"
module="_FemMeshGroup"
module="_FemMeshRegion"
module="_FemResultMechanical"
module="_FemShellThickness"
module="_FemSolverCalculix"
module="_FemSolverZ88"

renamed between the first and second big moveings
module="_FemMechanicalResult"
module="FemResult"
module="_MechanicalMaterial"

first big moving
split modules from one module into make, obj class, vp class, command
new obj class module names had a _
following the parent commit of the first split commit
https://github.com/FreeCAD/FreeCAD/tree/c3328d6b4e/src/Mod/Fem
in this modules there where object class and viewprovider class together
module="FemBeamSection"
module="FemShellThickness"
module="MechanicalAnalysis"
module="MechanicalMaterial"
"""
