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


# to run the examples copy the code:
"""
from femexamples.manager import *
run_all()

from femexamples.manager import *
doc = run_boxanalysisstatic()
doc = run_boxanalysisfrequency()
doc = run_ccx_cantileverfaceload()
doc = run_ccx_cantilevernodeload()
doc = run_ccx_cantileverprescribeddisplacement()
doc = setup_cantileverhexa20faceload()
doc = run_constraint_contact_shell_shell()
doc = run_constraint_contact_solid_solid()
doc = run_constraint_tie()
doc = run_material_nl_platewithhole()
doc = run_material_multiple_twoboxes()
doc = run_rcwall2d()
doc = run_thermomech_bimetall()
doc = run_thermomech_flow1d()
doc = run_thermomech_spine()

doc = run_ccx_cantilevernodeload("calculix")
doc = run_ccx_cantilevernodeload("ccxtools")
doc = run_ccx_cantilevernodeload("z88")

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
    if filepath is "":
        filepath = join(gettmp(), "FEM_examples")
    if not exists(filepath):
        makedirs(filepath)

    # find solver
    # ATM we only support one solver, search for a frame work solver and run it
    for m in doc.Analysis.Group:
        from femtools.femutils import is_derived_from
        if (
            is_derived_from(m, "Fem::FemSolverObjectPython")
            and m.Proxy.Type is not "Fem::FemSolverCalculixCcxTools"
        ):
            solver = m
            break

    # we need a file name for the besides dir to work
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


def run_boxanalysisstatic(solver=None, base_name=None):

    from .boxanalysis import setup_static as setup
    doc = setup()

    if base_name is None:
        base_name = "Box_Static_Analysis"
        if solver is not None:
            base_name += "_" + solver
    run_analysis(doc, base_name)
    doc.recompute()

    return doc


def run_boxanalysisfrequency(solver=None, base_name=None):

    from .boxanalysis import setup_frequency as setup
    doc = setup()

    if base_name is None:
        base_name = "Box_Frequency_Analysis"
        if solver is not None:
            base_name += "_" + solver
    run_analysis(doc, base_name)
    doc.recompute()

    return doc


def run_ccx_cantileverfaceload(solver=None, base_name=None):

    from .ccx_cantilever_std import setup_cantileverfaceload as setup
    doc = setup()

    if base_name is None:
        base_name = "CantilverFaceLoad"
        if solver is not None:
            base_name += "_" + solver
    run_analysis(doc, base_name)
    doc.recompute()

    return doc


def run_ccx_cantilevernodeload(solver=None, base_name=None):

    from .ccx_cantilever_std import setup_cantilevernodeload as setup
    doc = setup()

    if base_name is None:
        base_name = "CantileverNodeLoad"
        if solver is not None:
            base_name += "_" + solver
    run_analysis(doc, base_name)
    doc.recompute()

    return doc


def run_ccx_cantileverprescribeddisplacement(solver=None, base_name=None):

    from .ccx_cantilever_std import setup_cantileverprescribeddisplacement as setup
    doc = setup()

    if base_name is None:
        base_name = "CantileverPrescribedDisplacement"
        if solver is not None:
            base_name += "_" + solver
    run_analysis(doc, base_name)
    doc.recompute()

    return doc


def setup_cantileverhexa20faceload(solver=None, base_name=None):

    from .ccx_cantilever_std import setup_cantileverhexa20faceload as setup
    doc = setup()

    if base_name is None:
        base_name = "CantilverHexa20FaceLoad"
        if solver is not None:
            base_name += "_" + solver
    run_analysis(doc, base_name)
    doc.recompute()

    return doc


def run_constraint_contact_shell_shell(solver=None, base_name=None):

    from .constraint_contact_shell_shell import setup
    doc = setup()

    if base_name is None:
        base_name = "Constraint_Contact_Shell_Shell"
        if solver is not None:
            base_name += "_" + solver
    run_analysis(doc, base_name)
    doc.recompute()

    return doc


def run_constraint_contact_solid_solid(solver=None, base_name=None):

    from .constraint_contact_solid_solid import setup
    doc = setup()

    if base_name is None:
        base_name = "Constraint_Contact_Solid_Solid"
        if solver is not None:
            base_name += "_" + solver
    run_analysis(doc, base_name)
    doc.recompute()

    return doc


def run_constraint_tie(solver=None, base_name=None):

    from .constraint_tie import setup
    doc = setup()

    if base_name is None:
        base_name = "Constraint_Tie"
        if solver is not None:
            base_name += "_" + solver
    run_analysis(doc, base_name)
    doc.recompute()

    return doc


def run_material_multiple_twoboxes(solver=None, base_name=None):

    from .material_multiple_twoboxes import setup
    doc = setup()

    if base_name is None:
        base_name = "Multimaterial_Two-Boxes"
        if solver is not None:
            base_name += "_" + solver
    run_analysis(doc, base_name)
    doc.recompute()

    return doc


def run_material_nl_platewithhole(solver=None, base_name=None):

    from .material_nl_platewithhole import setup
    doc = setup()

    if base_name is None:
        base_name = "Nonlinear_material_plate_with_hole"
        if solver is not None:
            base_name += "_" + solver
    run_analysis(doc, base_name)
    doc.recompute()

    return doc


def run_rcwall2d(solver=None, base_name=None):

    from .rc_wall_2d import setup
    doc = setup()

    if base_name is None:
        base_name = "RC_FIB_Wall_2D"
        if solver is not None:
            base_name += "_" + solver
    run_analysis(doc, base_name)
    doc.recompute()

    return doc


def run_thermomech_bimetall(solver=None, base_name=None):

    from .thermomech_bimetall import setup
    doc = setup()

    if base_name is None:
        base_name = "Thermomech_Bimetall"
        if solver is not None:
            base_name += "_" + solver
    run_analysis(doc, base_name)
    doc.recompute()

    return doc


def run_thermomech_flow1d(solver=None, base_name=None):

    from .thermomech_flow1d import setup
    doc = setup()

    if base_name is None:
        base_name = "Thermomech_Spine"
        if solver is not None:
            base_name += "_" + solver
    run_analysis(doc, base_name)
    doc.recompute()

    return doc


def run_thermomech_spine(solver=None, base_name=None):

    from .thermomech_spine import setup
    doc = setup()

    if base_name is None:
        base_name = "Thermomech_Spine"
        if solver is not None:
            base_name += "_" + solver
    run_analysis(doc, base_name)
    doc.recompute()

    return doc


def run_all():
    run_boxanalysisstatic()
    run_boxanalysisfrequency()
    run_ccx_cantileverfaceload()
    run_ccx_cantilevernodeload()
    run_ccx_cantileverprescribeddisplacement()
    run_constraint_contact_shell_shell()
    run_constraint_contact_solid_solid()
    run_material_nl_platewithhole()
    run_material_multiple_twoboxes()
    run_rcwall2d()
    run_thermomech_bimetall()
    run_thermomech_flow1d()
    run_thermomech_spine()
