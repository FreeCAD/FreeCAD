# ***************************************************************************
# *   Copyright (c) 2019 Bernd Hahnebach <bernd@bimstatik.org>              *
# *   Copyright (c) 2020 Sudhanshu Dubey <sudhanshu.thethunder@gmail.com>   *
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

# run examples
"""
# all examples
from femexamples.manager import *
run_all()


# one special example
from femexamples.manager import run_example as run

doc = run("boxanalysis_static")
doc = run("boxanalysis_frequency")

...
"""

import FreeCAD


def run_analysis(doc, base_name, filepath=""):

    from os.path import join, exists
    from os import makedirs
    from tempfile import gettempdir as gettmp

    # recompute
    doc.recompute()

    # print(doc.Objects)
    # print([obj.Name for obj in doc.Objects])

    # filepath
    if filepath == "":
        filepath = join(gettmp(), "FEM_examples")
    if not exists(filepath):
        makedirs(filepath)

    # find the first solver
    # thus ATM only one solver per analysis is supported
    from femtools.femutils import is_derived_from
    for m in doc.Analysis.Group:
        if is_derived_from(m, "Fem::FemSolverObjectPython"):
            solver = m
            break

    # a file name is needed for the besides dir to work
    save_fc_file = join(filepath, (base_name + ".FCStd"))
    FreeCAD.Console.PrintMessage(
        "Save FreeCAD file for {} analysis to {}\n.".format(base_name, save_fc_file)
    )
    doc.saveAs(save_fc_file)

    # get analysis workig dir
    from femtools.femutils import get_beside_dir
    working_dir = get_beside_dir(solver)

    # run analysis
    from femsolver.run import run_fem_solver
    run_fem_solver(solver, working_dir)

    # save doc once again with results
    doc.save()


def run_example(example, solver=None, base_name=None):

    from importlib import import_module
    module = import_module("femexamples." + example)
    if not hasattr(module, "setup"):
        FreeCAD.Console.PrintError("Setup method not found in {}\n".format(example))
        return None

    if solver is None:
        doc = getattr(module, "setup")()
    else:
        doc = getattr(module, "setup")(solvertype=solver)

    if base_name is None:
        base_name = example
        if solver is not None:
            base_name += "_" + solver
    run_analysis(doc, base_name)
    doc.recompute()

    return doc


def run_all():
    run_example("boxanalysis_static")
    run_example("boxanalysis_frequency")
    run_example("ccx_cantilever_faceload")
    run_example("ccx_cantilever_nodeload")
    run_example("ccx_cantilever_prescribeddisplacement")
    run_example("ccx_cantilever_hexa20faceload")
    run_example("constraint_contact_shell_shell")
    run_example("constraint_contact_solid_solid")
    run_example("constraint_tie")
    run_example("material_multiple_twoboxes")
    run_example("material_nl_platewithhole")
    run_example("rc_wall_2d")
    run_example("thermomech_bimetall")
    run_example("thermomech_flow1d")
    run_example("thermomech_spine")
    run_example("boxanalysis_frequency")
    run_example("boxanalysis_frequency")
