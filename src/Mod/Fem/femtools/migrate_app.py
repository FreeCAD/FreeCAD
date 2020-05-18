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
https://forum.freecadweb.org/viewtopic.php?&t=46218
"""

__title__ = "migrate app"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

import FreeCAD


class FemMigrateApp(object):

    def find_module(self, fullname, path):
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
        if module.__name__ == "PyObjects":
            module.__path__ = "PyObjects"

        if module.__name__ == "PyObjects._FemConstraintBodyHeatSource":
            import femobjects._FemConstraintBodyHeatSource
            module._FemConstraintBodyHeatSource = femobjects._FemConstraintBodyHeatSource.Proxy
        if module.__name__ == "PyObjects._FemConstraintElectrostaticPotential":
            import femobjects._FemConstraintElectrostaticPotential
            module._FemConstraintElectrostaticPotential = femobjects._FemConstraintElectrostaticPotential.Proxy
        if module.__name__ == "PyObjects._FemConstraintFlowVelocity":
            import femobjects._FemConstraintFlowVelocity
            module._FemConstraintFlowVelocity = femobjects._FemConstraintFlowVelocity.Proxy
        if module.__name__ == "PyObjects._FemConstraintInitialFlowVelocity":
            import femobjects._FemConstraintInitialFlowVelocity
            module._FemConstraintInitialFlowVelocity = femobjects._FemConstraintInitialFlowVelocity.Proxy
        if module.__name__ == "PyObjects._FemConstraintSelfWeight":
            import femobjects._FemConstraintSelfWeight
            module._FemConstraintSelfWeight = femobjects._FemConstraintSelfWeight._FemConstraintSelfWeight
        if module.__name__ == "PyObjects._FemElementFluid1D":
            import femobjects._FemElementFluid1D
            module._FemElementFluid1D = femobjects._FemElementFluid1D._FemElementFluid1D
        if module.__name__ == "PyObjects._FemElementGeometry1D":
            import femobjects._FemElementGeometry1D
            module._FemElementGeometry1D = femobjects._FemElementGeometry1D._FemElementGeometry1D
        if module.__name__ == "PyObjects._FemElementGeometry2D":
            import femobjects._FemElementGeometry2D
            module._FemElementGeometry2D = femobjects._FemElementGeometry2D._FemElementGeometry2D
        if module.__name__ == "PyObjects._FemElementRotation1D":
            import femobjects._FemElementRotation1D
            module._FemElementRotation1D = femobjects._FemElementRotation1D._FemElementRotation1D
        if module.__name__ == "PyObjects._FemMaterial":
            import femobjects._FemMaterial
            module._FemMaterial = femobjects._FemMaterial._FemMaterial
        if module.__name__ == "PyObjects._FemMaterialMechanicalNonlinear":
            import femobjects._FemMaterialMechanicalNonlinear
            module._FemMaterialMechanicalNonlinear = femobjects._FemMaterialMechanicalNonlinear._FemMaterialMechanicalNonlinear
        if module.__name__ == "PyObjects._FemMeshBoundaryLayer":
            import femobjects._FemMeshBoundaryLayer
            module._FemMeshBoundaryLayer = femobjects._FemMeshBoundaryLayer._FemMeshBoundaryLayer
        if module.__name__ == "PyObjects._FemMeshGmsh":
            import femobjects._FemMeshGmsh
            module._FemMeshGmsh = femobjects._FemMeshGmsh._FemMeshGmsh
        if module.__name__ == "PyObjects._FemMeshGroup":
            import femobjects._FemMeshGroup
            module._FemMeshGroup = femobjects._FemMeshGroup._FemMeshGroup
        if module.__name__ == "PyObjects._FemMeshRegion":
            import femobjects._FemMeshRegion
            module._FemMeshRegion = femobjects._FemMeshRegion._FemMeshRegion
        if module.__name__ == "PyObjects._FemMeshResult":
            import femobjects._FemMeshResult
            module._FemMeshResult = femobjects._FemMeshResult._FemMeshResult
        if module.__name__ == "PyObjects._FemResultMechanical":
            import femobjects._FemResultMechanical
            module._FemResultMechanical = femobjects._FemResultMechanical._FemResultMechanical
        if module.__name__ == "PyObjects._FemSolverCalculix":
            import femobjects._FemSolverCalculix
            module._FemSolverCalculix = femobjects._FemSolverCalculix._FemSolverCalculix

        if module.__name__ == "PyObjects._FemBeamSection":
            import femobjects._FemElementGeometry1D
            module._FemBeamSection = femobjects._FemElementGeometry1D._FemElementGeometry1D
        if module.__name__ == "PyObjects._FemFluidSection":
            import femobjects._FemElementFluid1D
            module._FemFluidSection = femobjects._FemElementFluid1D._FemElementFluid1D
        if module.__name__ == "PyObjects._FemShellThickness":
            import femobjects._FemElementGeometry2D
            module._FemShellThickness = femobjects._FemElementGeometry2D._FemElementGeometry2D

        if module.__name__ == "_FemBeamSection":
            import femobjects._FemElementGeometry1D
            module._FemBeamSection = femobjects._FemElementGeometry1D._FemElementGeometry1D
        if module.__name__ == "_FemConstraintSelfWeight":
            import femobjects._FemConstraintSelfWeight
            module._FemConstraintSelfWeight = femobjects._FemConstraintSelfWeight._FemConstraintSelfWeight
        if module.__name__ == "_FemMaterial":
            import femobjects._FemMaterial
            module._FemMaterial = femobjects._FemMaterial._FemMaterial
        if module.__name__ == "_FemMaterialMechanicalNonlinear":
            import femobjects._FemMaterialMechanicalNonlinear
            module._FemMaterialMechanicalNonlinear = femobjects._FemMaterialMechanicalNonlinear._FemMaterialMechanicalNonlinear
        if module.__name__ == "_FemMeshGmsh":
            import femobjects._FemMeshGmsh
            module._FemMeshGmsh = femobjects._FemMeshGmsh._FemMeshGmsh
        if module.__name__ == "_FemMeshGroup":
            import femobjects._FemMeshGroup
            module._FemMeshGroup = femobjects._FemMeshGroup._FemMeshGroup
        if module.__name__ == "_FemMeshRegion":
            import femobjects._FemMeshRegion
            module._FemMeshRegion = femobjects._FemMeshRegion._FemMeshRegion
        if module.__name__ == "_FemResultMechanical":
            import femobjects._FemResultMechanical
            module._FemResultMechanical = femobjects._FemResultMechanical._FemResultMechanical
        if module.__name__ == "_FemShellThickness":
            import femobjects._FemElementGeometry2D
            module._FemShellThickness = femobjects._FemElementGeometry2D._FemElementGeometry2D
        if module.__name__ == "_FemSolverCalculix":
            import femobjects._FemSolverCalculix
            module._FemSolverCalculix = femobjects._FemSolverCalculix._FemSolverCalculix
        if module.__name__ == "_FemSolverZ88":
            import femsolver.z88.solver
            module._FemSolverZ88 = femsolver.z88.solver.Proxy

        if module.__name__ == "_FemMechanicalResult":
            import femobjects._FemResultMechanical
            module._FemMechanicalResult = femobjects._FemResultMechanical._FemResultMechanical
        if module.__name__ == "FemResult":
            import femobjects._FemResultMechanical
            module.FemResult = femobjects._FemResultMechanical._FemResultMechanical
        if module.__name__ == "_MechanicalMaterial":
            import femobjects._FemMaterial
            module._MechanicalMaterial = femobjects._FemMaterial._FemMaterial

        if module.__name__ == "FemBeamSection":
            import femobjects._FemElementGeometry1D
            module._FemBeamSection = femobjects._FemElementGeometry1D._FemElementGeometry1D
            if FreeCAD.GuiUp:
                import femguiobjects._ViewProviderFemElementGeometry1D
                module._ViewProviderFemBeamSection = femguiobjects._ViewProviderFemElementGeometry1D._ViewProviderFemElementGeometry1D
        if module.__name__ == "FemShellThickness":
            import femobjects._FemElementGeometry2D
            module._FemShellThickness = femobjects._FemElementGeometry2D._FemElementGeometry2D
            if FreeCAD.GuiUp:
                import femguiobjects._ViewProviderFemElementGeometry2D
                module._ViewProviderFemShellThickness = femguiobjects._ViewProviderFemElementGeometry2D._ViewProviderFemElementGeometry2D
        if module.__name__ == "MechanicalAnalysis":
            import femobjects.FemConstraint
            module._FemAnalysis = femobjects.FemConstraint.Proxy
            if FreeCAD.GuiUp:
                import femguiobjects.ViewProviderBaseObject
                module._ViewProviderFemAnalysis = femguiobjects.ViewProviderBaseObject.ViewProxy
        if module.__name__ == "MechanicalMaterial":
            import femobjects._FemMaterial
            module._MechanicalMaterial = femobjects._FemMaterial._FemMaterial
            if FreeCAD.GuiUp:
                import femguiobjects._ViewProviderFemMaterial
                module._ViewProviderMechanicalMaterial = femguiobjects._ViewProviderFemMaterial._ViewProviderFemMaterial
        return None


"""
possible entries in the old files:
(the class name in the old file does not matter, we ever only had one class per module)

third big moving
from PyObjects to femobjects, following the parent commit
https://github.com/berndhahnebach/FreeCAD_bhb/tree/07ae0e56c4/src/Mod/Fem/PyObjects
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

renamed between the second and third big moveings
module="PyObjects._FemBeamSection"
module="PyObjects._FemFluidSection"
module="PyObjects._FemShellThickness"

second big moveing
into PyObjects, following the parent commit
https://github.com/berndhahnebach/FreeCAD_bhb/tree/7f884e8bff/src/Mod/Fem
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
https://github.com/berndhahnebach/FreeCAD_bhb/tree/c3328d6b4e/src/Mod/Fem
in this modules there where object class and viewprovider class together
module="FemBeamSection"
module="FemShellThickness"
module="MechanicalAnalysis"
module="MechanicalMaterial"
"""
