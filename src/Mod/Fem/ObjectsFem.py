# ***************************************************************************
# *   Copyright (c) 2016 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "Objects FEM"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

import FreeCAD


# PythonFeatures from package femobjects
# standard object name == class name == type without 'Fem::'

# PythonFeatures from package femsolver
# standard object name == type without 'Fem::'
# the class name is Proxy

# TODO
# There are objects which use a base object. It should be tested if the base object
# is in the same document as the doc in which the obj should be created.
# Could only be happen if the make is called from Python.
# What happens ATM? Error or the obj is moved to the other doc?


# ********* analysis objects *********************************************************************
def makeAnalysis(
    doc,
    name="Analysis"
):
    """makeAnalysis(document, [name]):
    makes a Fem Analysis object"""
    obj = doc.addObject("Fem::FemAnalysis", name)
    return obj


# ********* constant objects *********************************************************************
def makeConstantVacuumPermittivity(
    doc,
    name="ConstantVacuumPermittivity"
):
    """makeConstantVacuumPermittivity(document, [name]):
    makes a Fem ConstantVacuumPermittivity object"""
    obj = doc.addObject("Fem::ConstraintPython", name)
    from femobjects import constant_vacuumpermittivity
    constant_vacuumpermittivity.ConstantVacuumPermittivity(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_constant_vacuumpermittivity
        view_constant_vacuumpermittivity.VPConstantVacuumPermittivity(obj.ViewObject)
    return obj


# ********* constraint objects *******************************************************************
def makeConstraintBearing(
    doc,
    name="ConstraintBearing"
):
    """makeConstraintBearing(document, [name]):
    makes a Fem ConstraintBearing object"""
    obj = doc.addObject("Fem::ConstraintBearing", name)
    return obj


def makeConstraintBodyHeatSource(
    doc,
    name="ConstraintBodyHeatSource"
):
    """makeConstraintBodyHeatSource(document, [name]):
    makes a Fem ConstraintBodyHeatSource object"""
    obj = doc.addObject("Fem::ConstraintPython", name)
    from femobjects import constraint_bodyheatsource
    constraint_bodyheatsource.ConstraintBodyHeatSource(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_constraint_bodyheatsource as viewprov
        viewprov.VPConstraintBodyHeatSource(obj.ViewObject)
    return obj


def makeConstraintCentrif(
    doc,
    name="ConstraintCentrif"
):
    """makeConstraintCentrif(document, [name]):
    creates a centrif object to define centrifugal body load constraint"""
    obj = doc.addObject("Fem::ConstraintPython", name)
    from femobjects import constraint_centrif
    constraint_centrif.ConstraintCentrif(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_constraint_centrif
        view_constraint_centrif.VPConstraintCentrif(obj.ViewObject)
    return obj


def makeConstraintCurrentDensity(
    doc,
    name="ConstraintCurrentDensity"
):
    """makeConstraintCurrentDensity(document, [name]):
    makes a Fem CurrentDensity object"""
    obj = doc.addObject("Fem::ConstraintPython", name)
    from femobjects import constraint_currentdensity
    constraint_currentdensity.ConstraintCurrentDensity(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_constraint_currentdensity
        view_constraint_currentdensity.VPConstraintCurrentDensity(obj.ViewObject)
    return obj


def makeConstraintContact(
    doc,
    name="ConstraintContact"
):
    """makeConstraintContact(document, [name]):
    makes a Fem ConstraintContact object"""
    obj = doc.addObject("Fem::ConstraintContact", name)
    return obj


def makeConstraintDisplacement(
    doc,
    name="ConstraintDisplacement"
):
    """makeConstraintDisplacement(document, [name]):
    makes a Fem ConstraintDisplacement object"""
    obj = doc.addObject("Fem::ConstraintDisplacement", name)
    return obj


def makeConstraintElectrostaticPotential(
    doc,
    name="ConstraintElectrostaticPotential"
):
    """makeConstraintElectrostaticPotential(document, [name]):
    makes a Fem ElectrostaticPotential object"""
    obj = doc.addObject("Fem::ConstraintPython", name)
    from femobjects import constraint_electrostaticpotential
    constraint_electrostaticpotential.ConstraintElectrostaticPotential(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_constraint_electrostaticpotential
        view_constraint_electrostaticpotential.VPConstraintElectroStaticPotential(obj.ViewObject)
    return obj


def makeConstraintFixed(
    doc,
    name="ConstraintFixed"
):
    """makeConstraintFixed(document, [name]):
    makes a Fem ConstraintFixed object"""
    obj = doc.addObject("Fem::ConstraintFixed", name)
    return obj


def makeConstraintRigidBody(
    doc,
    name="ConstraintRigidBody"
):
    """makeConstraintRigidBody(document, [name]):
    makes a Fem ConstraintRigidBody object"""
    obj = doc.addObject("Fem::ConstraintRigidBody", name)
    return obj


def makeConstraintFlowVelocity(
    doc,
    name="ConstraintFlowVelocity"
):
    """makeConstraintFlowVelocity(document, [name]):
    makes a Fem ConstraintFlowVelocity object"""
    obj = doc.addObject("Fem::ConstraintPython", name)
    from femobjects import constraint_flowvelocity
    constraint_flowvelocity.ConstraintFlowVelocity(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_constraint_flowvelocity
        view_constraint_flowvelocity.VPConstraintFlowVelocity(obj.ViewObject)
    return obj


def makeConstraintFluidBoundary(
    doc,
    name="ConstraintFluidBoundary"
):
    """makeConstraintFluidBoundary(document, name):
    makes a Fem ConstraintFluidBoundary object"""
    obj = doc.addObject("Fem::ConstraintFluidBoundary", name)
    return obj


def makeConstraintForce(
    doc,
    name="ConstraintForce"
):
    """makeConstraintForce(document, [name]):
    makes a Fem ConstraintForce object"""
    obj = doc.addObject("Fem::ConstraintForce", name)
    return obj


def makeConstraintGear(
    doc,
    name="ConstraintGear"
):
    """makeConstraintGear(document, [name]):
    makes a Fem ConstraintGear object"""
    obj = doc.addObject("Fem::ConstraintGear", name)
    return obj


def makeConstraintHeatflux(
    doc,
    name="ConstraintHeatflux"
):
    """makeConstraintHeatflux(document, [name]):
    makes a Fem ConstraintHeatflux object"""
    obj = doc.addObject("Fem::ConstraintHeatflux", name)
    return obj


def makeConstraintInitialFlowVelocity(
    doc,
    name="ConstraintInitialFlowVelocity"
):
    """makeConstraintInitialFlowVelocity(document, [name]):
    makes a Fem ConstraintInitialFlowVelocity object"""
    obj = doc.addObject("Fem::ConstraintPython", name)
    from femobjects import constraint_initialflowvelocity
    constraint_initialflowvelocity.ConstraintInitialFlowVelocity(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_constraint_initialflowvelocity
        view_constraint_initialflowvelocity.VPConstraintInitialFlowVelocity(obj.ViewObject)
    return obj


def makeConstraintInitialPressure(
    doc,
    name="ConstraintInitialPressure"
):
    """makeConstraintInitialPressure(document, [name]):
    makes a Fem ConstraintInitialPressure object"""
    obj = doc.addObject("Fem::ConstraintPython", name)
    from femobjects import constraint_initialpressure
    constraint_initialpressure.ConstraintInitialPressure(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_constraint_initialpressure
        view_constraint_initialpressure.VPConstraintInitialPressure(obj.ViewObject)
    return obj


def makeConstraintInitialTemperature(
    doc,
    name="ConstraintInitialTemperature"
):
    """makeConstraintInitialTemperature(document, name):
    makes a Fem ConstraintInitialTemperature object"""
    obj = doc.addObject("Fem::ConstraintInitialTemperature", name)
    return obj


def makeConstraintMagnetization(
    doc,
    name="ConstraintMagnetization"
):
    """makeConstraintMagnetization(document, [name]):
    makes a Fem Magnetization object"""
    obj = doc.addObject("Fem::ConstraintPython", name)
    from femobjects import constraint_magnetization
    constraint_magnetization.ConstraintMagnetization(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_constraint_magnetization
        view_constraint_magnetization.VPConstraintMagnetization(obj.ViewObject)
    return obj


def makeConstraintPlaneRotation(
    doc,
    name="ConstraintPlaneRotation"
):
    """makeConstraintPlaneRotation(document, [name]):
    makes a Fem ConstraintPlaneRotation object"""
    obj = doc.addObject("Fem::ConstraintPlaneRotation", name)
    return obj


def makeConstraintPressure(
    doc,
    name="ConstraintPressure"
):
    """makeConstraintPressure(document, [name]):
    makes a Fem ConstraintPressure object"""
    obj = doc.addObject("Fem::ConstraintPressure", name)
    return obj


def makeConstraintPulley(
    doc,
    name="ConstraintPulley"
):
    """makeConstraintPulley(document, [name]):
    makes a Fem ConstraintPulley object"""
    obj = doc.addObject("Fem::ConstraintPulley", name)
    return obj


def makeConstraintSelfWeight(
    doc,
    name="ConstraintSelfWeight"
):
    """makeConstraintSelfWeight(document, [name]):
    creates a self weight object to define a gravity load"""
    obj = doc.addObject("Fem::ConstraintPython", name)
    from femobjects import constraint_selfweight
    constraint_selfweight.ConstraintSelfWeight(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_constraint_selfweight
        view_constraint_selfweight.VPConstraintSelfWeight(
            obj.ViewObject
        )
    return obj


def makeConstraintTemperature(
    doc,
    name="ConstraintTemperature"
):
    """makeConstraintTemperature(document, [name]):
    makes a Fem ConstraintTemperature object"""
    obj = doc.addObject("Fem::ConstraintTemperature", name)
    return obj


def makeConstraintTie(
    doc,
    name="ConstraintTie"
):
    """makeConstraintTie(document, [name]):
    creates a tie object to define bonded faces constraint"""
    obj = doc.addObject("Fem::ConstraintPython", name)
    from femobjects import constraint_tie
    constraint_tie.ConstraintTie(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_constraint_tie
        view_constraint_tie.VPConstraintTie(obj.ViewObject)
    return obj


def makeConstraintTransform(
    doc,
    name="ConstraintTransform"
):
    """makeConstraintTransform(document, [name]):
    makes a Fem ConstraintTransform object"""
    obj = doc.addObject("Fem::ConstraintTransform", name)
    return obj


def makeConstraintSectionPrint(
    doc,
    name="ConstraintSectionPrint"
):
    """makeConstraintSectionPrint(document, [name]):
    creates a section print object to evaluate forces and moments of defined face"""
    obj = doc.addObject("Fem::ConstraintPython", name)
    from femobjects import constraint_sectionprint
    constraint_sectionprint.ConstraintSectionPrint(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_constraint_sectionprint
        view_constraint_sectionprint.VPConstraintSectionPrint(obj.ViewObject)
    return obj


def makeConstraintSpring(
    doc,
    name="ConstraintSpring"
):
    """makeConstraintSpring(document, [name]):
    makes a Fem ConstraintSpring object"""
    obj = doc.addObject("Fem::ConstraintSpring", name)
    return obj


# ********* element definition objects ***********************************************************
def makeElementFluid1D(
    doc,
    name="ElementFluid1D"
):
    """makeElementFluid1D(document, [name]):
    creates a 1D fluid element object to define 1D flow"""
    obj = doc.addObject("Fem::FeaturePython", name)
    from femobjects import element_fluid1D
    element_fluid1D.ElementFluid1D(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_element_fluid1D
        view_element_fluid1D.VPElementFluid1D(obj.ViewObject)
    return obj


def makeElementGeometry1D(
    doc,
    sectiontype="Rectangular",
    width=10.0,
    height=25.0,
    name="ElementGeometry1D"
):
    """makeElementGeometry1D(document, [width], [height], [name]):
    creates a 1D geometry element object to define a cross section"""
    obj = doc.addObject("Fem::FeaturePython", name)
    from femobjects import element_geometry1D
    element_geometry1D.ElementGeometry1D(obj)
    sec_types = element_geometry1D.ElementGeometry1D.known_beam_types
    if sectiontype not in sec_types:
        FreeCAD.Console.PrintError("Section type is unknown. Set to " + sec_types[0] + " \n")
        obj.SectionType = sec_types[0]
    else:
        obj.SectionType = sectiontype
    obj.RectWidth = width
    obj.RectHeight = height
    obj.CircDiameter = height
    obj.PipeDiameter = height
    obj.PipeThickness = width
    if FreeCAD.GuiUp:
        from femviewprovider import view_element_geometry1D
        view_element_geometry1D.VPElementGeometry1D(obj.ViewObject)
    return obj


def makeElementGeometry2D(
    doc,
    thickness=20.0,
    name="ElementGeometry2D"
):
    """makeElementGeometry2D(document, [thickness], [name]):
    creates a 2D geometry element object to define a plate thickness"""
    obj = doc.addObject("Fem::FeaturePython", name)
    from femobjects import element_geometry2D
    element_geometry2D.ElementGeometry2D(obj)
    obj.Thickness = thickness
    if FreeCAD.GuiUp:
        from femviewprovider import view_element_geometry2D
        view_element_geometry2D.VPElementGeometry2D(obj.ViewObject)
    return obj


def makeElementRotation1D(
    doc,
    name="ElementRotation1D"
):
    """makeElementRotation1D(document, [name]):
    creates a 1D geometry rotation element object to rotate a 1D cross section"""
    obj = doc.addObject("Fem::FeaturePython", name)
    from femobjects import element_rotation1D
    element_rotation1D.ElementRotation1D(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_element_rotation1D
        view_element_rotation1D.VPElementRotation1D(obj.ViewObject)
    return obj


# ********* material objects *********************************************************************
def makeMaterialFluid(
    doc,
    name="MaterialFluid"
):
    """makeMaterialFluid(document, [name]):
    makes a FEM Material for fluid"""
    obj = doc.addObject("App::MaterialObjectPython", name)
    from femobjects import material_common
    material_common.MaterialCommon(obj)
    obj.Category = "Fluid"
    if FreeCAD.GuiUp:
        from femviewprovider import view_material_common
        view_material_common.VPMaterialCommon(obj.ViewObject)
    return obj


def makeMaterialMechanicalNonlinear(
    doc,
    base_material,
    name="MaterialMechanicalNonlinear"
):
    """makeMaterialMechanicalNonlinear(document, base_material, [name]):
    creates a nonlinear material object"""
    obj = doc.addObject("Fem::FeaturePython", name)
    from femobjects import material_mechanicalnonlinear
    material_mechanicalnonlinear.MaterialMechanicalNonlinear(obj)
    obj.LinearBaseMaterial = base_material
    if FreeCAD.GuiUp:
        from femviewprovider import view_material_mechanicalnonlinear
        view_material_mechanicalnonlinear.VPMaterialMechanicalNonlinear(
            obj.ViewObject
        )
    return obj


def makeMaterialReinforced(
    doc,
    name="MaterialReinforced"
):
    """makeMaterialReinforced(document, [matrix_material], [reinforcement_material], [name]):
    creates a reinforced material object"""
    obj = doc.addObject("App::MaterialObjectPython", name)
    from femobjects import material_reinforced
    material_reinforced.MaterialReinforced(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_material_reinforced
        view_material_reinforced.VPMaterialReinforced(obj.ViewObject)
    return obj


def makeMaterialSolid(
    doc,
    name="MaterialSolid"
):
    """makeMaterialSolid(document, [name]):
    makes a FEM Material for solid"""
    obj = doc.addObject("App::MaterialObjectPython", name)
    from femobjects import material_common
    material_common.MaterialCommon(obj)
    obj.Category = "Solid"
    if FreeCAD.GuiUp:
        from femviewprovider import view_material_common
        view_material_common.VPMaterialCommon(obj.ViewObject)
    return obj


# ********* mesh objects *************************************************************************
def makeMeshBoundaryLayer(
    doc,
    base_mesh,
    name="MeshBoundaryLayer"
):
    """makeMeshBoundaryLayer(document, base_mesh, [name]):
    creates a FEM mesh BoundaryLayer object to define boundary layer properties"""
    obj = doc.addObject("Fem::FeaturePython", name)
    from femobjects import mesh_boundarylayer
    mesh_boundarylayer.MeshBoundaryLayer(obj)
    # obj.BaseMesh = base_mesh
    # App::PropertyLinkList does not support append
    # we will use a temporary list to append the mesh BoundaryLayer obj. to the list
    tmplist = base_mesh.MeshBoundaryLayerList
    tmplist.append(obj)
    base_mesh.MeshBoundaryLayerList = tmplist
    if FreeCAD.GuiUp:
        from femviewprovider import view_mesh_boundarylayer
        view_mesh_boundarylayer.VPMeshBoundaryLayer(obj.ViewObject)
    return obj


def makeMeshGmsh(
    doc,
    name="MeshGmsh"
):
    """makeMeshGmsh(document, [name]):
    makes a Gmsh FEM mesh object"""
    obj = doc.addObject("Fem::FemMeshObjectPython", name)
    from femobjects import mesh_gmsh
    mesh_gmsh.MeshGmsh(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_mesh_gmsh
        view_mesh_gmsh.VPMeshGmsh(obj.ViewObject)
    return obj


def makeMeshGroup(
    doc,
    base_mesh,
    use_label=False,
    name="MeshGroup"
):
    """makeMeshGroup(document, base_mesh, [use_label], [name]):
    creates a FEM mesh refinement object to define properties for a region of a FEM mesh"""
    obj = doc.addObject("Fem::FeaturePython", name)
    from femobjects import mesh_group
    mesh_group.MeshGroup(obj)
    obj.UseLabel = use_label
    # obj.BaseMesh = base_mesh
    # App::PropertyLinkList does not support append
    # we will use a temporary list to append the mesh group obj. to the list
    tmplist = base_mesh.MeshGroupList
    tmplist.append(obj)
    base_mesh.MeshGroupList = tmplist
    if FreeCAD.GuiUp:
        from femviewprovider import view_mesh_group
        view_mesh_group.VPMeshGroup(obj.ViewObject)
    return obj


def makeMeshNetgen(
    doc,
    name="MeshNetgen"
):
    """makeMeshNetgen(document, [name]):
    makes a Fem MeshShapeNetgenObject object"""
    obj = doc.addObject("Fem::FemMeshShapeNetgenObject", name)
    return obj


def makeMeshRegion(
    doc,
    base_mesh,
    element_length=0.0,
    name="MeshRegion"
):
    """makeMeshRegion(document, base_mesh, [element_length], [name]):
    creates a FEM mesh refinement object to define properties for a refinement of a FEM mesh"""
    obj = doc.addObject("Fem::FeaturePython", name)
    from femobjects import mesh_region
    mesh_region.MeshRegion(obj)
    obj.CharacteristicLength = element_length
    # obj.BaseMesh = base_mesh
    # App::PropertyLinkList does not support append
    # we will use a temporary list to append the mesh region obj. to the list
    tmplist = base_mesh.MeshRegionList
    tmplist.append(obj)
    base_mesh.MeshRegionList = tmplist
    if FreeCAD.GuiUp:
        from femviewprovider import view_mesh_region
        view_mesh_region.VPMeshRegion(obj.ViewObject)
    return obj


def makeMeshResult(
    doc,
    name="MeshResult"
):
    """makeMeshResult(document, name): makes a Fem MeshResult object"""
    obj = doc.addObject("Fem::FemMeshObjectPython", name)
    from femobjects import mesh_result
    mesh_result.MeshResult(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_mesh_result
        view_mesh_result.VPFemMeshResult(obj.ViewObject)
    return obj


# ********* post processing objects **************************************************************
def makeResultMechanical(
    doc,
    name="ResultMechanical"
):
    """makeResultMechanical(document, [name]):
    creates a mechanical result object to hold FEM results"""
    obj = doc.addObject("Fem::FemResultObjectPython", name)
    from femobjects import result_mechanical
    result_mechanical.ResultMechanical(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_result_mechanical
        view_result_mechanical.VPResultMechanical(obj.ViewObject)
    return obj


def makePostVtkFilterClipRegion(
    doc,
    base_vtk_result,
    name="VtkFilterClipRegion"
):
    """makePostVtkFilterClipRegion(document, base_vtk_result, [name]):
    creates a FEM post processing region clip filter object (vtk based)"""
    obj = doc.addObject("Fem::FemPostClipFilter", name)
    tmp_filter_list = base_vtk_result.Filter
    tmp_filter_list.append(obj)
    base_vtk_result.Filter = tmp_filter_list
    del tmp_filter_list
    return obj


def makePostVtkFilterClipScalar(
    doc,
    base_vtk_result,
    name="VtkFilterClipScalar"
):
    """makePostVtkFilterClipScalar(document, base_vtk_result, [name]):
    creates a FEM post processing scalar clip filter object (vtk based)"""
    obj = doc.addObject("Fem::FemPostScalarClipFilter", name)
    tmp_filter_list = base_vtk_result.Filter
    tmp_filter_list.append(obj)
    base_vtk_result.Filter = tmp_filter_list
    del tmp_filter_list
    return obj


def makePostVtkFilterCutFunction(
    doc,
    base_vtk_result,
    name="VtkFilterCutFunction"
):
    """makePostVtkFilterCutFunction(document, base_vtk_result, [name]):
    creates a FEM post processing cut function filter object (vtk based)"""
    obj = doc.addObject("Fem::FemPostClipFilter", name)
    tmp_filter_list = base_vtk_result.Filter
    tmp_filter_list.append(obj)
    base_vtk_result.Filter = tmp_filter_list
    del tmp_filter_list
    return obj


def makePostVtkFilterWarp(
    doc,
    base_vtk_result,
    name="VtkFilterWarp"
):
    """makePostVtkFilterWarp(document, base_vtk_result, [name]):
    creates a FEM post processing warp filter object (vtk based)"""
    obj = doc.addObject("Fem::FemPostWarpVectorFilter", name)
    tmp_filter_list = base_vtk_result.Filter
    tmp_filter_list.append(obj)
    base_vtk_result.Filter = tmp_filter_list
    del tmp_filter_list
    return obj


def makePostVtkFilterContours(
    doc,
    base_vtk_result,
    name="VtkFilterContours"
):
    """makePostVtkFilterContours(document, base_vtk_result, [name]):
    creates a FEM post processing contours filter object (vtk based)"""
    obj = doc.addObject("Fem::FemPostContoursFilter", name)
    tmp_filter_list = base_vtk_result.Filter
    tmp_filter_list.append(obj)
    base_vtk_result.Filter = tmp_filter_list
    del tmp_filter_list
    return obj


def makePostVtkResult(
    doc,
    base_result,
    name="VtkResult"
):
    """makePostVtkResult(document, base_result, [name]):
    creates a FEM post processing result object (vtk based) to hold FEM results"""
    Pipeline_Name = "Pipeline_" + name
    obj = doc.addObject("Fem::FemPostPipeline", Pipeline_Name)
    obj.load(base_result)
    if FreeCAD.GuiUp:
        obj.ViewObject.SelectionStyle = "BoundBox"
        # to assure the user sees something, set the default to Surface
        obj.ViewObject.DisplayMode = "Surface"
    return obj


# ********* solver objects ***********************************************************************
def makeEquationDeformation(
    doc,
    base_solver=None,
    name="Deformation"
):
    """makeEquationDeformation(document, [base_solver], [name]):
    creates a FEM deformation (nonlinear elasticity) equation for a solver"""
    from femsolver.elmer.equations import deformation
    obj = deformation.create(doc, name)
    if base_solver:
        base_solver.addObject(obj)
    return obj


def makeEquationElasticity(
    doc,
    base_solver=None,
    name="Elasticity"
):
    """makeEquationElasticity(document, [base_solver], [name]):
    creates a FEM elasticity equation for a solver"""
    from femsolver.elmer.equations import elasticity
    obj = elasticity.create(doc, name)
    if base_solver:
        base_solver.addObject(obj)
    return obj


def makeEquationElectricforce(
    doc,
    base_solver=None,
    name="Electricforce"
):
    """makeEquationElectricforce(document, [base_solver], [name]):
    creates a FEM Electricforce equation for a solver"""
    from femsolver.elmer.equations import electricforce
    obj = electricforce.create(doc, name)
    if base_solver:
        base_solver.addObject(obj)
    return obj


def makeEquationElectrostatic(
    doc,
    base_solver=None,
    name="Electrostatic"
):
    """makeEquationElectrostatic(document, [base_solver], [name]):
    creates a FEM electrostatic equation for a solver"""
    from femsolver.elmer.equations import electrostatic
    obj = electrostatic.create(doc, name)
    if base_solver:
        base_solver.addObject(obj)
    return obj


def makeEquationFlow(
    doc,
    base_solver=None,
    name="Flow"
):
    """makeEquationFlow(document, [base_solver], [name]):
    creates a FEM flow equation for a solver"""
    from femsolver.elmer.equations import flow
    obj = flow.create(doc, name)
    if base_solver:
        base_solver.addObject(obj)
    return obj


def makeEquationFlux(
    doc,
    base_solver=None,
    name="Flux"
):
    """makeEquationFlux(document, [base_solver], [name]):
    creates a FEM flux equation for a solver"""
    from femsolver.elmer.equations import flux
    obj = flux.create(doc, name)
    if base_solver:
        base_solver.addObject(obj)
    return obj


def makeEquationHeat(
    doc,
    base_solver=None,
    name="Heat"
):
    """makeEquationHeat(document, [base_solver], [name]):
    creates a FEM heat equation for a solver"""
    from femsolver.elmer.equations import heat
    obj = heat.create(doc, name)
    if base_solver:
        base_solver.addObject(obj)
    return obj


def makeEquationMagnetodynamic(
    doc,
    base_solver=None,
    name="Magnetodynamic"
):
    """makeEquationMagnetodynamic(document, [base_solver], [name]):
    creates a FEM magnetodynamic equation for a solver"""
    from femsolver.elmer.equations import magnetodynamic
    obj = magnetodynamic.create(doc, name)
    if base_solver:
        base_solver.addObject(obj)
    return obj


def makeEquationMagnetodynamic2D(
    doc,
    base_solver=None,
    name="Magnetodynamic2D"
):
    """makeEquationMagnetodynamic2D(document, [base_solver], [name]):
    creates a FEM magnetodynamic2D equation for a solver"""
    from femsolver.elmer.equations import magnetodynamic2D
    obj = magnetodynamic2D.create(doc, name)
    if base_solver:
        base_solver.addObject(obj)
    return obj


def makeSolverCalculixCcxTools(
    doc,
    name="SolverCcxTools"
):
    """makeSolverCalculixCcxTools(document, [name]):
    makes a Calculix solver object for the ccx tools module"""
    obj = doc.addObject("Fem::FemSolverObjectPython", name)
    from femobjects import solver_ccxtools
    solver_ccxtools.SolverCcxTools(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_solver_ccxtools
        view_solver_ccxtools.VPSolverCcxTools(obj.ViewObject)
    return obj


def makeSolverCalculix(
    doc,
    name="SolverCalculix"
):
    """makeSolverCalculix(document, [name]):
    makes a Calculix solver object"""
    import femsolver.calculix.solver
    obj = femsolver.calculix.solver.create(doc, name)
    return obj


def makeSolverElmer(
    doc,
    name="SolverElmer"
):
    """makeSolverElmer(document, [name]):
    makes a Elmer solver object"""
    import femsolver.elmer.solver
    obj = femsolver.elmer.solver.create(doc, name)
    return obj


def makeSolverMystran(
    doc,
    name="SolverMystran"
):
    """makeSolverMystran(document, [name]):
    makes a Mystran solver object"""
    import femsolver.mystran.solver
    obj = femsolver.mystran.solver.create(doc, name)
    return obj


def makeSolverZ88(
    doc,
    name="SolverZ88"
):
    """makeSolverZ88(document, [name]):
    makes a Z88 solver object"""
    import femsolver.z88.solver
    obj = femsolver.z88.solver.create(doc, name)
    return obj


"""
# get the supportedTypes
App.newDocument()
module = "Fem"
FreeCADGui.doCommand("import " + module)
for s in sorted(App.ActiveDocument.supportedTypes()):
    if s.startswith(module):
        print(s)

"""

##  @}
