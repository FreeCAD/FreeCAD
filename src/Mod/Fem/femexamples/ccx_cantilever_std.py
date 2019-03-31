# ***************************************************************************
# *   Copyright (c) 2019 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************


import FreeCAD
import ObjectsFem
import Fem

mesh_name = 'Mesh'  # needs to be Mesh to work with unit tests


def init_doc(doc=None):
    if doc is None:
        doc = FreeCAD.newDocument()
    return doc


def setup_cantileverbase(doc=None, solver='ccxtools'):
    # setup CalculiX cantilever base model

    if doc is None:
        doc = init_doc()

    # part
    box_obj = doc.addObject('Part::Box', 'Box')
    box_obj.Height = box_obj.Width = 1000
    box_obj.Length = 8000

    # analysis
    analysis = ObjectsFem.makeAnalysis(doc, 'Analysis')

    solver
    # TODO How to pass multiple solver for one analysis in one doc
    if solver is None:
        pass  # no solver is added
    elif solver is 'calculix':
        solver_object = analysis.addObject(ObjectsFem.makeSolverCalculix(doc, 'SolverCalculiX'))[0]
        solver_object.AnalysisType = 'static'
        solver_object.GeometricalNonlinearity = 'linear'
        solver_object.ThermoMechSteadyState = False
        solver_object.MatrixSolverType = 'default'
        solver_object.IterationsControlParameterTimeUse = False
    elif solver is 'ccxtools':
        solver_object = analysis.addObject(ObjectsFem.makeSolverCalculixCcxTools(doc, 'CalculiXccxTools'))[0]
        solver_object.AnalysisType = 'static'
        solver_object.GeometricalNonlinearity = 'linear'
        solver_object.ThermoMechSteadyState = False
        solver_object.MatrixSolverType = 'default'
        solver_object.IterationsControlParameterTimeUse = False
        solver_object.WorkingDir = u''
    elif solver is 'elmer':
        analysis.addObject(ObjectsFem.makeSolverElmer(doc, 'SolverElmer'))
    elif solver is 'z88':
        analysis.addObject(ObjectsFem.makeSolverZ88(doc, 'SolverZ88'))

    # material
    material_object = analysis.addObject(ObjectsFem.makeMaterialSolid(doc, 'FemMaterial'))[0]
    mat = material_object.Material
    mat['Name'] = "CalculiX-Steel"
    mat['YoungsModulus'] = "210000 MPa"
    mat['PoissonRatio'] = "0.30"
    mat['Density'] = "7900 kg/m^3"
    mat['ThermalExpansionCoefficient'] = "0.012 mm/m/K"
    material_object.Material = mat

    # fixed_constraint
    fixed_constraint = analysis.addObject(ObjectsFem.makeConstraintFixed(doc, name="ConstraintFixed"))[0]
    fixed_constraint.References = [(doc.Box, "Face1")]

    # mesh
    from femexamples.meshes.mesh_canticcx_tetra10 import create_nodes, create_elements
    fem_mesh = Fem.FemMesh()
    control = create_nodes(fem_mesh)
    if not control:
        print('ERROR on creating nodes')
    control = create_elements(fem_mesh)
    if not control:
        print('ERROR on creating elements')
    femmesh_obj = analysis.addObject(doc.addObject('Fem::FemMeshObject', mesh_name))[0]
    femmesh_obj.FemMesh = fem_mesh

    doc.recompute()
    return doc


def setup_cantileverfaceload(doc=None, solver='ccxtools'):
    # setup CalculiX cantilever, apply 9 MN on surface of front end face

    doc = setup_cantileverbase(doc, solver)

    # force_constraint
    force_constraint = doc.Analysis.addObject(ObjectsFem.makeConstraintForce(doc, name="ConstraintForce"))[0]
    force_constraint.References = [(doc.Box, "Face2")]
    force_constraint.Force = 9000000.0
    force_constraint.Direction = (doc.Box, ["Edge5"])
    force_constraint.Reversed = True

    doc.recompute()
    return doc


def setup_cantilevernodeload(doc=None, solver='ccxtools'):
    # setup CalculiX cantilever, apply 9 MN on the 4 nodes of the front end face

    doc = setup_cantileverbase(doc, solver)

    # force_constraint
    force_constraint = doc.Analysis.addObject(ObjectsFem.makeConstraintForce(doc, name="ConstraintForce"))[0]
    force_constraint.References = [(doc.Box, "Vertex5"), (doc.Box, "Vertex6"), (doc.Box, "Vertex7"), (doc.Box, "Vertex8")]  # should be possible in one tuple too
    force_constraint.Force = 9000000.0
    force_constraint.Direction = (doc.Box, ["Edge5"])
    force_constraint.Reversed = True

    doc.recompute()
    return doc


def setup_cantileverprescribeddisplacement(doc=None, solver='ccxtools'):
    # setup CalculiX cantilever, apply a prescribed displacement of 250 mm in -z on the front end face

    doc = setup_cantileverbase(doc, solver)

    # displacement_constraint
    displacement_constraint = doc.Analysis.addObject(ObjectsFem.makeConstraintDisplacement(doc, name="ConstraintDisplacmentPrescribed"))[0]
    displacement_constraint.References = [(doc.Box, "Face2")]
    displacement_constraint.zFix = False
    displacement_constraint.zFree = False
    displacement_constraint.zDisplacement = -250.0

    doc.recompute()
    return doc


'''
from femexamples import ccx_cantilever_std as canti

canti.setup_cantileverbase()
canti.setup_cantileverfaceload()
canti.setup_cantilevernodeload()
canti.setup_cantileverprescribeddisplacement()

'''
