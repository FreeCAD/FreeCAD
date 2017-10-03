# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 - Bernd Hahnebach <bernd@bimstatik.org>            *
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

__title__ = "Objects FEM"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## \addtogroup FEM
#  @{

import FreeCAD


########## analysis objects ##########
def makeAnalysis(doc, name="Analysis"):
    '''makeAnalysis(document, [name]): makes a Fem Analysis object'''
    obj = doc.addObject("Fem::FemAnalysisPython", name)
    if FreeCAD.GuiUp:
        obj.ViewObject.Proxy = 0
    return obj


########## constraint objects ##########
def makeConstraintBearing(doc, name="ConstraintBearing"):
    '''makeConstraintBearing(document, [name]): makes a Fem ConstraintBearing object'''
    obj = doc.addObject("Fem::ConstraintBearing", name)
    return obj


def makeConstraintBodyHeatSource(doc, name="ConstraintBodyHeatSource"):
    '''makeConstraintBodyHeatSource(document, [name]): makes a Fem ConstraintBodyHeatSource object'''
    obj = doc.addObject("Fem::ConstraintPython", name)
    import PyObjects._FemConstraintBodyHeatSource
    PyObjects._FemConstraintBodyHeatSource.Proxy(obj)
    if FreeCAD.GuiUp:
        import PyGui._ViewProviderFemConstraintBodyHeatSource
        PyGui._ViewProviderFemConstraintBodyHeatSource.ViewProxy(obj.ViewObject)
    return obj


def makeConstraintContact(doc, name="ConstraintContact"):
    '''makeConstraintContact(document, [name]): makes a Fem ConstraintContact object'''
    obj = doc.addObject("Fem::ConstraintContact", name)
    return obj


def makeConstraintDisplacement(doc, name="ConstraintDisplacement"):
    '''makeConstraintDisplacement(document, [name]): makes a Fem ConstraintDisplacement object'''
    obj = doc.addObject("Fem::ConstraintDisplacement", name)
    return obj


def makeConstraintFixed(doc, name="ConstraintFixed"):
    '''makeConstraintFixed(document, [name]): makes a Fem ConstraintFixed object'''
    obj = doc.addObject("Fem::ConstraintFixed", name)
    return obj


def makeConstraintFlowVelocity(doc, name="ConstraintFlowVelocity"):
    '''makeConstraintFlowVelocity(document, [name]): makes a Fem ConstraintFlowVelocity object'''
    obj = doc.addObject("Fem::ConstraintPython", name)
    import PyObjects._FemConstraintFlowVelocity
    PyObjects._FemConstraintFlowVelocity.Proxy(obj)
    if FreeCAD.GuiUp:
        import PyGui._ViewProviderFemConstraintFlowVelocity
        PyGui._ViewProviderFemConstraintFlowVelocity.ViewProxy(obj.ViewObject)
    return obj


def makeConstraintFluidBoundary(doc, name="ConstraintFluidBoundary"):
    '''makeConstraintFluidBoundary(document, name): makes a Fem ConstraintForce object'''
    obj = doc.addObject("Fem::ConstraintForce", name)
    return obj


def makeConstraintForce(doc, name="ConstraintForce"):
    '''makeConstraintForce(document, [name]): makes a Fem ConstraintForce object'''
    obj = doc.addObject("Fem::ConstraintForce", name)
    return obj


def makeConstraintGear(doc, name="ConstraintGear"):
    '''makeConstraintGear(document, [name]): makes a Fem ConstraintGear object'''
    obj = doc.addObject("Fem::ConstraintGear", name)
    return obj


def makeConstraintHeatflux(doc, name="ConstraintHeatflux"):
    '''makeConstraintHeatflux(document, [name]): makes a Fem ConstraintHeatflux object'''
    obj = doc.addObject("Fem::ConstraintHeatflux", name)
    return obj


def makeConstraintInitialFlowVelocity(doc, name="ConstraintInitialFlowVelocity"):
    '''makeConstraintInitialFlowVelocity(document, [name]): makes a Fem ConstraintInitialFlowVelocity object'''
    obj = doc.addObject("Fem::ConstraintPython", name)
    import PyObjects._FemConstraintInitialFlowVelocity
    PyObjects._FemConstraintInitialFlowVelocity.Proxy(obj)
    if FreeCAD.GuiUp:
        import PyGui._ViewProviderFemConstraintInitialFlowVelocity
        PyGui._ViewProviderFemConstraintInitialFlowVelocity.ViewProxy(obj.ViewObject)
    return obj


