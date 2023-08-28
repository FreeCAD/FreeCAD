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

import FreeCAD


# ************************************************************************************************
# setup and run examples by Python

# TODO: use method from examples gui to collect all examples in run_all method
# FreeCAD Gui update between the examples would makes sense too

"""
# setup all examples
from femexamples.manager import *
setup_all()


# run all examples
from femexamples.manager import *
run_all()


# one special example
from femexamples.manager import run_example as run

doc = run("boxanalysis_static")
doc = run("boxanalysis_frequency")

"""


def run_all():
    run_example("boxanalysis_frequency", run_solver=True)
    run_example("boxanalysis_static", run_solver=True)
    run_example("buckling_lateraltorsionalbuckling", run_solver=True)
    run_example("buckling_platebuckling", run_solver=True)
    run_example("ccx_buckling_flexuralbuckling", run_solver=True)
    run_example("ccx_cantilever_faceload", run_solver=True)
    run_example("ccx_cantilever_hexa20faceload", run_solver=True)
    run_example("ccx_cantilever_nodeload", run_solver=True)
    run_example("ccx_cantilever_prescribeddisplacement", run_solver=True)
    run_example("constraint_contact_shell_shell", run_solver=True)
    run_example("constraint_contact_solid_solid", run_solver=True)
    run_example("constraint_section_print", run_solver=True)
    run_example("constraint_selfweight_cantilever", run_solver=True)
    run_example("constraint_tie", run_solver=True)
    run_example("constraint_transform_beam_hinged", run_solver=True)
    run_example("elmer_nonguitutorial01_eigenvalue_of_elastic_beam", run_solver=True)
    run_example("equation_deformation_spring_elmer", run_solver=True)
    run_example("equation_electrostatics_capacitance_two_balls", run_solver=True)
    run_example("equation_electrostatics_electricforce_elmer_nongui6", run_solver=True)
    run_example("equation_flow_elmer_2D", run_solver=True)
    run_example("equation_flow_initial_elmer_2D", run_solver=True)
    run_example("equation_flow_turbulent_elmer_2D", run_solver=True)
    run_example("equation_flux_elmer", run_solver=True)
    run_example("equation_magnetodynamics_elmer", run_solver=True)
    run_example("equation_magnetodynamics_2D_elmer.py", run_solver=True)
    run_example("equation_magnetostatics_2D_elmer.py", run_solver=True)
    run_example("frequency_beamsimple", run_solver=True)
    run_example("material_multiple_bendingbeam_fiveboxes", run_solver=True)
    run_example("material_multiple_bendingbeam_fivefaces", run_solver=True)
    run_example("material_multiple_tensionrod_twoboxes", run_solver=True)
    run_example("material_nl_platewithhole", run_solver=True)
    run_example("rc_wall_2d", run_solver=True)
    run_example("square_pipe_end_twisted_edgeforces", run_solver=True)
    run_example("square_pipe_end_twisted_nodeforces", run_solver=True)
    run_example("thermomech_bimetall", run_solver=True)


def setup_all():
    run_example("boxanalysis_frequency")
    run_example("boxanalysis_static")
    run_example("buckling_lateraltorsionalbuckling")
    run_example("buckling_platebuckling")
    run_example("ccx_buckling_flexuralbuckling")
    run_example("ccx_cantilever_faceload")
    run_example("ccx_cantilever_hexa20faceload")
    run_example("ccx_cantilever_nodeload")
    run_example("ccx_cantilever_prescribeddisplacement")
    run_example("constraint_contact_shell_shell")
    run_example("constraint_contact_solid_solid")
    run_example("constraint_section_print")
    run_example("constraint_selfweight_cantilever")
    run_example("constraint_tie")
    run_example("constraint_transform_beam_hinged")
    run_example("elmer_nonguitutorial01_eigenvalue_of_elastic_beam")
    run_example("equation_deformation_spring_elmer")
    run_example("equation_electrostatics_capacitance_two_balls")
    run_example("equation_electrostatics_electricforce_elmer_nongui6")
    run_example("equation_flow_elmer_2D")
    run_example("equation_flow_initial_elmer_2D")
    run_example("equation_flow_turbulent_elmer_2D")
    run_example("equation_flux_elmer")
    run_example("equation_magnetodynamics_elmer")
    run_example("equation_magnetodynamics_2D_elmer.py")
    run_example("equation_magnetostatics_2D_elmer.py")
    run_example("frequency_beamsimple")
    run_example("material_multiple_bendingbeam_fiveboxes")
    run_example("material_multiple_bendingbeam_fivefaces")
    run_example("material_multiple_tensionrod_twoboxes")
    run_example("material_nl_platewithhole")
    run_example("rc_wall_2d")
    run_example("square_pipe_end_twisted_edgeforces")
    run_example("square_pipe_end_twisted_nodeforces")
    run_example("thermomech_bimetall")


def run_analysis(doc, base_name, filepath="", run_solver=False):

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
    if run_solver is True:
        run_fem_solver(solver, working_dir)

    # save doc once again with results
    doc.save()


def run_example(example, solver=None, base_name=None, run_solver=False):

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
    run_analysis(doc, base_name, run_solver=run_solver)
    doc.recompute()

    return doc


# ************************************************************************************************
# helper used from examples
def init_doc(doc=None):
    if doc is None:
        doc = FreeCAD.newDocument()
    return doc


def get_meshname():
    # needs to be "Mesh" to work with unit tests
    return "Mesh"


def get_header(information):
    return """{name}

{information}""".format(name=information["name"], information=print_info_dict(information))


def print_info_dict(information):
    the_text = ""
    for k, v in information.items():
        value_text = ""
        if isinstance(v, list):
            for j in v:
                value_text += "{}, ".format(j)
            value_text = value_text.rstrip(", ")
        else:
            value_text = v
        the_text += "{} --> {}\n".format(k, value_text)
    # print(the_text)
    return the_text


def add_explanation_obj(doc, the_text):
    text_obj = doc.addObject("App::TextDocument", "Explanation_Report")
    text_obj.Text = the_text
    text_obj.setPropertyStatus("Text", "ReadOnly")  # set property editor readonly
    if FreeCAD.GuiUp:
        text_obj.ViewObject.ReadOnly = True  # set editor view readonly
    return text_obj
