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
def makeAnalysis(doc, name="Analysis"):
    """makeAnalysis(document, [name]):
    makes a Fem Analysis object"""
    obj = doc.addObject("Fem::FemAnalysis", name)
    return obj


# ********* constant objects *********************************************************************
def makeConstantVacuumPermittivity(doc, name="ConstantVacuumPermittivity"):
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
def makeConstraintBearing(doc, name="ConstraintBearing"):
    """makeConstraintBearing(document, [name]):
    makes a Fem ConstraintBearing object"""
    obj = doc.addObject("Fem::ConstraintBearing", name)
    return obj


def makeConstraintBodyHeatSource(doc, name="ConstraintBodyHeatSource"):
    """makeConstraintBodyHeatSource(document, [name]):
    makes a Fem ConstraintBodyHeatSource object"""
    obj = doc.addObject("Fem::ConstraintPython", name)
    from femobjects import constraint_bodyheatsource

    constraint_bodyheatsource.ConstraintBodyHeatSource(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_constraint_bodyheatsource as viewprov

        viewprov.VPConstraintBodyHeatSource(obj.ViewObject)
    return obj


def makeConstraintCentrif(doc, name="ConstraintCentrif"):
    """makeConstraintCentrif(document, [name]):
    creates a centrif object to define centrifugal body load constraint"""
    obj = doc.addObject("Fem::ConstraintPython", name)
    from femobjects import constraint_centrif

    constraint_centrif.ConstraintCentrif(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_constraint_centrif

        view_constraint_centrif.VPConstraintCentrif(obj.ViewObject)
    return obj


def makeConstraintCurrentDensity(doc, name="ConstraintCurrentDensity"):
    """makeConstraintCurrentDensity(document, [name]):
    makes a Fem CurrentDensity object"""
    obj = doc.addObject("Fem::ConstraintPython", name)
    from femobjects import constraint_currentdensity

    constraint_currentdensity.ConstraintCurrentDensity(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_constraint_currentdensity

        view_constraint_currentdensity.VPConstraintCurrentDensity(obj.ViewObject)
    return obj


def makeConstraintContact(doc, name="ConstraintContact"):
    """makeConstraintContact(document, [name]):
    makes a Fem ConstraintContact object"""
    obj = doc.addObject("Fem::ConstraintContact", name)
    return obj


def makeConstraintDisplacement(doc, name="ConstraintDisplacement"):
    """makeConstraintDisplacement(document, [name]):
    makes a Fem ConstraintDisplacement object"""
    obj = doc.addObject("Fem::ConstraintDisplacement", name)
    return obj


def makeConstraintElectricChargeDensity(doc, name="ElectricChargeDensity"):
    """makeConstraintElectricChargeDensity(document, [name]):
    makes a Fem ElectricChargeDensity object"""
    obj = doc.addObject("Fem::ConstraintPython", name)
    from femobjects import constraint_electricchargedensity

    constraint_electricchargedensity.ConstraintElectricChargeDensity(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_constraint_electricchargedensity

        view_constraint_electricchargedensity.VPConstraintElectricChargeDensity(obj.ViewObject)
    return obj


def makeConstraintElectrostaticPotential(doc, name="ConstraintElectrostaticPotential"):
    """makeConstraintElectrostaticPotential(document, [name]):
    makes a Fem ElectrostaticPotential object"""
    obj = doc.addObject("Fem::ConstraintPython", name)
    from femobjects import constraint_electrostaticpotential

    constraint_electrostaticpotential.ConstraintElectrostaticPotential(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_constraint_electrostaticpotential

        view_constraint_electrostaticpotential.VPConstraintElectroStaticPotential(obj.ViewObject)
    return obj


def makeConstraintFixed(doc, name="ConstraintFixed"):
    """makeConstraintFixed(document, [name]):
    makes a Fem ConstraintFixed object"""
    obj = doc.addObject("Fem::ConstraintFixed", name)
    return obj


def makeConstraintRigidBody(doc, name="ConstraintRigidBody"):
    """makeConstraintRigidBody(document, [name]):
    makes a Fem ConstraintRigidBody object"""
    obj = doc.addObject("Fem::ConstraintRigidBody", name)
    return obj


def makeConstraintFlowVelocity(doc, name="ConstraintFlowVelocity"):
    """makeConstraintFlowVelocity(document, [name]):
    makes a Fem ConstraintFlowVelocity object"""
    obj = doc.addObject("Fem::ConstraintPython", name)
    from femobjects import constraint_flowvelocity

    constraint_flowvelocity.ConstraintFlowVelocity(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_constraint_flowvelocity

        view_constraint_flowvelocity.VPConstraintFlowVelocity(obj.ViewObject)
    return obj


def makeConstraintFluidBoundary(doc, name="ConstraintFluidBoundary"):
    """makeConstraintFluidBoundary(document, name):
    makes a Fem ConstraintFluidBoundary object"""
    obj = doc.addObject("Fem::ConstraintFluidBoundary", name)
    return obj


def makeConstraintForce(doc, name="ConstraintForce"):
    """makeConstraintForce(document, [name]):
    makes a Fem ConstraintForce object"""
    obj = doc.addObject("Fem::ConstraintForce", name)
    return obj


def makeConstraintGear(doc, name="ConstraintGear"):
    """makeConstraintGear(document, [name]):
    makes a Fem ConstraintGear object"""
    obj = doc.addObject("Fem::ConstraintGear", name)
    return obj


def makeConstraintHeatflux(doc, name="ConstraintHeatflux"):
    """makeConstraintHeatflux(document, [name]):
    makes a Fem ConstraintHeatflux object"""
    obj = doc.addObject("Fem::ConstraintHeatflux", name)
    return obj


def makeConstraintInitialFlowVelocity(doc, name="ConstraintInitialFlowVelocity"):
    """makeConstraintInitialFlowVelocity(document, [name]):
    makes a Fem ConstraintInitialFlowVelocity object"""
    obj = doc.addObject("Fem::ConstraintPython", name)
    from femobjects import constraint_initialflowvelocity

    constraint_initialflowvelocity.ConstraintInitialFlowVelocity(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_constraint_initialflowvelocity

        view_constraint_initialflowvelocity.VPConstraintInitialFlowVelocity(obj.ViewObject)
    return obj


def makeConstraintInitialPressure(doc, name="ConstraintInitialPressure"):
    """makeConstraintInitialPressure(document, [name]):
    makes a Fem ConstraintInitialPressure object"""
    obj = doc.addObject("Fem::ConstraintPython", name)
    from femobjects import constraint_initialpressure

    constraint_initialpressure.ConstraintInitialPressure(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_constraint_initialpressure

        view_constraint_initialpressure.VPConstraintInitialPressure(obj.ViewObject)
    return obj


def makeConstraintInitialTemperature(doc, name="ConstraintInitialTemperature"):
    """makeConstraintInitialTemperature(document, name):
    makes a Fem ConstraintInitialTemperature object"""
    obj = doc.addObject("Fem::ConstraintInitialTemperature", name)
    return obj


def makeConstraintMagnetization(doc, name="ConstraintMagnetization"):
    """makeConstraintMagnetization(document, [name]):
    makes a Fem Magnetization object"""
    obj = doc.addObject("Fem::ConstraintPython", name)
    from femobjects import constraint_magnetization

    constraint_magnetization.ConstraintMagnetization(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_constraint_magnetization

        view_constraint_magnetization.VPConstraintMagnetization(obj.ViewObject)
    return obj


def makeConstraintPlaneRotation(doc, name="ConstraintPlaneRotation"):
    """makeConstraintPlaneRotation(document, [name]):
    makes a Fem ConstraintPlaneRotation object"""
    obj = doc.addObject("Fem::ConstraintPlaneRotation", name)
    return obj


def makeConstraintPressure(doc, name="ConstraintPressure"):
    """makeConstraintPressure(document, [name]):
    makes a Fem ConstraintPressure object"""
    obj = doc.addObject("Fem::ConstraintPressure", name)
    return obj


def makeConstraintPulley(doc, name="ConstraintPulley"):
    """makeConstraintPulley(document, [name]):
    makes a Fem ConstraintPulley object"""
    obj = doc.addObject("Fem::ConstraintPulley", name)
    return obj


def makeConstraintSelfWeight(doc, name="ConstraintSelfWeight"):
    """makeConstraintSelfWeight(document, [name]):
    creates a self weight object to define a gravity load"""
    obj = doc.addObject("Fem::ConstraintPython", name)
    from femobjects import constraint_selfweight

    constraint_selfweight.ConstraintSelfWeight(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_constraint_selfweight

        view_constraint_selfweight.VPConstraintSelfWeight(obj.ViewObject)
    return obj


def makeConstraintTemperature(doc, name="ConstraintTemperature"):
    """makeConstraintTemperature(document, [name]):
    makes a Fem ConstraintTemperature object"""
    obj = doc.addObject("Fem::ConstraintTemperature", name)
    return obj


def makeConstraintTie(doc, name="ConstraintTie"):
    """makeConstraintTie(document, [name]):
    creates a tie object to define bonded faces constraint"""
    obj = doc.addObject("Fem::ConstraintPython", name)
    from femobjects import constraint_tie

    constraint_tie.ConstraintTie(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_constraint_tie

        view_constraint_tie.VPConstraintTie(obj.ViewObject)
    return obj


def makeConstraintTransform(doc, name="ConstraintTransform"):
    """makeConstraintTransform(document, [name]):
    makes a Fem ConstraintTransform object"""
    obj = doc.addObject("Fem::ConstraintTransform", name)
    return obj


def makeConstraintSectionPrint(doc, name="ConstraintSectionPrint"):
    """makeConstraintSectionPrint(document, [name]):
    creates a section print object to evaluate forces and moments of defined face"""
    obj = doc.addObject("Fem::ConstraintPython", name)
    from femobjects import constraint_sectionprint

    constraint_sectionprint.ConstraintSectionPrint(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_constraint_sectionprint

        view_constraint_sectionprint.VPConstraintSectionPrint(obj.ViewObject)
    return obj


def makeConstraintSpring(doc, name="ConstraintSpring"):
    """makeConstraintSpring(document, [name]):
    makes a Fem ConstraintSpring object"""
    obj = doc.addObject("Fem::ConstraintSpring", name)
    return obj


# ********* element definition objects ***********************************************************
def makeElementFluid1D(doc, name="ElementFluid1D"):
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
    doc, sectiontype="Rectangular", width=10.0, height=25.0, thickness=2.0, name="ElementGeometry1D"
):
    """makeElementGeometry1D(document, [width], [height], [name]):
    creates a 1D geometry element object to define a cross section"""
    obj = doc.addObject("Fem::FeaturePython", name)
    from femobjects import element_geometry1D

    element_geometry1D.ElementGeometry1D(obj)

    obj.SectionType = sectiontype
    obj.RectWidth = width
    obj.RectHeight = height
    obj.CircDiameter = height
    obj.PipeDiameter = height
    obj.PipeThickness = thickness
    obj.Axis1Length = width
    obj.Axis2Length = height
    obj.BoxHeight = height
    obj.BoxWidth = width
    obj.BoxT1 = thickness
    obj.BoxT2 = thickness
    obj.BoxT3 = thickness
    obj.BoxT4 = thickness

    if FreeCAD.GuiUp:
        from femviewprovider import view_element_geometry1D

        view_element_geometry1D.VPElementGeometry1D(obj.ViewObject)
    return obj


def makeElementGeometry2D(doc, thickness=1.0, name="ElementGeometry2D"):
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


def makeElementRotation1D(doc, name="ElementRotation1D"):
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
def makeMaterialFluid(doc, name="MaterialFluid"):
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


def makeMaterialMechanicalNonlinear(doc, base_material, name="MaterialMechanicalNonlinear"):
    """makeMaterialMechanicalNonlinear(document, base_material, [name]):
    creates a nonlinear material object"""
    obj = doc.addObject("Fem::FeaturePython", name)
    from femobjects import material_mechanicalnonlinear

    material_mechanicalnonlinear.MaterialMechanicalNonlinear(obj)
    base_material.Nonlinear = obj
    if FreeCAD.GuiUp:
        from femviewprovider import view_material_mechanicalnonlinear

        view_material_mechanicalnonlinear.VPMaterialMechanicalNonlinear(obj.ViewObject)
    return obj


def makeMaterialReinforced(doc, name="MaterialReinforced"):
    """makeMaterialReinforced(document, [matrix_material], [reinforcement_material], [name]):
    creates a reinforced material object"""
    obj = doc.addObject("App::MaterialObjectPython", name)
    from femobjects import material_reinforced

    material_reinforced.MaterialReinforced(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_material_reinforced

        view_material_reinforced.VPMaterialReinforced(obj.ViewObject)
    return obj


def makeMaterialSolid(doc, name="MaterialSolid"):
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
def makeMeshBoundaryLayer(doc, base_mesh, name="MeshBoundaryLayer"):
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


def makeMeshGmsh(doc, name="MeshGmsh"):
    """makeMeshGmsh(document, [name]):
    makes a Gmsh FEM mesh object"""
    obj = doc.addObject("Fem::FemMeshShapeBaseObjectPython", name)
    from femobjects import mesh_gmsh

    mesh_gmsh.MeshGmsh(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_mesh_gmsh

        view_mesh_gmsh.VPMeshGmsh(obj.ViewObject)
    return obj


def makeMeshGroup(doc, base_mesh, use_label=False, name="MeshGroup"):
    """makeMeshGroup(document, base_mesh, [use_label], [name]):
    creates a FEM mesh refinement object to define properties for a region of a FEM mesh
    """
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


def makeMeshNetgen(doc, name="MeshNetgen"):
    """makeMeshNetgen(document, [name]):
    makes a Netgen FEM mesh object"""
    obj = doc.addObject("Fem::FemMeshShapeBaseObjectPython", name)
    from femobjects import mesh_netgen

    mesh_netgen.MeshNetgen(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_mesh_netgen

        view_mesh_netgen.VPMeshNetgen(obj.ViewObject)
    return obj


def makeMeshNetgenLegacy(doc, name="MeshNetgen"):
    """makeMeshNetgenLegacy(document, [name]):
    makes a old implementation Netgen FEM mesh object"""
    obj = doc.addObject("Fem::FemMeshShapeNetgenObject", name)
    return obj


def makeMeshRegion(doc, base_mesh, element_length=0.0, name="MeshRegion"):
    """makeMeshRegion(document, base_mesh, [element_length], [name]):
    creates a FEM mesh refinement object to define properties for a refinement of a FEM mesh
    """
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


def makeMeshResult(doc, name="MeshResult"):
    """makeMeshResult(document, name): makes a Fem MeshResult object"""
    obj = doc.addObject("Fem::FemMeshObjectPython", name)
    from femobjects import mesh_result

    mesh_result.MeshResult(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_mesh_result

        view_mesh_result.VPFemMeshResult(obj.ViewObject)
    return obj


# ********* post processing objects **************************************************************
def makeResultMechanical(doc, name="ResultMechanical"):
    """makeResultMechanical(document, [name]):
    creates a mechanical result object to hold FEM results"""
    obj = doc.addObject("Fem::FemResultObjectPython", name)
    from femobjects import result_mechanical

    result_mechanical.ResultMechanical(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_result_mechanical

        view_result_mechanical.VPResultMechanical(obj.ViewObject)
    return obj


def makePostVtkFilterClipRegion(doc, base_vtk_result, name="VtkFilterClipRegion"):
    """makePostVtkFilterClipRegion(document, base_vtk_result, [name]):
    creates a FEM post processing region clip filter object (vtk based)"""
    obj = doc.addObject("Fem::FemPostClipFilter", name)
    base_vtk_result.addObject(obj)
    return obj


def makePostVtkFilterClipScalar(doc, base_vtk_result, name="VtkFilterClipScalar"):
    """makePostVtkFilterClipScalar(document, base_vtk_result, [name]):
    creates a FEM post processing scalar clip filter object (vtk based)"""
    obj = doc.addObject("Fem::FemPostScalarClipFilter", name)
    base_vtk_result.addObject(obj)
    return obj


def makePostVtkFilterCutFunction(doc, base_vtk_result, name="VtkFilterCutFunction"):
    """makePostVtkFilterCutFunction(document, base_vtk_result, [name]):
    creates a FEM post processing cut function filter object (vtk based)"""
    obj = doc.addObject("Fem::FemPostClipFilter", name)
    base_vtk_result.addObject(obj)
    return obj


def makePostVtkFilterWarp(doc, base_vtk_result, name="VtkFilterWarp"):
    """makePostVtkFilterWarp(document, base_vtk_result, [name]):
    creates a FEM post processing warp filter object (vtk based)"""
    obj = doc.addObject("Fem::FemPostWarpVectorFilter", name)
    base_vtk_result.addObject(obj)
    return obj


def makePostVtkFilterContours(doc, base_vtk_result, name="VtkFilterContours"):
    """makePostVtkFilterContours(document, base_vtk_result, [name]):
    creates a FEM post processing contours filter object (vtk based)"""
    obj = doc.addObject("Fem::FemPostContoursFilter", name)
    base_vtk_result.addObject(obj)
    return obj


def makePostFilterGlyph(doc, base_vtk_result, name="Glyph"):
    """makePostVtkFilterGlyph(document, [name]):
    creates a FEM post processing filter that visualizes vector fields with glyphs
    """
    obj = doc.addObject("Fem::PostFilterPython", name)
    from femobjects import post_glyphfilter

    post_glyphfilter.PostGlyphFilter(obj)
    base_vtk_result.addObject(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_post_glyphfilter

        view_post_glyphfilter.VPPostGlyphFilter(obj.ViewObject)
    return obj


def makePostVtkResult(doc, result_data, name="VtkResult"):
    """makePostVtkResult(document, base_result, [name]):
    creates a FEM post processing result data (vtk based) to hold FEM results
    Note: Result data get expanded, it can either be single result [result] or everything
          needed for a multistep result: [results_list, value_list, unit, description]
    """

    Pipeline_Name = "Pipeline_" + name
    obj = doc.addObject("Fem::FemPostPipeline", Pipeline_Name)
    obj.load(*result_data)
    if FreeCAD.GuiUp:
        obj.ViewObject.SelectionStyle = "BoundBox"
        # to assure the user sees something, set the default to Surface
        obj.ViewObject.DisplayMode = "Surface"
    return obj


def makePostLineplot(doc, name="Lineplot"):
    """makePostLineplot(document, [name]):
    creates a FEM post processing line plot
    """
    obj = doc.addObject("Fem::FeaturePython", name)
    from femobjects import post_lineplot

    post_lineplot.PostLineplot(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_post_lineplot

        view_post_lineplot.VPPostLineplot(obj.ViewObject)
    return obj


def makePostLineplotFieldData(doc, name="FieldData2D"):
    """makePostLineplotFieldData(document, [name]):
    creates a FEM post processing data extractor for 2D Field data
    """
    obj = doc.addObject("Fem::FeaturePython", name)
    from femobjects import post_lineplot

    post_lineplot.PostLineplotFieldData(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_post_lineplot

        view_post_lineplot.VPPostLineplotFieldData(obj.ViewObject)
    return obj


def makePostLineplotIndexOverFrames(doc, name="IndexOverFrames2D"):
    """makePostLineplotIndexOverFrames(document, [name]):
    creates a FEM post processing data extractor for 2D index data
    """
    obj = doc.addObject("Fem::FeaturePython", name)
    from femobjects import post_lineplot

    post_lineplot.PostLineplotIndexOverFrames(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_post_lineplot

        view_post_lineplot.VPPostLineplotIndexOverFrames(obj.ViewObject)
    return obj


def makePostHistogram(doc, name="Histogram"):
    """makePostHistogram(document, [name]):
    creates a FEM post processing histogram plot
    """
    obj = doc.addObject("Fem::FeaturePython", name)
    from femobjects import post_histogram

    post_histogram.PostHistogram(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_post_histogram

        view_post_histogram.VPPostHistogram(obj.ViewObject)
    return obj


def makePostHistogramFieldData(doc, name="FieldData1D"):
    """makePostHistogramFieldData(document, [name]):
    creates a FEM post processing data extractor for 1D Field data
    """
    obj = doc.addObject("Fem::FeaturePython", name)
    from femobjects import post_histogram

    post_histogram.PostHistogramFieldData(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_post_histogram

        view_post_histogram.VPPostHistogramFieldData(obj.ViewObject)
    return obj


def makePostHistogramIndexOverFrames(doc, name="IndexOverFrames1D"):
    """makePostHistogramIndexOverFrames(document, [name]):
    creates a FEM post processing data extractor for 1D Field data
    """
    obj = doc.addObject("Fem::FeaturePython", name)
    from femobjects import post_histogram

    post_histogram.PostHistogramIndexOverFrames(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_post_histogram

        view_post_histogram.VPPostHistogramIndexOverFrames(obj.ViewObject)
    return obj


def makePostTable(doc, name="Table"):
    """makePostTable(document, [name]):
    creates a FEM post processing histogram plot
    """
    obj = doc.addObject("Fem::FeaturePython", name)
    from femobjects import post_table

    post_table.PostTable(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_post_table

        view_post_table.VPPostTable(obj.ViewObject)
    return obj


def makePostTableFieldData(doc, name="FieldData1D"):
    """makePostTableFieldData(document, [name]):
    creates a FEM post processing data extractor for 1D Field data
    """
    obj = doc.addObject("Fem::FeaturePython", name)
    from femobjects import post_table

    post_table.PostTableFieldData(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_post_table

        view_post_table.VPPostTableFieldData(obj.ViewObject)
    return obj


def makePostTableIndexOverFrames(doc, name="IndexOverFrames1D"):
    """makePostTableIndexOverFrames(document, [name]):
    creates a FEM post processing data extractor for 1D Field data
    """
    obj = doc.addObject("Fem::FeaturePython", name)
    from femobjects import post_table

    post_table.PostTableIndexOverFrames(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_post_table

        view_post_table.VPPostTableIndexOverFrames(obj.ViewObject)
    return obj


# ********* solver objects ***********************************************************************
def _equation_creator(creator, base_solver, doc, name):
    eq = creator.create(doc, name)
    if base_solver:
        eq.Priority = 255 - len(base_solver.Group)
        base_solver.addObject(eq)
    return eq


def makeEquationDeformation(doc, base_solver=None, name="Deformation"):
    """makeEquationDeformation(document, [base_solver], [name]):
    creates a FEM deformation (nonlinear elasticity) equation for a solver"""
    from femsolver.elmer.equations import deformation

    return _equation_creator(deformation, base_solver, doc, name)


def makeEquationElasticity(doc, base_solver=None, name="Elasticity"):
    """makeEquationElasticity(document, [base_solver], [name]):
    creates a FEM elasticity equation for a solver"""
    from femsolver.elmer.equations import elasticity

    return _equation_creator(elasticity, base_solver, doc, name)


def makeEquationElectricforce(doc, base_solver=None, name="Electricforce"):
    """makeEquationElectricforce(document, [base_solver], [name]):
    creates a FEM Electricforce equation for a solver"""
    from femsolver.elmer.equations import electricforce

    return _equation_creator(electricforce, base_solver, doc, name)


def makeEquationElectrostatic(doc, base_solver=None, name="Electrostatic"):
    """makeEquationElectrostatic(document, [base_solver], [name]):
    creates a FEM electrostatic equation for a solver"""
    from femsolver.elmer.equations import electrostatic

    return _equation_creator(electrostatic, base_solver, doc, name)


def makeEquationFlow(doc, base_solver=None, name="Flow"):
    """makeEquationFlow(document, [base_solver], [name]):
    creates a FEM flow equation for a solver"""
    from femsolver.elmer.equations import flow

    return _equation_creator(flow, base_solver, doc, name)


def makeEquationFlux(doc, base_solver=None, name="Flux"):
    """makeEquationFlux(document, [base_solver], [name]):
    creates a FEM flux equation for a solver"""
    from femsolver.elmer.equations import flux

    return _equation_creator(flux, base_solver, doc, name)


def makeEquationHeat(doc, base_solver=None, name="Heat"):
    """makeEquationHeat(document, [base_solver], [name]):
    creates a FEM heat equation for a solver"""
    from femsolver.elmer.equations import heat

    return _equation_creator(heat, base_solver, doc, name)


def makeEquationMagnetodynamic(doc, base_solver=None, name="Magnetodynamic"):
    """makeEquationMagnetodynamic(document, [base_solver], [name]):
    creates a FEM magnetodynamic equation for a solver"""
    from femsolver.elmer.equations import magnetodynamic

    return _equation_creator(magnetodynamic, base_solver, doc, name)


def makeEquationMagnetodynamic2D(doc, base_solver=None, name="Magnetodynamic2D"):
    """makeEquationMagnetodynamic2D(document, [base_solver], [name]):
    creates a FEM magnetodynamic2D equation for a solver"""
    from femsolver.elmer.equations import magnetodynamic2D

    return _equation_creator(magnetodynamic2D, base_solver, doc, name)


def makeEquationStaticCurrent(doc, base_solver=None, name="StaticCurrent"):
    """makeEquationStaticCurrent(document, [base_solver], [name]):
    creates a FEM static current equation for a solver"""
    from femsolver.elmer.equations import staticcurrent

    return _equation_creator(staticcurrent, base_solver, doc, name)


def makeSolverCalculiXCcxTools(doc, name="SolverCcxTools"):
    """makeSolverCalculiXCcxTools(document, [name]):
    makes a Calculix solver object for the ccx tools module"""
    obj = doc.addObject("Fem::FemSolverObjectPython", name)
    from femobjects import solver_ccxtools

    solver_ccxtools.SolverCcxTools(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_solver_ccxtools

        view_solver_ccxtools.VPSolverCcxTools(obj.ViewObject)
    return obj


def makeSolverCalculiX(doc, name="SolverCalculiX"):
    """makeSolverCalculiX(document, [name]):
    makes a Calculix solver object"""
    obj = doc.addObject("Fem::FemSolverObjectPython", name)
    from femobjects import solver_calculix

    solver_calculix.SolverCalculiX(obj)
    if FreeCAD.GuiUp:
        from femviewprovider import view_solver_calculix

        view_solver_calculix.VPSolverCalculiX(obj.ViewObject)
    return obj


def makeSolverElmer(doc, name="SolverElmer"):
    """makeSolverElmer(document, [name]):
    makes a Elmer solver object"""
    obj = doc.addObject("Fem::FemSolverObjectPython", name)
    from femobjects import solver_elmer

    solver_elmer.SolverElmer(obj)
    obj.SimulationType = "Steady State"
    if FreeCAD.GuiUp:
        from femviewprovider import view_solver_elmer

        view_solver_elmer.VPSolverElmer(obj.ViewObject)
    return obj


def makeSolverMystran(doc, name="SolverMystran"):
    """makeSolverMystran(document, [name]):
    makes a Mystran solver object"""
    import femsolver.mystran.solver

    obj = femsolver.mystran.solver.create(doc, name)
    return obj


def makeSolverZ88(doc, name="SolverZ88"):
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