def makeConstraintInitialTemperature(doc, name="ConstraintInitialTemperature"):
    '''makeConstraintInitialTemperature(document, name): makes a Fem ConstraintInitialTemperature object'''
    obj = doc.addObject("Fem::ConstraintInitialTemperature", name)
    return obj


def makeConstraintPlaneRotation(doc, name="ConstraintPlaneRotation"):
    '''makeConstraintPlaneRotation(document, [name]): makes a Fem ConstraintPlaneRotation object'''
    obj = doc.addObject("Fem::ConstraintPlaneRotation", name)
    return obj


def makeConstraintPressure(doc, name="ConstraintPressure"):
    '''makeConstraintPressure(document, [name]): makes a Fem ConstraintPressure object'''
    obj = doc.addObject("Fem::ConstraintPressure", name)
    return obj


def makeConstraintPulley(doc, name="ConstraintPulley"):
    '''makeConstraintPulley(document, [name]): makes a Fem ConstraintPulley object'''
    obj = doc.addObject("Fem::ConstraintPulley", name)
    return obj


def makeConstraintSelfWeight(doc, name="ConstraintSelfWeight"):
    '''makeConstraintSelfWeight(document, [name]): creates an self weight object to define a gravity load'''
    obj = doc.addObject("Fem::ConstraintPython", name)
    import PyObjects._FemConstraintSelfWeight
    PyObjects._FemConstraintSelfWeight._FemConstraintSelfWeight(obj)
    if FreeCAD.GuiUp:
        import PyGui._ViewProviderFemConstraintSelfWeight
        PyGui._ViewProviderFemConstraintSelfWeight._ViewProviderFemConstraintSelfWeight(obj.ViewObject)
    return obj


def makeConstraintTemperature(doc, name="ConstraintTemperature"):
    '''makeConstraintTemperature(document, [name]): makes a Fem ConstraintTemperature object'''
    obj = doc.addObject("Fem::ConstraintTemperature", name)
    return obj


def makeConstraintTransform(doc, name="ConstraintTransform"):
    '''makeConstraintTransform(document, [name]): makes a Fem ConstraintTransform object'''
    obj = doc.addObject("Fem::ConstraintTransform", name)
    return obj


########## element definition objects ##########
def makeElementFluid1D(doc, name="ElementFluid1D"):
    '''makeElementFluid1D(document, [name]): creates an 1D fluid element object to define 1D flow'''
    obj = doc.addObject("Fem::FeaturePython", name)
    import PyObjects._FemElementFluid1D
    PyObjects._FemElementFluid1D._FemElementFluid1D(obj)
    if FreeCAD.GuiUp:
        import PyGui._ViewProviderFemElementFluid1D
        PyGui._ViewProviderFemElementFluid1D._ViewProviderFemElementFluid1D(obj.ViewObject)
    return obj


def makeElementGeometry1D(doc, sectiontype='Rectangular', width=10.0, height=25.0, name="ElementGeometry1D"):
    '''makeElementGeometry1D(document, [width], [height], [name]): creates an 1D geometry element object to define a cross section'''
    obj = doc.addObject("Fem::FeaturePython", name)
    import PyObjects._FemElementGeometry1D
    PyObjects._FemElementGeometry1D._FemElementGeometry1D(obj)
    sec_types = PyObjects._FemElementGeometry1D._FemElementGeometry1D.known_beam_types
    if sectiontype not in sec_types:
        FreeCAD.Console.PrintError("Section type is not known. Set to " + sec_types[0] + " \n")
        obj.SectionType = sec_types[0]
    else:
        obj.SectionType = sectiontype
    obj.RectWidth = width
    obj.RectHeight = height
    obj.CircDiameter = height
    obj.PipeDiameter = height
    obj.PipeThickness = width
    if FreeCAD.GuiUp:
        import PyGui._ViewProviderFemElementGeometry1D
        PyGui._ViewProviderFemElementGeometry1D._ViewProviderFemElementGeometry1D(obj.ViewObject)
    return obj


def makeElementGeometry2D(doc, thickness=20.0, name="ElementGeometry2D"):
    '''makeElementGeometry2D(document, [thickness], [name]): creates an 2D geometry element object to define a plate thickness'''
    obj = doc.addObject("Fem::FeaturePython", name)
    import PyObjects._FemElementGeometry2D
    PyObjects._FemElementGeometry2D._FemElementGeometry2D(obj)
    obj.Thickness = thickness
    if FreeCAD.GuiUp:
        import PyGui._ViewProviderFemElementGeometry2D
        PyGui._ViewProviderFemElementGeometry2D._ViewProviderFemElementGeometry2D(obj.ViewObject)
    return obj


