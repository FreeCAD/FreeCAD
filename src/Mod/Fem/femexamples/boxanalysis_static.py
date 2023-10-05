# ***************************************************************************
# *   Copyright (c) 2019 Bernd Hahnebach <bernd@bimstatik.org>              *
# *   Copyright (c) 2020 Sudhanshu Dubey <sudhanshu.thethunder@gmail.com    *
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

import FreeCAD

import ObjectsFem

from . import manager
from .boxanalysis_base import setup_boxanalysisbase
from .manager import init_doc


def get_information():
    return {
        "name": "Box Analysis Static",
        "meshtype": "solid",
        "meshelement": "Tet10",
        "constraints": ["fixed", "force", "pressure"],
        "solvers": ["calculix", "ccxtools", "elmer"],
        "material": "solid",
        "equations": ["mechanical"]
    }


def get_explanation(header=""):
    return header + """

To run the example from Python console use:
from femexamples.boxanalysis_static import setup
setup()


See forum topic post:
...

"""


def setup(doc=None, solvertype="ccxtools"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # setup box static, add a fixed, force and a pressure constraint
    doc = setup_boxanalysisbase(doc, solvertype)
    geom_obj = doc.Box
    analysis = doc.Analysis

    # solver
    if solvertype == "calculix":
        solver_obj = ObjectsFem.makeSolverCalculix(doc, "SolverCalculiX")
    elif solvertype == "ccxtools":
        solver_obj = ObjectsFem.makeSolverCalculixCcxTools(doc, "CalculiXccxTools")
        solver_obj.WorkingDir = u""
    elif solvertype == "elmer":
        solver_obj = ObjectsFem.makeSolverElmer(doc, "SolverElmer")
        ObjectsFem.makeEquationElasticity(doc, solver_obj)
    else:
        FreeCAD.Console.PrintWarning(
            "Unknown or unsupported solver type: {}. "
            "No solver object was created.\n".format(solvertype)
        )
    if solvertype == "calculix" or solvertype == "ccxtools":
        solver_obj.SplitInputWriter = False
        solver_obj.AnalysisType = "static"
        solver_obj.GeometricalNonlinearity = "linear"
        solver_obj.ThermoMechSteadyState = False
        solver_obj.MatrixSolverType = "default"
        solver_obj.IterationsControlParameterTimeUse = False
    analysis.addObject(solver_obj)

    # constraint fixed
    con_fixed = ObjectsFem.makeConstraintFixed(doc, "FemConstraintFixed")
    con_fixed.References = [(geom_obj, "Face1")]
    analysis.addObject(con_fixed)

    # constraint force
    con_force = ObjectsFem.makeConstraintForce(doc, "FemConstraintForce")
    con_force.References = [(geom_obj, "Face6")]
    con_force.Force = "40000.0 N"
    con_force.Direction = (geom_obj, ["Edge5"])
    con_force.Reversed = True
    analysis.addObject(con_force)

    # constraint pressure
    con_pressure = ObjectsFem.makeConstraintPressure(doc, name="FemConstraintPressure")
    con_pressure.References = [(geom_obj, "Face2")]
    con_pressure.Pressure = "1000.0 MPa"
    con_pressure.Reversed = False
    analysis.addObject(con_pressure)

    doc.recompute()
    return doc
