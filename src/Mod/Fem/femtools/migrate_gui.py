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
https://forum.freecadweb.org/viewtopic.php?&t=46218
"""

__title__ = "migrate gui"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"


class FemMigrateGui(object):

    def find_module(self, fullname, path):
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

        if fullname == "PyGui._ViewProviderFemBeamSection":
            return self
        if fullname == "PyGui._ViewProviderFemFluidSection":
            return self
        if fullname == "PyGui._ViewProviderFemShellThickness":
            return self

        if fullname == "_ViewProviderFemBeamSection":
            return self
        if fullname == "_FemConstraintSelfWeight":
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
        if module.__name__ == "PyGui":
            module.__path__ = "PyGui"

        if module.__name__ == "PyGui._ViewProviderFemConstraintBodyHeatSource":
            import femguiobjects._ViewProviderFemConstraintBodyHeatSource
            module._ViewProviderFemConstraintBodyHeatSource = femguiobjects._ViewProviderFemConstraintBodyHeatSource.ViewProxy
        if module.__name__ == "PyGui._ViewProviderFemConstraintElectrostaticPotential":
            import femguiobjects._ViewProviderFemConstraintElectrostaticPotential
            module._ViewProviderFemConstraintElectrostaticPotential = femguiobjects._ViewProviderFemConstraintElectrostaticPotential.ViewProxy
        if module.__name__ == "PyGui._ViewProviderFemConstraintFlowVelocity":
            import femguiobjects._ViewProviderFemConstraintFlowVelocity
            module._ViewProviderFemConstraintFlowVelocity = femguiobjects._ViewProviderFemConstraintFlowVelocity.ViewProxy
        if module.__name__ == "PyGui._ViewProviderFemConstraintInitialFlowVelocity":
            import femguiobjects._ViewProviderFemConstraintInitialFlowVelocity
            module._ViewProviderFemConstraintInitialFlowVelocity = femguiobjects._ViewProviderFemConstraintInitialFlowVelocity.ViewProxy
        if module.__name__ == "PyGui._ViewProviderFemConstraintSelfWeight":
            import femguiobjects._ViewProviderFemConstraintSelfWeight
            module._ViewProviderFemConstraintSelfWeight = femguiobjects._ViewProviderFemConstraintSelfWeight._ViewProviderFemConstraintSelfWeight
        if module.__name__ == "PyGui._ViewProviderFemElementFluid1D":
            import femguiobjects._ViewProviderFemElementFluid1D
            module._ViewProviderFemElementFluid1D = femguiobjects._ViewProviderFemElementFluid1D._ViewProviderFemElementFluid1D
        if module.__name__ == "PyGui._ViewProviderFemElementGeometry1D":
            import femguiobjects._ViewProviderFemElementGeometry1D
            module._ViewProviderFemElementGeometry1D = femguiobjects._ViewProviderFemElementGeometry1D._ViewProviderFemElementGeometry1D
        if module.__name__ == "PyGui._ViewProviderFemElementGeometry2D":
            import femguiobjects._ViewProviderFemElementGeometry2D
            module._ViewProviderFemElementGeometry2D = femguiobjects._ViewProviderFemElementGeometry2D._ViewProviderFemElementGeometry2D
        if module.__name__ == "PyGui._ViewProviderFemElementRotation1D":
            import femguiobjects._ViewProviderFemElementRotation1D
            module._ViewProviderFemElementRotation1D = femguiobjects._ViewProviderFemElementRotation1D._ViewProviderFemElementRotation1D
        if module.__name__ == "PyGui._ViewProviderFemMaterial":
            import femguiobjects._ViewProviderFemMaterial
            module._ViewProviderFemMaterial = femguiobjects._ViewProviderFemMaterial._ViewProviderFemMaterial
        if module.__name__ == "PyGui._ViewProviderFemMaterialMechanicalNonlinear":
            import femguiobjects._ViewProviderFemMaterialMechanicalNonlinear
            module._ViewProviderFemMaterialMechanicalNonlinear = femguiobjects._ViewProviderFemMaterialMechanicalNonlinear._ViewProviderFemMaterialMechanicalNonlinear
        if module.__name__ == "PyGui._ViewProviderFemMeshBoundaryLayer":
            import femguiobjects._ViewProviderFemMeshBoundaryLayer
            module._ViewProviderFemMeshBoundaryLayer = femguiobjects._ViewProviderFemMeshBoundaryLayer._ViewProviderFemMeshBoundaryLayer
        if module.__name__ == "PyGui._ViewProviderFemMeshGmsh":
            import femguiobjects._ViewProviderFemMeshGmsh
            module._ViewProviderFemMeshGmsh = femguiobjects._ViewProviderFemMeshGmsh._ViewProviderFemMeshGmsh
        if module.__name__ == "PyGui._ViewProviderFemMeshGroup":
            import femguiobjects._ViewProviderFemMeshGroup
            module._ViewProviderFemMeshGroup = femguiobjects._ViewProviderFemMeshGroup._ViewProviderFemMeshGroup
        if module.__name__ == "PyGui._ViewProviderFemMeshRegion":
            import femguiobjects._ViewProviderFemMeshRegion
            module._ViewProviderFemMeshRegion = femguiobjects._ViewProviderFemMeshRegion._ViewProviderFemMeshRegion
        if module.__name__ == "PyGui._ViewProviderFemMeshResult":
            import femguiobjects._ViewProviderFemMeshResult
            module._ViewProviderFemMeshResult = femguiobjects._ViewProviderFemMeshResult._ViewProviderFemMeshResult
        if module.__name__ == "PyGui._ViewProviderFemResultMechanical":
            import femguiobjects._ViewProviderFemResultMechanical
            module._ViewProviderFemResultMechanical = femguiobjects._ViewProviderFemResultMechanical._ViewProviderFemResultMechanical
        if module.__name__ == "PyGui._ViewProviderFemSolverCalculix":
            import femguiobjects._ViewProviderFemSolverCalculix
            module._ViewProviderFemSolverCalculix = femguiobjects._ViewProviderFemSolverCalculix._ViewProviderFemSolverCalculix

        if module.__name__ == "PyGui._ViewProviderFemBeamSection":
            import femguiobjects._ViewProviderFemElementGeometry1D
            module._ViewProviderFemBeamSection = femguiobjects._ViewProviderFemElementGeometry1D._ViewProviderFemElementGeometry1D
        if module.__name__ == "PyGui._ViewProviderFemFluidSection":
            import femguiobjects._ViewProviderFemElementFluid1D
            module._ViewProviderFemFluidSection = femguiobjects._ViewProviderFemElementFluid1D._ViewProviderFemElementFluid1D
        if module.__name__ == "PyGui._ViewProviderFemShellThickness":
            import femguiobjects._ViewProviderFemElementGeometry2D
            module._ViewProviderFemShellThickness = femguiobjects._ViewProviderFemElementGeometry2D._ViewProviderFemElementGeometry2D

        if module.__name__ == "_ViewProviderFemBeamSection":
            import femguiobjects._ViewProviderFemElementGeometry1D
            module._ViewProviderFemBeamSection = femguiobjects._ViewProviderFemElementGeometry1D._ViewProviderFemElementGeometry1D
        if module.__name__ == "_ViewProviderFemConstraintSelfWeight":
            import femguiobjects._ViewProviderFemConstraintSelfWeight
            module._ViewProviderFemConstraintSelfWeight = femguiobjects._ViewProviderFemConstraintSelfWeight._ViewProviderFemConstraintSelfWeight
        if module.__name__ == "_ViewProviderFemMaterial":
            import femguiobjects._ViewProviderFemMaterial
            module._ViewProviderFemMaterial = femguiobjects._ViewProviderFemMaterial._ViewProviderFemMaterial
        if module.__name__ == "_ViewProviderFemMaterialMechanicalNonlinear":
            import femguiobjects._ViewProviderFemMaterialMechanicalNonlinear
            module._ViewProviderFemMaterialMechanicalNonlinear = femguiobjects._ViewProviderFemMaterialMechanicalNonlinear._ViewProviderFemMaterialMechanicalNonlinear
        if module.__name__ == "_ViewProviderFemMeshGmsh":
            import femguiobjects._ViewProviderFemMeshGmsh
            module._ViewProviderFemMeshGmsh = femguiobjects._ViewProviderFemMeshGmsh._ViewProviderFemMeshGmsh
        if module.__name__ == "_ViewProviderFemMeshGroup":
            import femguiobjects._ViewProviderFemMeshGroup
            module._ViewProviderFemMeshGroup = femguiobjects._ViewProviderFemMeshGroup._ViewProviderFemMeshGroup
        if module.__name__ == "_ViewProviderFemMeshRegion":
            import femguiobjects._ViewProviderFemMeshRegion
            module._ViewProviderFemMeshRegion = femguiobjects._ViewProviderFemMeshRegion._ViewProviderFemMeshRegion
        if module.__name__ == "_ViewProviderFemResultMechanical":
            import femguiobjects._ViewProviderFemResultMechanical
            module._ViewProviderFemResultMechanical = femguiobjects._ViewProviderFemResultMechanical._ViewProviderFemResultMechanical
        if module.__name__ == "_ViewProviderFemShellThickness":
            import femguiobjects._ViewProviderFemElementGeometry2D
            module._ViewProviderFemShellThickness = femguiobjects._ViewProviderFemElementGeometry2D._ViewProviderFemElementGeometry2D
        if module.__name__ == "_ViewProviderFemSolverCalculix":
            import femguiobjects._ViewProviderFemSolverCalculix
            module._ViewProviderFemSolverCalculix = femguiobjects._ViewProviderFemSolverCalculix._ViewProviderFemSolverCalculix
        if module.__name__ == "_ViewProviderFemSolverZ88":
            import femsolver.z88.solver
            module._ViewProviderFemSolverZ88 = femsolver.z88.solver.ViewProxy

        if module.__name__ == "_ViewProviderFemMechanicalResult":
            import femguiobjects._ViewProviderFemResultMechanical
            module._ViewProviderFemMechanicalResult = femguiobjects._ViewProviderFemResultMechanical._ViewProviderFemResultMechanical
        if module.__name__ == "ViewProviderFemResult":
            import femguiobjects._ViewProviderFemResultMechanical
            module.ViewProviderFemResult = femguiobjects._ViewProviderFemResultMechanical._ViewProviderFemResultMechanical
        if module.__name__ == "_ViewProviderMechanicalMaterial":
            import femguiobjects._ViewProviderFemMaterial
            module._ViewProviderMechanicalMaterial = femguiobjects._ViewProviderFemMaterial._ViewProviderFemMaterial

        return None


"""
possible entries in the old files:
(the class name in the old file does not matter, we ever only had one class per module)

third big moving
from PyGui to femguiobjects, following the parent commit
https://github.com/berndhahnebach/FreeCAD_bhb/tree/07ae0e56c4/src/Mod/Fem/PyGui
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

renamed between the second and third big moveings
module="PyGui._ViewProviderFemBeamSection"
module="PyGui._ViewProviderFemFluidSection"
module="PyGui._ViewProviderFemShellThickness"

second big moveing
into PyObjects, following the parent commit
https://github.com/berndhahnebach/FreeCAD_bhb/tree/7f884e8bff/src/Mod/Fem
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
https://github.com/berndhahnebach/FreeCAD_bhb/tree/c3328d6b4e/src/Mod/Fem
in this modules there where object class and viewprovider class together
# see migrate App
module="FemBeamSection"
module="FemShellThickness"
module="MechanicalAnalysis"
module="MechanicalMaterial"


"""