########## material objects ##########
def makeMaterialFluid(doc, name="FluidMaterial"):
    '''makeMaterialFluid(document, [name]): makes a FEM Material for fluid'''
    obj = doc.addObject("App::MaterialObjectPython", name)
    import PyObjects._FemMaterial
    PyObjects._FemMaterial._FemMaterial(obj)
    obj.Category = 'Fluid'
    if FreeCAD.GuiUp:
        import PyGui._ViewProviderFemMaterial
        PyGui._ViewProviderFemMaterial._ViewProviderFemMaterial(obj.ViewObject)
    # doc.recompute()
    return obj


def makeMaterialMechanicalNonlinear(doc, base_material, name="MechanicalMaterialNonlinear"):
    '''makeMaterialMechanicalNonlinear(document, base_material, [name]): creates a nonlinear material object'''
    obj = doc.addObject("Fem::FeaturePython", name)
    import PyObjects._FemMaterialMechanicalNonlinear
    PyObjects._FemMaterialMechanicalNonlinear._FemMaterialMechanicalNonlinear(obj)
    obj.LinearBaseMaterial = base_material
    if FreeCAD.GuiUp:
        import PyGui._ViewProviderFemMaterialMechanicalNonlinear
        PyGui._ViewProviderFemMaterialMechanicalNonlinear._ViewProviderFemMaterialMechanicalNonlinear(obj.ViewObject)
    return obj


def makeMaterialSolid(doc, name="MechanicalSolidMaterial"):
    '''makeMaterialSolid(document, [name]): makes a FEM Material for solid'''
    obj = doc.addObject("App::MaterialObjectPython", name)
    import PyObjects._FemMaterial
    PyObjects._FemMaterial._FemMaterial(obj)
    obj.Category = 'Solid'
    if FreeCAD.GuiUp:
        import PyGui._ViewProviderFemMaterial
        PyGui._ViewProviderFemMaterial._ViewProviderFemMaterial(obj.ViewObject)
    # doc.recompute()
    return obj


########## mesh objects ##########
def makeMeshBoundaryLayer(doc, base_mesh, name="MeshBoundaryLayer"):
    '''makeMeshBoundaryLayer(document, base_mesh, [name]): creates a FEM mesh BoundaryLayer object to define boundary layer properties'''
    obj = doc.addObject("Fem::FeaturePython", name)
    import PyObjects._FemMeshBoundaryLayer
    PyObjects._FemMeshBoundaryLayer._FemMeshBoundaryLayer(obj)
    # obj.BaseMesh = base_mesh
    # App::PropertyLinkList does not support append, we will use a temporary list to append the mesh BoundaryLayer obj. to the list
    tmplist = base_mesh.MeshBoundaryLayerList
    tmplist.append(obj)
    base_mesh.MeshBoundaryLayerList = tmplist
    if FreeCAD.GuiUp:
        import PyGui._ViewProviderFemMeshBoundaryLayer
        PyGui._ViewProviderFemMeshBoundaryLayer._ViewProviderFemMeshBoundaryLayer(obj.ViewObject)
    return obj


def makeMeshGmsh(doc, name="FEMMeshGMSH"):
    '''makeMeshGmsh(document, [name]): makes a GMSH FEM mesh object'''
    obj = doc.addObject("Fem::FemMeshObjectPython", name)
    import PyObjects._FemMeshGmsh
    PyObjects._FemMeshGmsh._FemMeshGmsh(obj)
    if FreeCAD.GuiUp:
        import PyGui._ViewProviderFemMeshGmsh
        PyGui._ViewProviderFemMeshGmsh._ViewProviderFemMeshGmsh(obj.ViewObject)
    return obj


def makeMeshGroup(doc, base_mesh, use_label=False, name="FEMMeshGroup"):
    '''makeMeshGroup(document, base_mesh, [use_label], [name]): creates a FEM mesh region object to define properties for a region of a FEM mesh'''
    obj = doc.addObject("Fem::FeaturePython", name)
    import PyObjects._FemMeshGroup
    PyObjects._FemMeshGroup._FemMeshGroup(obj)
    obj.UseLabel = use_label
    # obj.BaseMesh = base_mesh
    # App::PropertyLinkList does not support append, we will use a temporary list to append the mesh group obj. to the list
    tmplist = base_mesh.MeshGroupList
    tmplist.append(obj)
    base_mesh.MeshGroupList = tmplist
    if FreeCAD.GuiUp:
        import PyGui._ViewProviderFemMeshGroup
        PyGui._ViewProviderFemMeshGroup._ViewProviderFemMeshGroup(obj.ViewObject)
    return obj


