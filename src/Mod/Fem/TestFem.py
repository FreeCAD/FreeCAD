# ***************************************************************************
# *   Copyright (c) 2018 Przemo Firszt <przemo@firszt.eu>                   *
# *   Copyright (c) 2018 Bernd Hahnebach <bernd@bimstatik.org>              *
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


# Unit test for the FEM module
# to get the right order import as is used
from femtest.app.test_femimport import TestFemImport as FemTest01
from femtest.app.test_common import TestFemCommon as FemTest02
from femtest.app.test_object import TestObjectCreate as FemTest03
from femtest.app.test_object import TestObjectType as FemTest04
from femtest.app.test_material import TestMaterialUnits as FemTest05
from femtest.app.test_mesh import TestMeshCommon as FemTest06
from femtest.app.test_mesh import TestMeshEleTetra10 as FemTest07
from femtest.app.test_result import TestResult as FemTest08
from femtest.app.test_ccxtools import TestCcxTools as FemTest09
from femtest.app.test_solverframework import TestSolverFrameWork as FemTest10

# dummy usage to get flake8 and lgtm quiet
False if FemTest01.__name__ else True
False if FemTest02.__name__ else True
False if FemTest03.__name__ else True
False if FemTest04.__name__ else True
False if FemTest05.__name__ else True
False if FemTest06.__name__ else True
False if FemTest07.__name__ else True
False if FemTest08.__name__ else True
False if FemTest09.__name__ else True
False if FemTest10.__name__ else True


# For more information on how to run a specific test class or a test method see
# file src/Mod/Test/__init__
# forum https://forum.freecadweb.org/viewtopic.php?f=10&t=22190#p175546

# It may be useful to temporary comment FreeCAD.closeDocument(self.doc_name)
# in tearDown method to not close the document


