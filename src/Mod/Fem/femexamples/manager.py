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


def run_analysis(doc, base_name, filepath=''):

    from os.path import join, exists
    from os import makedirs
    from tempfile import gettempdir as gettmp

    # recompute
    doc.recompute()

    # print(doc.Objects)
    # print([obj.Name for obj in doc.Objects])

    # filepath
    if filepath is '':
        filepath = join(gettmp(), 'FEM_examples')
    if not exists(filepath):
        makedirs(filepath)

    # find solver
    # ATM we only support one solver, search for a frame work solver and run it
    for m in doc.Analysis.Group:
        from femtools.femutils import is_derived_from
        if is_derived_from(m, "Fem::FemSolverObjectPython") \
                and m.Proxy.Type is not 'Fem::FemSolverCalculixCcxTools':
            solver = m
            break

    # we need a file name for the besides dir to work
    save_fc_file = join(filepath, (base_name + '.FCStd'))
    FreeCAD.Console.PrintMessage(
        'Save FreeCAD file for {} analysis to {}\n.'.format(base_name, save_fc_file)
    )
    doc.saveAs(save_fc_file)

    # get analysis workig dir
    from femsolver.run import _getBesideDir as getpath
    working_dir = getpath(solver)

    # run analysis
    from femsolver.run import run_fem_solver
    run_fem_solver(solver, working_dir)

    # save doc once again with results
    doc.save()


def run_all():
    run_ccx_cantileverfaceload()
    run_ccx_cantilevernodeload()
    run_ccx_cantileverprescribeddisplacement()
    run_rcwall2d()


def run_ccx_cantileverfaceload(solver=None, base_name=None):

    from femexamples.ccx_cantilever_std import setup_cantileverfaceload as setup
    doc = setup()

    if base_name is None:
        base_name = 'CantilverFaceLoad'
        if solver is not None:
            base_name += ('_' + solver)
    run_analysis(doc, base_name)

    return doc


def run_ccx_cantilevernodeload(solver=None, base_name=None):

    from femexamples.ccx_cantilever_std import setup_cantilevernodeload as setup
    doc = setup()

    if base_name is None:
        base_name = 'CantileverNodeLoad'
        if solver is not None:
            base_name += ('_' + solver)
    run_analysis(doc, base_name)

    return doc


def run_ccx_cantileverprescribeddisplacement(solver=None, base_name=None):

    from femexamples.ccx_cantilever_std import setup_cantileverprescribeddisplacement as setup
    doc = setup()

    if base_name is None:
        base_name = 'CantileverPrescribedDisplacement'
        if solver is not None:
            base_name += ('_' + solver)
    run_analysis(doc, base_name)

    return doc


def run_rcwall2d(solver=None, base_name=None):

    from femexamples.rc_wall_2d import setup_rcwall2d as setup
    doc = setup()

    if base_name is None:
        base_name = 'RC_FIB_Wall_2D'
        if solver is not None:
            base_name += ('_' + solver)
    run_analysis(doc, base_name)

    return doc


'''
from femexamples.manager import *

run_all()

doc = run_ccx_cantileverfaceload()
doc = run_ccx_cantilevernodeload()
doc = run_ccx_cantileverprescribeddisplacement()

doc = run_ccx_cantilevernodeload('calculix')
doc = run_ccx_cantilevernodeload('ccxtools')
doc = run_ccx_cantilevernodeload('z88')

doc = run_rcwall2d()

'''