def makeMeshNetgen(doc, name="FEMMeshNetgen"):
    '''makeMeshNetgen(document, [name]): makes a Fem MeshShapeNetgenObject object'''
    obj = doc.addObject("Fem::FemMeshShapeNetgenObject", name)
    return obj


def makeMeshRegion(doc, base_mesh, element_length=0.0, name="FEMMeshRegion"):
    '''makeMeshRegion(document, base_mesh, [element_length], [name]): creates a FEM mesh region object to define properties for a region of a FEM mesh'''
    obj = doc.addObject("Fem::FeaturePython", name)
    import PyObjects._FemMeshRegion
    PyObjects._FemMeshRegion._FemMeshRegion(obj)
    obj.CharacteristicLength = element_length
    # obj.BaseMesh = base_mesh
    # App::PropertyLinkList does not support append, we will use a temporary list to append the mesh region obj. to the list
    tmplist = base_mesh.MeshRegionList
    tmplist.append(obj)
    base_mesh.MeshRegionList = tmplist
    if FreeCAD.GuiUp:
        import PyGui._ViewProviderFemMeshRegion
        PyGui._ViewProviderFemMeshRegion._ViewProviderFemMeshRegion(obj.ViewObject)
    return obj


def makeMeshResult(doc, name="FEMMeshResult"):
    '''makeMeshResult(document, name): makes a Fem MeshResult object'''
    obj = doc.addObject("Fem::FemMeshObjectPython", name)
    import PyObjects._FemMeshResult
    PyObjects._FemMeshResult._FemMeshResult(obj)
    if FreeCAD.GuiUp:
        import PyGui._ViewProviderFemMeshResult
        PyGui._ViewProviderFemMeshResult._ViewProviderFemMeshResult(obj.ViewObject)
    return obj


########## result objects ##########
def makeResultMechanical(doc, name="MechanicalResult"):
    '''makeResultMechanical(document, [name]): creates an mechanical result object to hold FEM results'''
    obj = doc.addObject('Fem::FemResultObjectPython', name)
    import PyObjects._FemResultMechanical
    PyObjects._FemResultMechanical._FemResultMechanical(obj)
    if FreeCAD.GuiUp:
        import PyGui._ViewProviderFemResultMechanical
        PyGui._ViewProviderFemResultMechanical._ViewProviderFemResultMechanical(obj.ViewObject)
    return obj


########## solver objects ##########
def makeSolverCalculixOld(doc, name="CalculiXOld"):
    '''makeSolverCalculixOld(document, [name]): makes a depreciated Calculix solver object'''
    obj = doc.addObject("Fem::FemSolverObjectPython", name)
    import PyObjects._FemSolverCalculix
    PyObjects._FemSolverCalculix._FemSolverCalculix(obj)
    if FreeCAD.GuiUp:
        import PyGui._ViewProviderFemSolverCalculix
        PyGui._ViewProviderFemSolverCalculix._ViewProviderFemSolverCalculix(obj.ViewObject)
    return obj


def makeSolverCalculix(doc, name="SolverCalculiX"):
    '''makeSolverCalculix(document, [name]): makes a Calculix solver object'''
    import femsolver.calculix.solver
    obj = femsolver.calculix.solver.create(doc, name)
    return obj


def makeSolverElmer(doc, name="SolverElmer"):
    '''makeSolverElmer(document, [name]): makes a Elmer solver object'''
    import femsolver.elmer.solver
    obj = femsolver.elmer.solver.create(doc, name)
    return obj


def makeSolverZ88(doc, name="SolverZ88"):
    '''makeSolverZ88(document, [name]): makes a Z88 solver object'''
    import femsolver.z88.solver
    obj = femsolver.z88.solver.create(doc, name)
    return obj


'''
# get the supportedTypes
App.newDocument()
module = 'Fem'
FreeCADGui.doCommand('import ' + module)
for s in sorted(App.ActiveDocument.supportedTypes()):
    if s.startswith(module):
        print(s)
'''

##  @}
