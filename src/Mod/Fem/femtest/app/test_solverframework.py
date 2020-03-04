# ***************************************************************************
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
# ***************************************************************************/

import FreeCAD
import ObjectsFem
import femsolver.run
import unittest
from . import support_utils as testtools
from .support_utils import fcc_print

from os.path import join


class TestSolverFrameWork(unittest.TestCase):
    fcc_print("import TestSolverFrameWork")

    # ********************************************************************************************
    def setUp(
        self
    ):
        # setUp is executed before every test
        # setting up a document to hold the tests
        self.doc_name = self.__class__.__name__
        if FreeCAD.ActiveDocument:
            if FreeCAD.ActiveDocument.Name != self.doc_name:
                FreeCAD.newDocument(self.doc_name)
        else:
            FreeCAD.newDocument(self.doc_name)
        FreeCAD.setActiveDocument(self.doc_name)
        self.active_doc = FreeCAD.ActiveDocument

        # more inits
        self.mesh_name = "Mesh"
        self.temp_dir = testtools.get_unit_test_tmp_dir(
            testtools.get_fem_test_tmp_dir(),
            "FEM_solverframework"
        )

    def test_00print(
        self
    ):
        fcc_print("\n{0}\n{1} run FEM TestSolverFrameWork tests {2}\n{0}".format(
            100 * "*",
            10 * "*",
            55 * "*"
        ))

    # ********************************************************************************************
    def test_solver_calculix(
        self
    ):
        fcc_print("\n--------------- Start of FEM tests solver framework solver CalculiX ------")

        # set up the CalculiX static analysis example
        from femexamples import boxanalysis as box
        box.setup_static(self.active_doc, "calculix")

        solver_obj = self.active_doc.SolverCalculiX

        base_name = "cube_static"
        analysis_dir = testtools.get_unit_test_tmp_dir(self.temp_dir, solver_obj.Name)

        # save the file
        save_fc_file = join(analysis_dir, solver_obj.Name + "_" + base_name + ".FCStd")
        fcc_print("Save FreeCAD file to {}...".format(save_fc_file))
        self.active_doc.saveAs(save_fc_file)

        # write input file
        fcc_print("Checking FEM input file writing for CalculiX solver framework solver ...")
        machine_ccx = solver_obj.Proxy.createMachine(
            solver_obj,
            analysis_dir
        )
        machine_ccx.target = femsolver.run.PREPARE
        machine_ccx.start()
        machine_ccx.join()  # wait for the machine to finish.

        infile_given = join(
            testtools.get_fem_test_home_dir(),
            "ccx",
            (base_name + ".inp")
        )
        inpfile_totest = join(analysis_dir, (self.mesh_name + ".inp"))
        fcc_print("Comparing {} to {}".format(infile_given, inpfile_totest))
        ret = testtools.compare_inp_files(infile_given, inpfile_totest)
        self.assertFalse(ret, "ccxtools write_inp_file test failed.\n{}".format(ret))

        fcc_print("--------------- End of FEM tests solver framework solver CalculiX --------")

    # ********************************************************************************************
    def test_solver_elmer(
        self
    ):
        fcc_print("\n--------------- Start of FEM tests solver framework solver Elmer ---------")

        # set up the Elmer static analysis example
        from femexamples import boxanalysis as box
        box.setup_static(self.active_doc, "elmer")

        analysis_obj = self.active_doc.Analysis
        solver_obj = self.active_doc.SolverElmer
        material_obj = self.active_doc.MechanicalMaterial
        mesh_obj = self.active_doc.Mesh
        box_object = self.active_doc.Box

        base_name = "cube_static"
        analysis_dir = testtools.get_unit_test_tmp_dir(self.temp_dir, solver_obj.Name)

        # TODO move to elmer solver of femexample code
        ObjectsFem.makeEquationElasticity(self.active_doc, solver_obj)

        # set ThermalExpansionCoefficient
        # FIXME elmer elasticity needs the dictionary key "ThermalExpansionCoefficient"
        # even on simple elasticity analysis, otherwise it fails
        mat = material_obj.Material
        mat["ThermalExpansionCoefficient"] = "0 um/m/K"
        material_obj.Material = mat

        # elmer needs a GMHS mesh object
        # FIXME error message on Python solver run
        mesh_gmsh = ObjectsFem.makeMeshGmsh(self.active_doc)
        mesh_gmsh.CharacteristicLengthMin = "9 mm"
        mesh_gmsh.FemMesh = mesh_obj.FemMesh
        mesh_gmsh.Part = box_object
        analysis_obj.addObject(mesh_gmsh)
        self.active_doc.removeObject(mesh_obj.Name)  # remove original mesh object

        # save the file
        save_fc_file = join(analysis_dir, solver_obj.Name + "_" + base_name + ".FCStd")
        fcc_print("Save FreeCAD file to {}...".format(save_fc_file))
        self.active_doc.saveAs(save_fc_file)

        # write input files
        fcc_print("Checking FEM input file writing for Elmer solver framework solver ...")
        machine_elmer = solver_obj.Proxy.createMachine(
            solver_obj,
            analysis_dir,
            True
        )
        machine_elmer.target = femsolver.run.PREPARE
        machine_elmer.start()
        machine_elmer.join()  # wait for the machine to finish.

        # compare startinfo, case and gmsh input files
        test_file_dir_elmer = join(testtools.get_fem_test_home_dir(), "elmer")
        fcc_print(test_file_dir_elmer)

        fcc_print("Test writing STARTINFO file")
        startinfo_given = join(test_file_dir_elmer, "ELMERSOLVER_STARTINFO")
        startinfo_totest = join(analysis_dir, "ELMERSOLVER_STARTINFO")
        fcc_print("Comparing {} to {}".format(startinfo_given, startinfo_totest))
        ret = testtools.compare_files(startinfo_given, startinfo_totest)
        self.assertFalse(ret, "STARTINFO write file test failed.\n{}".format(ret))

        fcc_print("Test writing case file")
        casefile_given = join(test_file_dir_elmer, "case.sif")
        casefile_totest = join(analysis_dir, "case.sif")
        fcc_print("Comparing {} to {}".format(casefile_given, casefile_totest))
        ret = testtools.compare_files(casefile_given, casefile_totest)
        self.assertFalse(ret, "case write file test failed.\n{}".format(ret))

        fcc_print("Test writing GMSH geo file")
        gmshgeofile_given = join(test_file_dir_elmer, "group_mesh.geo")
        gmshgeofile_totest = join(analysis_dir, "group_mesh.geo")
        fcc_print("Comparing {} to {}".format(gmshgeofile_given, gmshgeofile_totest))
        ret = testtools.compare_files(gmshgeofile_given, gmshgeofile_totest)
        self.assertFalse(ret, "GMSH geo write file test failed.\n{}".format(ret))

        fcc_print("--------------- End of FEM tests solver framework solver Elmer -----------")

    # ********************************************************************************************
    def tearDown(
        self
    ):
        # clearance, is executed after every test
        FreeCAD.closeDocument(self.doc_name)
