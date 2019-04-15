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


# ********* analysis objects *********
def makeAnalysis(doc, name="Analysis"):
    '''makeAnalysis(document, [name]): makes a Fem Analysis object'''
    obj = doc.addObject("Fem::FemAnalysis", name)
    return obj


# ********* constraint objects *********
def makeConstraintBearing(doc, name="ConstraintBearing"):
    '''makeConstraintBearing(document, [name]): makes a Fem ConstraintBearing object'''
    obj = doc.addObject("Fem::ConstraintBearing", name)
    return obj


def makeConstraintBodyHeatSource(doc, name="ConstraintBodyHeatSource"):
    '''makeConstraintBodyHeatSource(document, [name]): makes a Fem ConstraintBodyHeatSource object'''
    obj = doc.addObject("Fem::ConstraintPython", name)
    from femobjects import _FemConstraintBodyHeatSource
    _FemConstraintBodyHeatSource.Proxy(obj)
    if FreeCAD.GuiUp:
        from femguiobjects import _ViewProviderFemConstraintBodyHeatSource
        _ViewProviderFemConstraintBodyHeatSource.ViewProxy(obj.ViewObject)
    return obj


def makeConstraintContact(doc, name="ConstraintContact"):
    '''makeConstraintContact(document, [name]): makes a Fem ConstraintContact object'''
    obj = doc.addObject("Fem::ConstraintContact", name)
    return obj


def makeConstraintDisplacement(doc, name="ConstraintDisplacement"):
    '''makeConstraintDisplacement(document, [name]): makes a Fem ConstraintDisplacement object'''
    obj = doc.addObject("Fem::ConstraintDisplacement", name)
    return obj


def makeConstraintElectrostaticPotential(doc, name="ConstraintElectrostaticPotential"):
    '''makeConstraintElectrostaticPotential(document, [name]): makes a Fem ElectrostaticPotential object'''
    obj = doc.addObject("Fem::ConstraintPython", name)
    from femobjects import _FemConstraintElectrostaticPotential
    _FemConstraintElectrostaticPotential.Proxy(obj)
    if FreeCAD.GuiUp:
        from femguiobjects import _ViewProviderFemConstraintElectrostaticPotential
        _ViewProviderFemConstraintElectrostaticPotential.ViewProxy(obj.ViewObject)
    return obj


def makeConstraintFixed(doc, name="ConstraintFixed"):
    '''makeConstraintFixed(document, [name]): makes a Fem ConstraintFixed object'''
    obj = doc.addObject("Fem::ConstraintFixed", name)
    return obj


def makeConstraintFlowVelocity(doc, name="ConstraintFlowVelocity"):
    '''makeConstraintFlowVelocity(document, [name]): makes a Fem ConstraintFlowVelocity object'''
    obj = doc.addObject("Fem::ConstraintPython", name)
    from femobjects import _FemConstraintFlowVelocity
    _FemConstraintFlowVelocity.Proxy(obj)
    if FreeCAD.GuiUp:
        from femguiobjects import _ViewProviderFemConstraintFlowVelocity
        _ViewProviderFemConstraintFlowVelocity.ViewProxy(obj.ViewObject)
    return obj


def makeConstraintFluidBoundary(doc, name="ConstraintFluidBoundary"):
    '''makeConstraintFluidBoundary(document, name): makes a Fem ConstraintFluidBoundary object'''
    obj = doc.addObject("Fem::ConstraintFluidBoundary", name)
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
    from femobjects import _FemConstraintInitialFlowVelocity
    _FemConstraintInitialFlowVelocity.Proxy(obj)
    if FreeCAD.GuiUp:
        from femguiobjects import _ViewProviderFemConstraintInitialFlowVelocity
        _ViewProviderFemConstraintInitialFlowVelocity.ViewProxy(obj.ViewObject)
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
    from femobjects import _FemConstraintSelfWeight
    _FemConstraintSelfWeight._FemConstraintSelfWeight(obj)
    if FreeCAD.GuiUp:
        from femguiobjects import _ViewProviderFemConstraintSelfWeight
        _ViewProviderFemConstraintSelfWeight._ViewProviderFemConstraintSelfWeight(obj.ViewObject)
    return obj


def makeConstraintTemperature(doc, name="ConstraintTemperature"):
    '''makeConstraintTemperature(document, [name]): makes a Fem ConstraintTemperature object'''
    obj = doc.addObject("Fem::ConstraintTemperature", name)
    return obj


def makeConstraintTransform(doc, name="ConstraintTransform"):
    '''makeConstraintTransform(document, [name]): makes a Fem ConstraintTransform object'''
    obj = doc.addObject("Fem::ConstraintTransform", name)
    return obj


