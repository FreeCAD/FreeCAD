
# ***************************************************************************
# *   Copyright (c) 2025 Stefan Tröger <stefantroeger@gmx.net>              *
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

__title__ = "GMSH adaptive meshing examples"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"


# **************************************************************************
#
# Important: The generated file is used for gmsh tests. Do not change
# content (geometry, options, names etc.) or add/remove content without
# regenerating the gmsh test data files
#
# **************************************************************************


import math

import FreeCAD
import Part

import ObjectsFem
from .manager import init_doc

def get_information():
    return {
        "name": "GMSH adaptive meshing",
        "meshgeneration": "Refinement",
        "hasanalysis": False
    }


def get_explanation(header=""):
    return (
        header
        + """

To run the example from Python console use:
from femexamples.gmsh_transfinite import setup
setup()

Shows how gmsh refinements can be used to adapt mesh size to result values.

"""
    )

def create_analysis(doc, name, part_obj):
    # setup of a full example document structure
    # returns the part and gmsh mesh object

    analysis = ObjectsFem.makeAnalysis(doc, name)
    gmsh = ObjectsFem.makeMeshGmsh(doc)
    gmsh.Shape = part_obj
    gmsh.CharacteristicLengthMax = 3
    gmsh.ParallelProcessing = False
    analysis.addObject(gmsh)

    # constraint fixed
    con_fixed = ObjectsFem.makeConstraintFixed(doc, "FemConstraintFixed")
    con_fixed.References = [(part_obj, "Face1")]
    analysis.addObject(con_fixed)

    # constraint force
    con_force = ObjectsFem.makeConstraintForce(doc, "FemConstraintForce")
    con_force.References = [(part_obj, "Face4")]
    con_force.Force = "4000.0 N"
    analysis.addObject(con_force)

    # material
    material_obj = ObjectsFem.makeMaterialSolid(doc, "MechanicalMaterial")
    mat = material_obj.Material
    mat["Name"] = "Steel-Generic"
    mat["YoungsModulus"] = "200000 MPa"
    mat["PoissonRatio"] = "0.30"
    mat["Density"] = "7900 kg/m^3"
    material_obj.Material = mat
    analysis.addObject(material_obj)

    # solver
    solver_obj = ObjectsFem.makeSolverCalculiXCcxTools(doc, "CalculiXCcxTools")
    analysis.addObject(solver_obj)

    return analysis, gmsh


def setup(doc=None, solver=None):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # create geomerty
    Part.makeBox
    b1 = Part.makeBox(10,30,10)
    b2 = Part.makeBox(30,10,10)
    b = b1.fuse(b2)
    c = Part.makeCylinder(6 ,10, FreeCAD.Vector(10,10,0))
    s = b.cut(c).removeSplitter()

    part_obj = doc.addObject("Part::Feature", "Geometry")
    part_obj.Shape = s

    # initial analysis for stress estimation
    # with pipeline so that we can setup the adaptive analysis already
    analysis, gmsh = create_analysis(doc, "Initial Analysis", part_obj)
    pipeline = doc.addObject("Fem::FemPostPipeline", "Result")
    analysis.addObject(pipeline)

    # Adapted Analysis
    analysis, gmsh = create_analysis(doc, "Adaptive Analysis", part_obj)

    gmsh.CharacteristicLengthMax = 10

    # result refinement
    result = ObjectsFem.makeMeshAdvanced(doc, gmsh, "StressResult")
    result.Type = "Result"
    result.ResultObject = pipeline
    result.ResultField = "von Mises Stress"

    # mean refinement for smoothing
    mean = ObjectsFem.makeMeshManipulate(doc, gmsh, "Mean")
    mean.Type = "Mean"
    mean.Delta = "10mm"
    mean.Refinement = result

    # math refinement
    math = ObjectsFem.makeMeshAdvanced(doc, gmsh, "Math")
    math.Type = "MathEval"
    math.Equation = "3/(F1/1e8)"
    math.Refinements = [mean]

    gmsh.MeshRefinementList = [math]

    return doc