"""
# examples from within FreeCAD:
# all FEM tests
import Test, TestFem
Test.runTestsFromModule(TestFem)

# module
import Test, femtest.app.test_common
Test.runTestsFromModule(femtest.app.test_common)

# class
import Test, femtest.app.test_common
Test.runTestsFromClass(femtest.app.test_common.TestFemCommon)

# method
import unittest
thetest = "femtest.app.test_common.TestFemCommon.test_pyimport_all_FEM_modules"
alltest = unittest.TestLoader().loadTestsFromName(thetest)
unittest.TextTestRunner().run(alltest)


# examples from shell in build dir:
# all FreeCAD tests
./bin/FreeCAD --run-test 0
./bin/FreeCADCmd --run-test 0

# all FEM tests
./bin/FreeCAD --run-test "TestFem"
./bin/FreeCADCmd --run-test "TestFem"

# import Fem and FemGui
./bin/FreeCAD --run-test "femtest.app.test_femimport"
./bin/FreeCADCmd --run-test "femtest.app.test_femimport"

# other module
./bin/FreeCAD --run-test "femtest.app.test_femimport"
./bin/FreeCAD --run-test "femtest.app.test_ccxtools"
./bin/FreeCAD --run-test "femtest.app.test_common"
./bin/FreeCAD --run-test "femtest.app.test_material"
./bin/FreeCAD --run-test "femtest.app.test_mesh"
./bin/FreeCAD --run-test "femtest.app.test_object"
./bin/FreeCAD --run-test "femtest.app.test_result"
./bin/FreeCAD --run-test "femtest.app.test_solverframework"
./bin/FreeCADCmd --run-test "femtest.app.test_femimport"
./bin/FreeCADCmd --run-test "femtest.app.test_ccxtools"
./bin/FreeCADCmd --run-test "femtest.app.test_common"
./bin/FreeCADCmd --run-test "femtest.app.test_material"
./bin/FreeCADCmd --run-test "femtest.app.test_mesh"
./bin/FreeCADCmd --run-test "femtest.app.test_object"
./bin/FreeCADCmd --run-test "femtest.app.test_result"
./bin/FreeCADCmd --run-test "femtest.app.test_solverframework"

# class
./bin/FreeCAD --run-test "femtest.app.test_common.TestFemCommon"

# method
./bin/FreeCAD --run-test "femtest.app.test_common.TestFemCommon.test_pyimport_all_FEM_modules"

# unit test command to run a specific FEM unit test to copy for fast tests :-)
# to get all commands to start FreeCAD from build dir on Linux
# and run FEM unit test this could be used:
from femtest.utilstest import get_fem_test_defs as gf
gf()

./bin/FreeCADCmd --run-test "femtest.app.test_femimport.TestObjectExistance.test_objects_existance"
./bin/FreeCADCmd --run-test "femtest.app.test_ccxtools.TestCcxTools.test_freq_analysis"
./bin/FreeCADCmd --run-test "femtest.app.test_ccxtools.TestCcxTools.test_static_analysis"
./bin/FreeCADCmd --run-test "femtest.app.test_ccxtools.TestCcxTools.test_static_constraint_force_faceload_hexa20"
./bin/FreeCADCmd --run-test "femtest.app.test_ccxtools.TestCcxTools.test_static_constraint_contact_shell_shell"
./bin/FreeCADCmd --run-test "femtest.app.test_ccxtools.TestCcxTools.test_static_constraint_contact_solid_solid"
./bin/FreeCADCmd --run-test "femtest.app.test_ccxtools.TestCcxTools.test_static_constraint_tie"
./bin/FreeCADCmd --run-test "femtest.app.test_ccxtools.TestCcxTools.test_static_material_multiple"
./bin/FreeCADCmd --run-test "femtest.app.test_ccxtools.TestCcxTools.test_static_material_nonlinar"
./bin/FreeCADCmd --run-test "femtest.app.test_ccxtools.TestCcxTools.test_thermomech_bimetall"
./bin/FreeCADCmd --run-test "femtest.app.test_ccxtools.TestCcxTools.test_thermomech_flow1D_analysis"
./bin/FreeCADCmd --run-test "femtest.app.test_ccxtools.TestCcxTools.test_thermomech_spine_analysis"
./bin/FreeCADCmd --run-test "femtest.app.test_common.TestFemCommon.test_adding_refshaps"
./bin/FreeCADCmd --run-test "femtest.app.test_common.TestFemCommon.test_pyimport_all_FEM_modules"
./bin/FreeCADCmd --run-test "femtest.app.test_material.TestMaterialUnits.test_known_quantity_units"
./bin/FreeCADCmd --run-test "femtest.app.test_material.TestMaterialUnits.test_material_card_quantities"
./bin/FreeCADCmd --run-test "femtest.app.test_mesh.TestMeshCommon.test_mesh_seg2_python"
./bin/FreeCADCmd --run-test "femtest.app.test_mesh.TestMeshCommon.test_mesh_seg3_python"
./bin/FreeCADCmd --run-test "femtest.app.test_mesh.TestMeshCommon.test_unv_save_load"
./bin/FreeCADCmd --run-test "femtest.app.test_mesh.TestMeshCommon.test_writeAbaqus_precision"
./bin/FreeCADCmd --run-test "femtest.app.test_mesh.TestMeshEleTetra10.test_tetra10_create"
./bin/FreeCADCmd --run-test "femtest.app.test_mesh.TestMeshEleTetra10.test_tetra10_inp"
./bin/FreeCADCmd --run-test "femtest.app.test_mesh.TestMeshEleTetra10.test_tetra10_unv"
./bin/FreeCADCmd --run-test "femtest.app.test_mesh.TestMeshEleTetra10.test_tetra10_vkt"
./bin/FreeCADCmd --run-test "femtest.app.test_mesh.TestMeshEleTetra10.test_tetra10_yml"
./bin/FreeCADCmd --run-test "femtest.app.test_mesh.TestMeshEleTetra10.test_tetra10_z88"
./bin/FreeCADCmd --run-test "femtest.app.test_object.TestObjectCreate.test_femobjects_make"
./bin/FreeCADCmd --run-test "femtest.app.test_object.TestObjectType.test_femobjects_type"
./bin/FreeCADCmd --run-test "femtest.app.test_object.TestObjectType.test_femobjects_isoftype"
./bin/FreeCADCmd --run-test "femtest.app.test_object.TestObjectType.test_femobjects_derivedfromfem"
./bin/FreeCADCmd --run-test "femtest.app.test_object.TestObjectType.test_femobjects_derivedfromstd"
./bin/FreeCADCmd --run-test "femtest.app.test_result.TestResult.test_read_frd_massflow_networkpressure"
./bin/FreeCADCmd --run-test "femtest.app.test_result.TestResult.test_stress_von_mises"
./bin/FreeCADCmd --run-test "femtest.app.test_result.TestResult.test_stress_principal_std"
./bin/FreeCADCmd --run-test "femtest.app.test_result.TestResult.test_stress_principal_reinforced"
./bin/FreeCADCmd --run-test "femtest.app.test_result.TestResult.test_rho"
./bin/FreeCADCmd --run-test "femtest.app.test_result.TestResult.test_disp_abs"
./bin/FreeCADCmd --run-test "femtest.app.test_solverframework.TestSolverFrameWork.test_solver_calculix"
./bin/FreeCADCmd --run-test "femtest.app.test_solverframework.TestSolverFrameWork.test_solver_elmer"


# to get all command to start FreeCAD from build dir on Linux
# and run FEM unit test this could be used:
from femtest.utilstest import get_fem_test_defs as gf
gf("in")

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_ccxtools.TestCcxTools.test_freq_analysis"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_ccxtools.TestCcxTools.test_static_analysis"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_ccxtools.TestCcxTools.test_static_constraint_force_faceload_hexa20"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_ccxtools.TestCcxTools.test_static_constraint_contact_shell_shell"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_ccxtools.TestCcxTools.test_static_constraint_contact_solid_solid"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_ccxtools.TestCcxTools.test_static_constraint_tie"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_ccxtools.TestCcxTools.test_static_material_multiple"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_ccxtools.TestCcxTools.test_static_material_nonlinar"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_ccxtools.TestCcxTools.test_thermomech_bimetall"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_ccxtools.TestCcxTools.test_thermomech_flow1D_analysis"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_ccxtools.TestCcxTools.test_thermomech_spine_analysis"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_common.TestFemCommon.test_adding_refshaps"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_common.TestFemCommon.test_pyimport_all_FEM_modules"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_material.TestMaterialUnits.test_known_quantity_units"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_material.TestMaterialUnits.test_material_card_quantities"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_mesh.TestMeshCommon.test_mesh_seg2_python"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_mesh.TestMeshCommon.test_mesh_seg3_python"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_mesh.TestMeshCommon.test_unv_save_load"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_mesh.TestMeshCommon.test_writeAbaqus_precision"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_mesh.TestMeshEleTetra10.test_tetra10_create"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_mesh.TestMeshEleTetra10.test_tetra10_inp"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_mesh.TestMeshEleTetra10.test_tetra10_unv"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_mesh.TestMeshEleTetra10.test_tetra10_vkt"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_mesh.TestMeshEleTetra10.test_tetra10_yml"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_mesh.TestMeshEleTetra10.test_tetra10_z88"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_object.TestObjectCreate.test_femobjects_make"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_object.TestObjectType.test_femobjects_type"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_object.TestObjectType.test_femobjects_isoftype"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_object.TestObjectType.test_femobjects_derivedfromfem"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_object.TestObjectType.test_femobjects_derivedfromstd"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_result.TestResult.test_read_frd_massflow_networkpressure"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_result.TestResult.test_stress_von_mises"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_result.TestResult.test_stress_principal_std"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_result.TestResult.test_stress_principal_reinforced"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_result.TestResult.test_rho"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_result.TestResult.test_disp_abs"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_solverframework.TestSolverFrameWork.test_solver_calculix"))

import unittest
unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName("femtest.app.test_solverframework.TestSolverFrameWork.test_solver_elmer"))


# open files from FEM test suite source code
# be careful on updating these files, they contain the original results!
# TODO update files, because some of them have non-existing FEM object classes
doc = FreeCAD.open(FreeCAD.ConfigGet("AppHomePath") + 'Mod/Fem/femtest/data/ccx/cube.FCStd')
doc = FreeCAD.open(FreeCAD.ConfigGet("AppHomePath") + 'Mod/Fem/femtest/data/ccx/cube_frequency.FCStd')
doc = FreeCAD.open(FreeCAD.ConfigGet("AppHomePath") + 'Mod/Fem/femtest/data/ccx/cube_static.FCStd')
doc = FreeCAD.open(FreeCAD.ConfigGet("AppHomePath") + 'Mod/Fem/femtest/data/ccx/Flow1D_thermomech.FCStd')
doc = FreeCAD.open(FreeCAD.ConfigGet("AppHomePath") + 'Mod/Fem/femtest/data/ccx/multimat.FCStd')
doc = FreeCAD.open(FreeCAD.ConfigGet("AppHomePath") + 'Mod/Fem/femtest/data/ccx/spine_thermomech.FCStd')

# open files generated from test suite
import femtest.utilstest as ut
ut.all_test_files()

doc = ut.cube_frequency()
doc = ut.cube_static()
doc = ut.Flow1D_thermomech()
doc = ut.multimat()
doc = ut.spine_thermomech()

# load std FEM example files
app_home = FreeCAD.ConfigGet("AppHomePath")
doc = FreeCAD.open(app_home + "data/examples/FemCalculixCantilever2D.FCStd")
doc = FreeCAD.open(app_home + "data/examples/FemCalculixCantilever3D.FCStd")
doc = FreeCAD.open(app_home + "data/examples/FemCalculixCantilever3D_newSolver.FCStd")
doc = FreeCAD.open(app_home + "data/examples/Fem.FCStd")
doc = FreeCAD.open(app_home + "data/examples/Fem2.FCStd")

"""