# ********* element definition objects *********
def makeElementFluid1D(doc, name="ElementFluid1D"):
    '''makeElementFluid1D(document, [name]): creates an 1D fluid element object to define 1D flow'''
    obj = doc.addObject("Fem::FeaturePython", name)
    from femobjects import _FemElementFluid1D
    _FemElementFluid1D._FemElementFluid1D(obj)
    if FreeCAD.GuiUp:
        from femguiobjects import _ViewProviderFemElementFluid1D
        _ViewProviderFemElementFluid1D._ViewProviderFemElementFluid1D(obj.ViewObject)
    return obj


def makeElementGeometry1D(doc, sectiontype='Rectangular', width=10.0, height=25.0, name="ElementGeometry1D"):
    '''makeElementGeometry1D(document, [width], [height], [name]): creates an 1D geometry element object to define a cross section'''
    obj = doc.addObject("Fem::FeaturePython", name)
    from femobjects import _FemElementGeometry1D
    _FemElementGeometry1D._FemElementGeometry1D(obj)
    sec_types = _FemElementGeometry1D._FemElementGeometry1D.known_beam_types
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
        from femguiobjects import _ViewProviderFemElementGeometry1D
        _ViewProviderFemElementGeometry1D._ViewProviderFemElementGeometry1D(obj.ViewObject)
    return obj


def makeElementGeometry2D(doc, thickness=20.0, name="ElementGeometry2D"):
    '''makeElementGeometry2D(document, [thickness], [name]): creates an 2D geometry element object to define a plate thickness'''
    obj = doc.addObject("Fem::FeaturePython", name)
    from femobjects import _FemElementGeometry2D
    _FemElementGeometry2D._FemElementGeometry2D(obj)
    obj.Thickness = thickness
    if FreeCAD.GuiUp:
        from femguiobjects import _ViewProviderFemElementGeometry2D
        _ViewProviderFemElementGeometry2D._ViewProviderFemElementGeometry2D(obj.ViewObject)
    return obj


def makeElementRotation1D(doc, name="ElementRotation1D"):
    '''makeElementRotation1D(document, [name]): creates an 1D geometry rotation element object to rotate a 1D cross section'''
    obj = doc.addObject("Fem::FeaturePython", name)
    from femobjects import _FemElementRotation1D
    _FemElementRotation1D._FemElementRotation1D(obj)
    if FreeCAD.GuiUp:
        from femguiobjects import _ViewProviderFemElementRotation1D
        _ViewProviderFemElementRotation1D._ViewProviderFemElementRotation1D(obj.ViewObject)
    return obj


# ********* material objects *********
def makeMaterialFluid(doc, name="FluidMaterial"):
    '''makeMaterialFluid(document, [name]): makes a FEM Material for fluid'''
    obj = doc.addObject("App::MaterialObjectPython", name)
    from femobjects import _FemMaterial
    _FemMaterial._FemMaterial(obj)
    obj.Category = 'Fluid'
    if FreeCAD.GuiUp:
        from femguiobjects import _ViewProviderFemMaterial
        _ViewProviderFemMaterial._ViewProviderFemMaterial(obj.ViewObject)
    return obj


def makeMaterialMechanicalNonlinear(doc, base_material, name="MechanicalMaterialNonlinear"):
    '''makeMaterialMechanicalNonlinear(document, base_material, [name]): creates a nonlinear material object'''
    obj = doc.addObject("Fem::FeaturePython", name)
    from femobjects import _FemMaterialMechanicalNonlinear
    _FemMaterialMechanicalNonlinear._FemMaterialMechanicalNonlinear(obj)
    obj.LinearBaseMaterial = base_material
    if FreeCAD.GuiUp:
        from femguiobjects import _ViewProviderFemMaterialMechanicalNonlinear
        _ViewProviderFemMaterialMechanicalNonlinear._ViewProviderFemMaterialMechanicalNonlinear(obj.ViewObject)
    return obj


def makeMaterialSolid(doc, name="MechanicalSolidMaterial"):
    '''makeMaterialSolid(document, [name]): makes a FEM Material for solid'''
    obj = doc.addObject("App::MaterialObjectPython", name)
    from femobjects import _FemMaterial
    _FemMaterial._FemMaterial(obj)
    obj.Category = 'Solid'
    if FreeCAD.GuiUp:
        from femguiobjects import _ViewProviderFemMaterial
        _ViewProviderFemMaterial._ViewProviderFemMaterial(obj.ViewObject)
    return obj


