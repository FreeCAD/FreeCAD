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
def makeAnalysis(name="Analysis"):
    '''makeAnalysis(name): makes a Fem Analysis object'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::FemAnalysisPython", name)
    return obj


########## constraint objects ##########
def makeConstraintBearing(name="ConstraintBearing"):
    '''makeConstraintBearing(name): makes a Fem ConstraintBearing object'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::ConstraintBearing", name)
    return obj


def makeConstraintContact(name="ConstraintContact"):
    '''makeConstraintContact(name): makes a Fem ConstraintContact object'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::ConstraintContact", name)
    return obj


def makeConstraintDisplacement(name="ConstraintDisplacement"):
    '''makeConstraintDisplacement(name): makes a Fem ConstraintDisplacement object'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::ConstraintDisplacement", name)
    return obj


def makeConstraintFixed(name="ConstraintFixed"):
    '''makeConstraintFixed(name): makes a Fem ConstraintFixed object'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::ConstraintFixed", name)
    return obj


def makeConstraintFluidBoundary(name="ConstraintFluidBoundary"):
    '''makeConstraintFluidBoundary(name): makes a Fem ConstraintForce object'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::ConstraintForce", name)
    return obj


def makeConstraintForce(name="ConstraintForce"):
    '''makeConstraintForce(name): makes a Fem ConstraintForce object'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::ConstraintForce", name)
    return obj


def makeConstraintGear(name="ConstraintGear"):
    '''makeConstraintGear(name): makes a Fem ConstraintGear object'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::ConstraintGear", name)
    return obj


def makeConstraintHeatflux(name="ConstraintHeatflux"):
    '''makeConstraintHeatflux(name): makes a Fem ConstraintHeatflux object'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::ConstraintHeatflux", name)
    return obj


def makeConstraintInitialTemperature(name="ConstraintInitialTemperature"):
    '''makeConstraintInitialTemperature(name): makes a Fem ConstraintInitialTemperature object'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::ConstraintInitialTemperature", name)
    return obj


def makeConstraintPlaneRotation(name="ConstraintPlaneRotation"):
    '''makeConstraintPlaneRotation(name): makes a Fem ConstraintPlaneRotation object'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::ConstraintPlaneRotation", name)
    return obj


def makeConstraintPressure(name="ConstraintPressure"):
    '''makeConstraintPressure(name): makes a Fem ConstraintPressure object'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::ConstraintPressure", name)
    return obj


def makeConstraintPulley(name="ConstraintPulley"):
    '''makeConstraintPulley(name): makes a Fem ConstraintPulley object'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::ConstraintPulley", name)
    return obj


def makeConstraintSelfWeight(name="ConstraintSelfWeight"):
    '''makeConstraintSelfWeight([name]): creates an self weight object to define a gravity load'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::FeaturePython", name)
    import PyObjects._FemConstraintSelfWeight
    PyObjects._FemConstraintSelfWeight._FemConstraintSelfWeight(obj)
    if FreeCAD.GuiUp:
        import PyGui._ViewProviderFemConstraintSelfWeight
        PyGui._ViewProviderFemConstraintSelfWeight._ViewProviderFemConstraintSelfWeight(obj.ViewObject)
    return obj


def makeConstraintTemperature(name="ConstraintTemperature"):
    '''makeConstraintTemperature(name): makes a Fem ConstraintTemperature object'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::ConstraintTemperature", name)
    return obj


def makeConstraintTransform(name="ConstraintTransform"):
    '''makeConstraintTransform(name): makes a Fem ConstraintTransform object'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::ConstraintTransform", name)
    return obj


########## element definition objects ##########
def makeElementFluid1D(name="ElementFluid1D"):
    '''makeElementFluid1D([name]): creates an 1D fluid element object to define 1D flow'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::FeaturePython", name)
    import PyObjects._FemElementFluid1D
    PyObjects._FemElementFluid1D._FemElementFluid1D(obj)
    if FreeCAD.GuiUp:
        import PyGui._ViewProviderFemElementFluid1D
        PyGui._ViewProviderFemElementFluid1D._ViewProviderFemElementFluid1D(obj.ViewObject)
    return obj


def makeElementGeometry1D(sectiontype='Rectangular', width=10.0, height=25.0, name="ElementGeometry1D"):
    '''makeElementGeometry1D([width], [height], [name]): creates an 1D geometry element object to define a cross section'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::FeaturePython", name)
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


def makeElementGeometry2D(thickness=20.0, name="ElementGeometry2D"):
    '''makeElementGeometry2D([thickness], [name]): creates an 2D geometry element object to define a plate thickness'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::FeaturePython", name)
    import PyObjects._FemElementGeometry2D
    PyObjects._FemElementGeometry2D._FemElementGeometry2D(obj)
    obj.Thickness = thickness
    if FreeCAD.GuiUp:
        import PyGui._ViewProviderFemElementGeometry2D
        PyGui._ViewProviderFemElementGeometry2D._ViewProviderFemElementGeometry2D(obj.ViewObject)
    return obj


########## material objects ##########
def makeMaterialSolid(name="MechanicalSolidMaterial"):
    '''makeMaterialSolid(name): makes an FEM Material for solid'''
    obj = FreeCAD.ActiveDocument.addObject("App::MaterialObjectPython", name)
    import PyObjects._FemMaterial
    PyObjects._FemMaterial._FemMaterial(obj)
    obj.Category = 'Solid'
    if FreeCAD.GuiUp:
        import PyGui._ViewProviderFemMaterial
        PyGui._ViewProviderFemMaterial._ViewProviderFemMaterial(obj.ViewObject)
    # FreeCAD.ActiveDocument.recompute()
    return obj


def makeMaterialFluid(name="FluidMaterial"):
    '''makeMaterialFluid(name): makes an FEM Material for fluid'''
    obj = FreeCAD.ActiveDocument.addObject("App::MaterialObjectPython", name)
    import PyObjects._FemMaterial
    PyObjects._FemMaterial._FemMaterial(obj)
    obj.Category = 'Fluid'
    if FreeCAD.GuiUp:
        import PyGui._ViewProviderFemMaterial
        PyGui._ViewProviderFemMaterial._ViewProviderFemMaterial(obj.ViewObject)
    # FreeCAD.ActiveDocument.recompute()
    return obj


def makeMaterialMechanicalNonlinear(base_material, name="MechanicalMaterialNonlinear"):
    '''makeMaterialMechanicalNonlinear(base_material, [name]): creates an nonlinear material object'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::FeaturePython", name)
    import PyObjects._FemMaterialMechanicalNonlinear
    PyObjects._FemMaterialMechanicalNonlinear._FemMaterialMechanicalNonlinear(obj)
    obj.LinearBaseMaterial = base_material
    if FreeCAD.GuiUp:
        import PyGui._ViewProviderFemMaterialMechanicalNonlinear
        PyGui._ViewProviderFemMaterialMechanicalNonlinear._ViewProviderFemMaterialMechanicalNonlinear(obj.ViewObject)
    return obj


########## mesh objects ##########
def makeMeshGmsh(name="FEMMeshGMSH"):
    '''makeMeshGmsh(name): makes a GMSH FEM mesh object'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::FemMeshObjectPython", name)
    import PyObjects._FemMeshGmsh
    PyObjects._FemMeshGmsh._FemMeshGmsh(obj)
    if FreeCAD.GuiUp:
        import PyGui._ViewProviderFemMeshGmsh
        PyGui._ViewProviderFemMeshGmsh._ViewProviderFemMeshGmsh(obj.ViewObject)
    return obj


def makeMeshGroup(base_mesh, use_label=False, name="FEMMeshGroup"):
    '''makeMeshGroup([length], [name]): creates a  FEM mesh region object to define properties for a regon of a FEM mesh'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::FeaturePython", name)
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


def makeMeshShapeNetgenObject(name="MeshShapeNetgenObject"):
    '''makeMeshShapeNetgenObject(name): makes a Fem MeshShapeNetgenObject object'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::FemMeshShapeNetgenObject", name)
    return obj


def makeMeshRegion(base_mesh, element_length=0.0, name="FEMMeshRegion"):
    '''makeMeshRegion([length], [name]): creates a  FEM mesh region object to define properties for a regon of a FEM mesh'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::FeaturePython", name)
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


########## result objects ##########
def makeResultMechanical(name="MechanicalResult"):
    '''makeResultMechanical(name): creates an mechanical result object to hold FEM results'''
    obj = FreeCAD.ActiveDocument.addObject('Fem::FemResultObjectPython', name)
    import PyObjects._FemResultMechanical
    PyObjects._FemResultMechanical._FemResultMechanical(obj)
    if FreeCAD.GuiUp:
        import PyGui._ViewProviderFemResultMechanical
        PyGui._ViewProviderFemResultMechanical._ViewProviderFemResultMechanical(obj.ViewObject)
    return obj


########## solver objects ##########
def makeSolverCalculix(name="CalculiX"):
    '''makeSolverCalculix(name): makes a Calculix solver object'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::FemSolverObjectPython", name)
    import PyObjects._FemSolverCalculix
    PyObjects._FemSolverCalculix._FemSolverCalculix(obj)
    if FreeCAD.GuiUp:
        import PyGui._ViewProviderFemSolverCalculix
        PyGui._ViewProviderFemSolverCalculix._ViewProviderFemSolverCalculix(obj.ViewObject)
    return obj


def makeSolverZ88(name="Z88"):
    '''makeSolverZ88(name): makes a Z88 solver object'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::FemSolverObjectPython", name)
    import PyObjects._FemSolverZ88
    PyObjects._FemSolverZ88._FemSolverZ88(obj)
    if FreeCAD.GuiUp:
        import PyGui._ViewProviderFemSolverZ88
        PyGui._ViewProviderFemSolverZ88._ViewProviderFemSolverZ88(obj.ViewObject)
    return obj


'''
# print supportedTypes
App.newDocument()
module = 'Fem'
FreeCADGui.doCommand('import ' + module)
for s in sorted(App.ActiveDocument.supportedTypes()):
    if s.startswith(module):
        print s
'''

##  @}