# ********* mesh objects *********
def makeMeshBoundaryLayer(doc, base_mesh, name="MeshBoundaryLayer"):
    '''makeMeshBoundaryLayer(document, base_mesh, [name]): creates a FEM mesh BoundaryLayer object to define boundary layer properties'''
    obj = doc.addObject("Fem::FeaturePython", name)
    from femobjects import _FemMeshBoundaryLayer
    _FemMeshBoundaryLayer._FemMeshBoundaryLayer(obj)
    # obj.BaseMesh = base_mesh
    # App::PropertyLinkList does not support append, we will use a temporary list to append the mesh BoundaryLayer obj. to the list
    tmplist = base_mesh.MeshBoundaryLayerList
    tmplist.append(obj)
    base_mesh.MeshBoundaryLayerList = tmplist
    if FreeCAD.GuiUp:
        from femguiobjects import _ViewProviderFemMeshBoundaryLayer
        _ViewProviderFemMeshBoundaryLayer._ViewProviderFemMeshBoundaryLayer(obj.ViewObject)
    return obj


def makeMeshGmsh(doc, name="FEMMeshGmsh"):
    '''makeMeshGmsh(document, [name]): makes a Gmsh FEM mesh object'''
    obj = doc.addObject("Fem::FemMeshObjectPython", name)
    from femobjects import _FemMeshGmsh
    _FemMeshGmsh._FemMeshGmsh(obj)
    if FreeCAD.GuiUp:
        from femguiobjects import _ViewProviderFemMeshGmsh
        _ViewProviderFemMeshGmsh._ViewProviderFemMeshGmsh(obj.ViewObject)
    return obj


def makeMeshGroup(doc, base_mesh, use_label=False, name="FEMMeshGroup"):
    '''makeMeshGroup(document, base_mesh, [use_label], [name]): creates a FEM mesh region object to define properties for a region of a FEM mesh'''
    obj = doc.addObject("Fem::FeaturePython", name)
    from femobjects import _FemMeshGroup
    _FemMeshGroup._FemMeshGroup(obj)
    obj.UseLabel = use_label
    # obj.BaseMesh = base_mesh
    # App::PropertyLinkList does not support append, we will use a temporary list to append the mesh group obj. to the list
    tmplist = base_mesh.MeshGroupList
    tmplist.append(obj)
    base_mesh.MeshGroupList = tmplist
    if FreeCAD.GuiUp:
        from femguiobjects import _ViewProviderFemMeshGroup
        _ViewProviderFemMeshGroup._ViewProviderFemMeshGroup(obj.ViewObject)
    return obj


def makeMeshNetgen(doc, name="FEMMeshNetgen"):
    '''makeMeshNetgen(document, [name]): makes a Fem MeshShapeNetgenObject object'''
    obj = doc.addObject("Fem::FemMeshShapeNetgenObject", name)
    return obj


def makeMeshRegion(doc, base_mesh, element_length=0.0, name="FEMMeshRegion"):
    '''makeMeshRegion(document, base_mesh, [element_length], [name]): creates a FEM mesh region object to define properties for a region of a FEM mesh'''
    obj = doc.addObject("Fem::FeaturePython", name)
    from femobjects import _FemMeshRegion
    _FemMeshRegion._FemMeshRegion(obj)
    obj.CharacteristicLength = element_length
    # obj.BaseMesh = base_mesh
    # App::PropertyLinkList does not support append, we will use a temporary list to append the mesh region obj. to the list
    tmplist = base_mesh.MeshRegionList
    tmplist.append(obj)
    base_mesh.MeshRegionList = tmplist
    if FreeCAD.GuiUp:
        from femguiobjects import _ViewProviderFemMeshRegion
        _ViewProviderFemMeshRegion._ViewProviderFemMeshRegion(obj.ViewObject)
    return obj


def makeMeshResult(doc, name="FEMMeshResult"):
    '''makeMeshResult(document, name): makes a Fem MeshResult object'''
    obj = doc.addObject("Fem::FemMeshObjectPython", name)
    from femobjects import _FemMeshResult
    _FemMeshResult._FemMeshResult(obj)
    if FreeCAD.GuiUp:
        from femguiobjects import _ViewProviderFemMeshResult
        _ViewProviderFemMeshResult._ViewProviderFemMeshResult(obj.ViewObject)
    return obj


# ********* post processing objects *********
def makeResultMechanical(doc, name="MechanicalResult"):
    '''makeResultMechanical(document, [name]): creates an mechanical result object to hold FEM results'''
    obj = doc.addObject('Fem::FemResultObjectPython', name)
    from femobjects import _FemResultMechanical
    _FemResultMechanical._FemResultMechanical(obj)
    if FreeCAD.GuiUp:
        from femguiobjects import _ViewProviderFemResultMechanical
        _ViewProviderFemResultMechanical._ViewProviderFemResultMechanical(obj.ViewObject)
    return obj


def makePostVtkFilterClipRegion(doc, base_vtk_result, name="FEMVtkFilterClipRegion"):
    '''makePostVtkFilterClipRegion(document, base_vtk_result, [name]): creates an FEM post processing region clip filter object (vtk based)'''
    obj = doc.addObject("Fem::FemPostClipFilter", name)
    tmp_filter_list = base_vtk_result.Filter
    tmp_filter_list.append(obj)
    base_vtk_result.Filter = tmp_filter_list
    del tmp_filter_list
    return obj


def makePostVtkFilterClipScalar(doc, base_vtk_result, name="FEMVtkFilterClipScalar"):
    '''makePostVtkFilterClipScalar(document, base_vtk_result, [name]): creates an FEM post processing scalar clip filter object (vtk based)'''
    obj = doc.addObject("Fem::FemPostScalarClipFilter", name)
    tmp_filter_list = base_vtk_result.Filter
    tmp_filter_list.append(obj)
    base_vtk_result.Filter = tmp_filter_list
    del tmp_filter_list
    return obj


def makePostVtkFilterCutFunction(doc, base_vtk_result, name="FEMVtkFilterCutFunction"):
    '''makePostVtkFilterCutFunction(document, base_vtk_result, [name]): creates an FEM post processing cut function filter object (vtk based)'''
    obj = doc.addObject("Fem::FemPostClipFilter", name)
    tmp_filter_list = base_vtk_result.Filter
    tmp_filter_list.append(obj)
    base_vtk_result.Filter = tmp_filter_list
    del tmp_filter_list
    return obj


def makePostVtkFilterWarp(doc, base_vtk_result, name="FEMVtkFilterWarp"):
    '''makePostVtkFilterWarp(document, base_vtk_result, [name]): creates an FEM post processing warp filter object (vtk based)'''
    obj = doc.addObject("Fem::FemPostWarpVectorFilter", name)
    tmp_filter_list = base_vtk_result.Filter
    tmp_filter_list.append(obj)
    base_vtk_result.Filter = tmp_filter_list
    del tmp_filter_list
    return obj


def makePostVtkResult(doc, base_result, name="FEMVtkResult"):
    '''makePostVtkResult(document, base_result [name]): creates an FEM post processing result object (vtk based) to hold FEM results'''
    obj = doc.addObject("Fem::FemPostPipeline", name)
    obj.load(base_result)
    return obj


# ********* solver objects *********
def makeEquationElasticity(doc, base_solver):
    '''makeEquationElasticity(document, base_solver): creates a FEM elasticity equation for a solver'''
    obj = doc.SolverElmer.addObject(doc.SolverElmer.Proxy.createEquation(doc.SolverElmer.Document, 'Elasticity'))[0]
    return obj


def makeEquationElectrostatic(doc, base_solver):
    '''makeEquationElectrostatic(document, base_solver): creates a FEM electrostatic equation for a solver'''
    obj = doc.SolverElmer.addObject(doc.SolverElmer.Proxy.createEquation(doc.SolverElmer.Document, 'Electrostatic'))[0]
    return obj


def makeEquationFlow(doc, base_solver):
    '''makeEquationFlow(document, base_solver): creates a FEM flow equation for a solver'''
    obj = doc.SolverElmer.addObject(doc.SolverElmer.Proxy.createEquation(doc.SolverElmer.Document, 'Flow'))[0]
    return obj


def makeEquationFluxsolver(doc, base_solver):
    '''makeEquationFluxsolver(document, base_solver): creates a FEM fluxsolver equation for a solver'''
    obj = doc.SolverElmer.addObject(doc.SolverElmer.Proxy.createEquation(doc.SolverElmer.Document, 'Fluxsolver'))[0]
    return obj


def makeEquationHeat(doc, base_solver):
    '''makeEquationHeat(document, base_solver): creates a FEM heat equation for a solver'''
    obj = doc.SolverElmer.addObject(doc.SolverElmer.Proxy.createEquation(doc.SolverElmer.Document, 'Heat'))[0]
    return obj


def makeSolverCalculixCcxTools(doc, name="CalculiXccxTools"):
    '''makeSolverCalculixCcxTools(document, [name]): makes a Calculix solver object for the ccx tools module'''
    obj = doc.addObject("Fem::FemSolverObjectPython", name)
    from femobjects import _FemSolverCalculix
    _FemSolverCalculix._FemSolverCalculix(obj)
    if FreeCAD.GuiUp:
        from femguiobjects import _ViewProviderFemSolverCalculix
        _ViewProviderFemSolverCalculix._ViewProviderFemSolverCalculix(obj.ViewObject)
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
