# SPDX-License-Identifier: LGPL-2.1-or-later

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

__title__ = "Solver elmer FEM unit tests"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

import unittest
from os.path import join

import FreeCAD

from femsolver.elmer import writer
from femtools import materialtables
from . import support_utils as testtools
from .support_utils import fcc_print
from .support_utils import get_namefromdef


class TestSolverElmer(unittest.TestCase):
    fcc_print("import TestSolverElmer")

    # ********************************************************************************************
    def setUp(self):
        # setUp is executed before every test

        # new document
        self.document = FreeCAD.newDocument(self.__class__.__name__)

        # more inits
        self.pre_dir_name = "solver_elmer_"
        self.ending = ".sif"
        self.infilename = "case"
        self.test_file_dir = join(testtools.get_fem_test_home_dir(), "elmer")
        # set Units
        # since in Elmer writer the FreeCAD pref is used, here we need to set the FreeCAD pref
        # the use of FreeCAD.Units.setScheme would not take affect because the pref is not changed
        # see https://forum.freecad.org/viewtopic.php?t=48451
        param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units")
        self.saved_unit_schema = param.GetInt("UserSchema")

    # ********************************************************************************************
    def tearDown(self):
        # set back unit unit schema
        param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units")
        unit_schema = param.GetInt("UserSchema")
        if unit_schema != self.saved_unit_schema:
            fcc_print(f"Reset unit schema back to {self.saved_unit_schema}")
            param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units")
            param.SetInt("UserSchema", self.saved_unit_schema)

        # tearDown is executed after every test
        FreeCAD.closeDocument(self.document.Name)
        fcc_print("")

    # ********************************************************************************************
    def test_00print(self):
        # since method name starts with 00 this will be run first
        # this test just prints a line with stars

        fcc_print(
            "\n{0}\n{1} run FEM TestSolverElmer tests {2}\n{0}".format(
                100 * "*", 10 * "*", 55 * "*"
            )
        )

    # ********************************************************************************************
    def set_unit_schema(self, new_unit_schema=0):
        fcc_print(
            "\nSaved unit schema: {}. Set unit schema to {}.".format(
                self.saved_unit_schema, new_unit_schema
            )
        )
        param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units")
        param.SetInt("UserSchema", new_unit_schema)

    # ********************************************************************************************
    def test_box_static_0_mm(self):
        fcc_print("")
        self.set_unit_schema(0)  # mm/kg/s

        # set up the Elmer static analysis example
        from femexamples.boxanalysis_static import setup

        setup(self.document, "elmer", test_mode=True)

        # for information:
        # elmer needs gmsh mesho object
        # FIXME error message on Python solver run
        # the examples do use a gmsh mesh object thus ok

        base_name = get_namefromdef("test_")
        analysis_dir = testtools.get_fem_test_tmp_dir(self.pre_dir_name + base_name)

        # save the file
        save_fc_file = join(analysis_dir, base_name + ".FCStd")
        # fcc_print("Save FreeCAD file to {}...".format(save_fc_file))
        self.document.saveAs(save_fc_file)

        # write input files
        # fcc_print("Checking FEM input file writing for Elmer solver framework solver ...")
        w = writer.Writer(self.document.SolverElmer, analysis_dir, testmode=True)
        w.write_solver_input()

        fcc_print("Test writing STARTINFO file")
        startinfo_given = join(self.test_file_dir, "ELMERSOLVER_STARTINFO")
        startinfo_totest = join(analysis_dir, "ELMERSOLVER_STARTINFO")
        # fcc_print("Comparing {} to {}".format(startinfo_given, startinfo_totest))
        ret = testtools.compare_files(startinfo_given, startinfo_totest)
        self.assertFalse(ret, f"STARTINFO write file test failed.\n{ret}")

        fcc_print("Test writing case file")
        casefile_given = join(self.test_file_dir, base_name + self.ending)
        casefile_totest = join(analysis_dir, self.infilename + self.ending)
        # fcc_print("Comparing {} to {}".format(casefile_given, casefile_totest))
        ret = testtools.compare_files(casefile_given, casefile_totest)
        self.assertFalse(ret, f"case write file test failed.\n{ret}")

    # ********************************************************************************************
    def test_ccxcantilever_faceload_0_mm(self):
        fcc_print("")
        self.set_unit_schema(0)  # mm/kg/s
        from femexamples.ccx_cantilever_faceload import setup

        setup(self.document, "elmer", test_mode=True)
        self.input_file_writing_test(get_namefromdef("test_"))

    # ********************************************************************************************
    def test_ccxcantilever_faceload_1_si(self):
        fcc_print("")
        self.set_unit_schema(1)  # SI-units m/kg/s
        from femexamples.ccx_cantilever_faceload import setup

        setup(self.document, "elmer", test_mode=True)
        self.input_file_writing_test(get_namefromdef("test_"))

    # ********************************************************************************************
    def test_ccxcantilever_nodeload_0_mm(self):
        fcc_print("")
        self.set_unit_schema(0)  # mm/kg/s
        from femexamples.ccx_cantilever_nodeload import setup

        setup(self.document, "elmer", test_mode=True)
        self.input_file_writing_test(get_namefromdef("test_"))

    # ********************************************************************************************
    def test_ccxcantilever_prescribeddisplacement_0_mm(self):
        fcc_print("")
        self.set_unit_schema(0)  # mm/kg/s
        from femexamples.ccx_cantilever_prescribeddisplacement import setup

        setup(self.document, "elmer", test_mode=True)
        self.input_file_writing_test(get_namefromdef("test_"))

    # ********************************************************************************************
    def test_ccxcantilever_temperature_tables_0_mm(self):
        fcc_print("")
        self.set_unit_schema(0)  # mm/kg/s
        import Materials
        import ObjectsFem
        from femexamples.ccx_cantilever_faceload import setup

        setup(self.document, "elmer", test_mode=True)
        ObjectsFem.makeEquationHeat(self.document, self.document.SolverElmer)
        material = self.document.FemMaterial
        card_values = dict(material.Material)
        card_values["Density"] = "8960 kg/m^3"
        material.Material = card_values
        # The card of the CalculiX test, with empty cells and a falling temperature
        thermal_card = Materials.MaterialManager().getMaterial(
            "12cc83fb-d1d4-460e-a8fc-5236afef6ad0"
        )
        for table in ("ConductivityTable", "SpecificHeatTable"):
            setattr(material, table, materialtables.rows_from_material(thermal_card, table))
        written = self.input_file_writing_test(get_namefromdef("test_"))
        # compare_inp_files misses a deleted line, each table is checked whole
        self.assert_table_written(
            written, "Heat Capacity", ["0 385", "300 385", "400 397", "1000000 397"]
        )
        self.assert_table_written(
            written,
            "Heat Conductivity",
            ["0 413", "200 413", "300 401", "400 393", "1000000 393"],
        )

    # ********************************************************************************************
    def test_ccxcantilever_electrical_table_0_mm(self):
        fcc_print("")
        self.set_unit_schema(0)  # mm/kg/s
        import Materials
        import ObjectsFem
        from femexamples.ccx_cantilever_faceload import setup

        setup(self.document, "elmer", test_mode=True)
        # The table reads the temperature of the heat solver, created first it runs first
        ObjectsFem.makeEquationHeat(self.document, self.document.SolverElmer)
        ObjectsFem.makeEquationStaticCurrent(self.document, self.document.SolverElmer)
        material = self.document.FemMaterial
        card_values = dict(material.Material)
        card_values["Density"] = "8960 kg/m^3"
        card_values["ThermalConductivity"] = "401 W/m/K"
        card_values["SpecificHeat"] = "385 J/kg/K"
        material.Material = card_values
        # Empty cell, falling temperature
        electrical_card = Materials.MaterialManager().getMaterial(
            "bf4cce7d-9e0b-4687-bd7a-69d5385c1f12"
        )
        self.document.FemMaterial.ElectricalConductivityTable = materialtables.rows_from_material(
            electrical_card, "ElectricalConductivityTable"
        )
        written = self.input_file_writing_test(get_namefromdef("test_"))
        self.assert_table_written(
            written,
            "Electric Conductivity",
            ["0 60000", "900 60000", "1500 70000", "1000000 70000"],
        )

    def assert_table_written(self, path, keyword, rows):
        with open(path, encoding="utf-8") as fh:
            text = fh.read()
        block = "".join(f"    {row}\n" for row in rows)
        self.assertIn(f"  {keyword} = Variable Temperature\n  Real\n{block}  End\n", text)

    # ********************************************************************************************
    def test_flow_keeps_conductivity_table(self):
        fcc_print("")
        import ObjectsFem
        from femexamples.ccx_cantilever_faceload import setup

        # Flow writes the material after Heat and used to put the scalar back
        setup(self.document, "elmer", test_mode=True)
        ObjectsFem.makeEquationHeat(self.document, self.document.SolverElmer)
        ObjectsFem.makeEquationFlow(self.document, self.document.SolverElmer)
        material = self.document.FemMaterial
        card_values = dict(material.Material)
        card_values["Density"] = "1000 kg/m^3"
        card_values["DynamicViscosity"] = "0.001 Pa*s"
        card_values["ThermalConductivity"] = "0.6 W/m/K"
        card_values["SpecificHeat"] = "4180 J/kg/K"
        material.Material = card_values
        material.ConductivityTable = ["300 K, 0.5 W/m/K", "400 K, 0.7 W/m/K"]
        self.document.recompute()
        working_dir = testtools.get_fem_test_tmp_dir(self.pre_dir_name + "flow_keeps_table")
        writer.Writer(self.document.SolverElmer, working_dir).write_solver_input()
        self.assert_table_written(
            join(working_dir, self.infilename + self.ending),
            "Heat Conductivity",
            ["0 0.5", "300 0.5", "400 0.7", "1000000 0.7"],
        )

    # ********************************************************************************************
    def test_magnetodynamic_reads_electrical_table(self):
        import ObjectsFem

        self.magnetic_table_case(ObjectsFem.makeEquationMagnetodynamic, "magneto_table")

    def test_magnetodynamic2D_reads_electrical_table(self):
        import ObjectsFem

        self.magnetic_table_case(ObjectsFem.makeEquationMagnetodynamic2D, "magneto2D_table")

    def test_magnetic_post_solver_order(self):
        import ObjectsFem

        # The 3D post solver runs Before Saving, after the solver loop and Heat
        notes = self.magnetic_table_case(
            ObjectsFem.makeEquationMagnetodynamic, "magneto_post", heat_priority=20
        )
        self.assertEqual([note.split(" before")[0] for note in notes], ["MgDyn runs"])

    def test_magnetic2D_post_solver_order(self):
        import ObjectsFem

        # The 2D post solver runs Always, inside the loop, before Heat here
        notes = self.magnetic_table_case(
            ObjectsFem.makeEquationMagnetodynamic2D, "magneto2D_post", heat_priority=20
        )
        self.assertEqual(
            [note.split(" before")[0] for note in notes], ["MgDyn2D runs", "MgDyn2DPost runs"]
        )

    def magnetic_table_case(self, make_equation, dir_name, heat_priority=None):
        fcc_print("")
        import ObjectsFem
        from femexamples.ccx_cantilever_faceload import setup

        # No scalar conductivity on the card, the magnetic writers used to require one
        setup(self.document, "elmer", test_mode=True)
        heat = ObjectsFem.makeEquationHeat(self.document, self.document.SolverElmer)
        magnetic = make_equation(self.document, self.document.SolverElmer)
        if heat_priority is not None:
            heat.Priority, magnetic.Priority = heat_priority, 50
        material = self.document.FemMaterial
        card_values = dict(material.Material)
        card_values.pop("ElectricalConductivity", None)
        card_values["RelativePermeability"] = "1"
        card_values["Density"] = "8960 kg/m^3"
        card_values["ThermalConductivity"] = "401 W/m/K"
        card_values["SpecificHeat"] = "385 J/kg/K"
        material.Material = card_values
        material.ElectricalConductivityTable = ["300 K, 100 S/m", "400 K, 200 S/m"]
        # The magnetic writers want every body referenced by a material
        material.References = [(self.document.Box, "Solid1")]
        self.document.recompute()
        working_dir = testtools.get_fem_test_tmp_dir(self.pre_dir_name + dir_name)
        w = writer.Writer(self.document.SolverElmer, working_dir)
        w.write_solver_input()
        self.assert_table_written(
            join(working_dir, self.infilename + self.ending),
            "Electric Conductivity",
            ["0 100", "300 100", "400 200", "1000000 200"],
        )
        return w.solverOrderNotes()

    # ********************************************************************************************
    def test_refused_write_announces_no_range(self):
        fcc_print("")
        import ObjectsFem
        from femexamples.ccx_cantilever_faceload import setup

        # The magnetic writer refuses a card without permeability after reading its table
        setup(self.document, "elmer", test_mode=True)
        ObjectsFem.makeEquationHeat(self.document, self.document.SolverElmer)
        ObjectsFem.makeEquationMagnetodynamic(self.document, self.document.SolverElmer)
        material = self.document.FemMaterial
        card_values = dict(material.Material)
        card_values.pop("RelativePermeability", None)
        card_values.update(
            {
                "Density": "8960 kg/m^3",
                "ThermalConductivity": "401 W/m/K",
                "SpecificHeat": "385 J/kg/K",
            }
        )
        material.Material = card_values
        material.ElectricalConductivityTable = ["300 K, 100 S/m", "400 K, 200 S/m"]
        material.References = [(self.document.Box, "Solid1")]
        self.document.recompute()
        working_dir = testtools.get_fem_test_tmp_dir(self.pre_dir_name + "refused_range")
        w = writer.Writer(self.document.SolverElmer, working_dir)
        printed = self.printed_warnings(w.write_solver_input, writer.WriteError)
        self.assertEqual([text for text in printed if "table covers" in text], [])

    def printed_warnings(self, action, expected_error=None):
        """The warnings the writer printed while running action"""
        printed = []

        class Recorder:
            def __getattr__(self, name):
                return getattr(console, name)

            def PrintWarning(self, text):
                printed.append(text)

        console = writer.Console
        writer.Console = Recorder()
        try:
            if expected_error:
                with self.assertRaises(expected_error):
                    action()
            else:
                action()
        finally:
            writer.Console = console
        return printed

    # ********************************************************************************************
    def test_notes_follow_the_written_sif(self):
        fcc_print("")
        import re
        import ObjectsFem
        from femexamples.ccx_cantilever_faceload import setup

        setup(self.document, "elmer", test_mode=True)
        solver = self.document.SolverElmer
        current = ObjectsFem.makeEquationStaticCurrent(self.document, solver)
        heat = ObjectsFem.makeEquationHeat(self.document, solver)
        material = self.document.FemMaterial
        card_values = dict(material.Material)
        card_values.update(
            {
                "Density": "8960 kg/m^3",
                "ThermalConductivity": "401 W/m/K",
                "SpecificHeat": "385 J/kg/K",
            }
        )
        material.Material = card_values
        material.ElectricalConductivityTable = ["900 K, 60000 S/m", "1500 K, 70000 S/m"]
        working_dir = testtools.get_fem_test_tmp_dir(self.pre_dir_name + "written_notes")
        # With negative priorities the sif numbers these solvers by reference, Heat comes first
        heat.Priority, current.Priority = -2, -1
        self.document.recompute()
        w = writer.Writer(solver, working_dir)
        printed = self.printed_warnings(w.write_solver_input)
        # Elmer runs the solvers by their number, here not the order of the blocks in the file
        with open(join(working_dir, self.infilename + self.ending), encoding="utf-8") as fh:
            numbers = {
                name: int(number)
                for number, name in re.findall(
                    r'Solver (\d+)\n.*?Equation = String "(.*?)"', fh.read(), re.S
                )
            }
        self.assertLess(numbers["Heat"], numbers["Stat Current Solver"])
        self.assertEqual([text for text in printed if "runs before" in text], [])
        # A later material without references puts a scalar over the table, see #34384
        heat.Priority, current.Priority = 20, 50
        default = ObjectsFem.makeMaterialSolid(self.document, "DefaultMaterial")
        default_values = dict(card_values)
        default_values["ElectricalConductivity"] = "123 S/m"
        default.Material = default_values
        self.document.Analysis.addObject(default)
        self.document.recompute()
        w = writer.Writer(solver, working_dir)
        printed = self.printed_warnings(w.write_solver_input)
        self.assertEqual([t for t in printed if "electrical conductivity" in t], [])
        # The same table on the later material gives only its own range, the first one is gone
        default.ElectricalConductivityTable = ["900 K, 60000 S/m", "1500 K, 70000 S/m"]
        self.document.recompute()
        w = writer.Writer(solver, working_dir)
        printed = self.printed_warnings(w.write_solver_input)
        ranges = [t for t in printed if "electrical conductivity: the table covers" in t]
        self.assertEqual(len(ranges), 1)
        self.assertTrue(ranges[0].startswith("DefaultMaterial"))
        self.document.Analysis.removeObject(default)
        self.document.removeObject(default.Name)
        # A write that fails tells nothing
        self.document.recompute()
        w = writer.Writer(solver, join(working_dir, "missing", "folder"))
        printed = self.printed_warnings(w.write_solver_input, OSError)
        self.assertEqual(printed, [])

    # ********************************************************************************************
    def test_electrical_scalar_has_no_order_note(self):
        fcc_print("")
        import ObjectsFem
        from femexamples.ccx_cantilever_faceload import setup

        # Current created before Heat, a one row table is a scalar with nothing to tell
        # A thermal table on the same body changes nothing
        setup(self.document, "elmer", test_mode=True)
        ObjectsFem.makeEquationStaticCurrent(self.document, self.document.SolverElmer)
        ObjectsFem.makeEquationHeat(self.document, self.document.SolverElmer)
        material = self.document.FemMaterial
        card_values = dict(material.Material)
        card_values.update(
            {
                "Density": "8960 kg/m^3",
                "ThermalConductivity": "401 W/m/K",
                "SpecificHeat": "385 J/kg/K",
            }
        )
        material.Material = card_values
        material.ElectricalConductivityTable = ["900 K, 60000 S/m"]
        material.ConductivityTable = ["300 K, 401 W/m/K", "400 K, 393 W/m/K"]
        self.document.recompute()
        working_dir = testtools.get_fem_test_tmp_dir(self.pre_dir_name + "scalar_order")
        w = writer.Writer(self.document.SolverElmer, working_dir)
        w.write_solver_input()
        self.assertEqual([key for _b, key, _t in w._writtenTables()], ["Heat Conductivity"])
        self.assertEqual(w.solverOrderNotes(), [])

    # ********************************************************************************************
    def test_electrical_table_needs_heat(self):
        fcc_print("")
        import ObjectsFem
        from femexamples.ccx_cantilever_faceload import setup

        # Elmer stops on a table whose variable no solver defines (ListParseStrToVars)
        setup(self.document, "elmer", test_mode=True)
        ObjectsFem.makeEquationStaticCurrent(self.document, self.document.SolverElmer)
        self.document.FemMaterial.ElectricalConductivityTable = [
            "900 K, 60000 S/m",
            "1500 K, 70000 S/m",
        ]
        self.document.recompute()
        working_dir = testtools.get_fem_test_tmp_dir(self.pre_dir_name + "table_needs_heat")
        w = writer.Writer(self.document.SolverElmer, working_dir)
        printed = self.printed_warnings(w.write_solver_input, writer.WriteError)
        # A refused table must not have told its range first
        self.assertEqual(printed, [])
        # A Heat equation on another body leaves no temperature here, Elmer reads zero
        heat = ObjectsFem.makeEquationHeat(self.document, self.document.SolverElmer)
        heat.References = [(self.document.Box, "Solid2")]
        self.document.recompute()
        w = writer.Writer(self.document.SolverElmer, working_dir)
        with self.assertRaises(writer.WriteError) as caught:
            w.write_solver_input()
        self.assertIn("Heat equation active on Solid1", str(caught.exception))
        # A later material puts a scalar over the table, the sif needs no temperature then
        default = ObjectsFem.makeMaterialSolid(self.document, "DefaultMaterial")
        default_values = dict(self.document.FemMaterial.Material)
        default_values["ElectricalConductivity"] = "123 S/m"
        default.Material = default_values
        self.document.Analysis.addObject(default)
        self.document.recompute()
        w = writer.Writer(self.document.SolverElmer, working_dir)
        w.write_solver_input()
        self.assertEqual(w._writtenTables(), [])
        with open(join(working_dir, self.infilename + self.ending), encoding="utf-8") as fh:
            self.assertIn("Electric Conductivity = Real 123.0", fh.read())

    # ********************************************************************************************
    def test_electrical_table_solver_order(self):
        fcc_print("")
        import re
        import ObjectsFem
        from femexamples.ccx_cantilever_faceload import setup

        # Elmer runs its solvers in their numbered order, one steady state iteration by default
        setup(self.document, "elmer", test_mode=True)
        solver = self.document.SolverElmer
        current = ObjectsFem.makeEquationStaticCurrent(self.document, solver)
        heat = ObjectsFem.makeEquationHeat(self.document, solver)
        material = self.document.FemMaterial
        card_values = dict(material.Material)
        card_values["Density"] = "8960 kg/m^3"
        card_values["ThermalConductivity"] = "401 W/m/K"
        card_values["SpecificHeat"] = "385 J/kg/K"
        material.Material = card_values
        material.ElectricalConductivityTable = ["900 K, 60000 S/m", "1500 K, 70000 S/m"]
        self.document.recompute()
        working_dir = testtools.get_fem_test_tmp_dir(self.pre_dir_name + "solver_order")
        w = writer.Writer(solver, working_dir)
        printed = self.printed_warnings(w.write_solver_input)
        notes = w.solverOrderNotes()
        self.assertEqual(len(notes), 1)
        # The solvers are named as in the sif, where the user finds them
        self.assertIn("Stat Current Solver runs before Heat", notes[0])
        # A note computed but never shown is worth nothing to the user
        self.assertIn(notes[0] + "\n", printed)
        self.assertEqual(len([t for t in printed if "covers 900 K to 1500 K" in t]), 1)
        # Equations created later get a lower Priority, raising Heat's puts it first
        self.assertLess(heat.Priority, current.Priority)
        heat.Priority = current.Priority + 1
        self.document.recompute()
        w = writer.Writer(solver, working_dir)
        w.write_solver_input()
        self.assertEqual(w.solverOrderNotes(), [])
        with open(join(working_dir, self.infilename + self.ending), encoding="utf-8") as fh:
            numbers = dict(
                (name, int(number))
                for number, name in re.findall(
                    r'Solver (\d+)\n.*?Equation = String "(.*?)"', fh.read(), re.S
                )
            )
        self.assertLess(numbers["Heat"], numbers["Stat Current Solver"])
        # More steady state iterations let the current see the computed temperature
        heat.Priority = current.Priority - 1
        solver.SteadyStateMaxIterations = 5
        self.document.recompute()
        w = writer.Writer(solver, working_dir)
        w.write_solver_input()
        self.assertEqual(w.solverOrderNotes(), [])
        # A transient run writes no Steady State Max Iterations, raising it changes nothing
        solver.SimulationType = "Transient"
        self.document.recompute()
        w = writer.Writer(solver, working_dir)
        w.write_solver_input()
        self.assertEqual(len(w.solverOrderNotes()), 1)
        self.assertIn("previous time step", w.solverOrderNotes()[0])
        solver.SimulationType = "Steady State"
        solver.SteadyStateMaxIterations = 1
        # With equal priorities Heat is written first, nothing to say
        heat.Priority = current.Priority
        self.document.recompute()
        w = writer.Writer(solver, working_dir)
        w.write_solver_input()
        self.assertEqual(w.solverOrderNotes(), [])
        # A Heat on another body, run first, must not hide the one of this body run after
        heat.Priority = current.Priority - 1
        other = ObjectsFem.makeEquationHeat(self.document, solver)
        other.Priority = current.Priority + 10
        other.References = [(self.document.Box, "Solid2")]
        self.document.recompute()
        w = writer.Writer(solver, working_dir)
        w.write_solver_input()
        notes = w.solverOrderNotes()
        self.assertEqual(len(notes), 1)
        self.assertIn("Stat Current Solver runs before Heat", notes[0])
        # The same Heat on this body runs first, the current reads its temperature
        other.References = []
        self.document.recompute()
        w = writer.Writer(solver, working_dir)
        w.write_solver_input()
        self.assertEqual(w.solverOrderNotes(), [])

    # ********************************************************************************************
    def test_flow_without_heat_keeps_scalar(self):
        fcc_print("")
        import ObjectsFem
        from femexamples.ccx_cantilever_faceload import setup

        # FlowSolve doesn't read the conductivity, with no Temperature a table has no use
        setup(self.document, "elmer", test_mode=True)
        ObjectsFem.makeEquationFlow(self.document, self.document.SolverElmer)
        material = self.document.FemMaterial
        card_values = dict(material.Material)
        card_values["Density"] = "1000 kg/m^3"
        card_values["DynamicViscosity"] = "0.001 Pa*s"
        card_values["ThermalConductivity"] = "0.6 W/m/K"
        material.Material = card_values
        # 2 rows at one temperature, the writer would refuse it as a table
        material.ConductivityTable = ["300 K, 0.5 W/m/K", "300 K, 0.7 W/m/K"]
        self.document.recompute()
        working_dir = testtools.get_fem_test_tmp_dir(self.pre_dir_name + "flow_no_heat")
        w = writer.Writer(self.document.SolverElmer, working_dir)
        printed = self.printed_warnings(w.write_solver_input)
        with open(join(working_dir, self.infilename + self.ending), encoding="utf-8") as fh:
            text = fh.read()
        self.assertIn("Heat Conductivity = Real 0.6", text)
        self.assertEqual([t for t in printed if "table covers" in t], [])

    # ********************************************************************************************
    def test_elastic_tables_not_written(self):
        fcc_print("")
        import ObjectsFem
        from femexamples.ccx_cantilever_faceload import setup

        # The elasticity equation writes the scalars, the user must know the tables are left out
        setup(self.document, "elmer", test_mode=True)
        material = self.document.FemMaterial
        material.ElasticTable = ["300 K, 119000 MPa, 0.34", "400 K, 110000 MPa, 0.34"]
        material.ExpansionTable = ["300 K, 1.65e-05 1/K", "400 K, 1.72e-05 1/K"]
        self.document.recompute()
        working_dir = testtools.get_fem_test_tmp_dir(self.pre_dir_name + "elastic_tables")
        w = writer.Writer(self.document.SolverElmer, working_dir)
        printed = [t for t in self.printed_warnings(w.write_solver_input) if "Elmer yet" in t]
        self.assertEqual(len(printed), 1)
        self.assertTrue(
            printed[0].startswith("FemMaterial: ElasticTable and ExpansionTable aren't")
        )
        material.ExpansionTable = []
        self.document.recompute()
        w = writer.Writer(self.document.SolverElmer, working_dir)
        printed = [t for t in self.printed_warnings(w.write_solver_input) if "Elmer yet" in t]
        self.assertEqual(len(printed), 1)
        self.assertTrue(printed[0].startswith("FemMaterial: ElasticTable isn't written"))
        material.ExpansionTable = ["300 K, 1.65e-05 1/K", "400 K, 1.72e-05 1/K"]
        # A later material without tables takes the body, nothing of the first one is left out
        default = ObjectsFem.makeMaterialSolid(self.document, "DefaultMaterial")
        default.Material = dict(material.Material)
        self.document.Analysis.addObject(default)
        self.document.recompute()
        w = writer.Writer(self.document.SolverElmer, working_dir)
        printed = self.printed_warnings(w.write_solver_input)
        self.assertEqual([t for t in printed if "Elmer yet" in t], [])
        self.document.Analysis.removeObject(default)
        self.document.removeObject(default.Name)
        # No elasticity equation, the tables have no reader to miss
        self.document.removeObject(self.document.SolverElmer.Group[0].Name)
        self.document.recompute()
        w = writer.Writer(self.document.SolverElmer, working_dir)
        printed = self.printed_warnings(w.write_solver_input)
        self.assertEqual([t for t in printed if "Elmer yet" in t], [])

    # ********************************************************************************************
    def test_temperature_table_rules(self):
        table = writer.temperature_table
        self.assertIsNone(table([], "empty"))
        # Elmer would read a single row as value * T
        self.assertEqual(table([(300.0, 401.0)], "one row"), 401.0)
        with self.assertRaises(writer.WriteError) as caught:
            table([(300.0, 401.0), (300.0, 398.0)], "Copper thermal conductivity")
        self.assertIn("Copper thermal conductivity", str(caught.exception))
        self.assertIn("300 K", str(caught.exception))
        # Flat points keep the edge values outside the table, as CalculiX does
        written = table([(300.0, 401.0), (600.0, 379.0)], "two rows")
        rows = [line.split() for line in written.splitlines()[2:-1]]
        self.assertEqual(
            rows,
            [["0", "401"], ["300", "401"], ["600", "379"], ["1000000", "379"]],
        )
        # The floor and the ceiling while the ends lie inside them, past them a margin that
        # stays apart from the end once written on 13 digits
        flat = writer.flat_point
        self.assertEqual((flat(300.0, -1), flat(600.0, 1)), (0.0, 1.0e6))
        self.assertEqual((flat(0.0, -1), flat(1.0e6, 1)), (-1.0, 2000001.0))
        for end, side in ((1.0e13, 1), (-1.0e13, -1), (5.0e15, 1)):
            self.assertNotEqual(f"{flat(end, side):.13g}", f"{end:.13g}")
        self.assertEqual(flat(1.0e308, 1), float("inf"))
        # A table reaching 0 K or 1e6 K still gets a flat point past each end
        written = table([(0.0, 5.0), (1.0e6, 20.0)], "wide rows")
        rows = [line.split() for line in written.splitlines()[2:-1]]
        self.assertEqual(rows[0][1], "5")
        self.assertLess(float(rows[0][0]), 0.0)
        self.assertEqual(rows[-1][1], "20")
        self.assertGreater(float(rows[-1][0]), 1.0e6)
        # Two temperatures Elmer would read as one once written
        with self.assertRaises(writer.WriteError):
            table([(300.0, 401.0), (300.00000000001, 379.0)], "close rows")
        # A quantity like 1e309 K parses to inf, and 1e308 K has no room for its flat point
        for last in (float("inf"), float("nan"), 1.0e308):
            with self.assertRaises(writer.WriteError):
                table([(300.0, 5.0), (last, 20.0)], "endless rows")
        # The card holds 1e308 K, not inf, the message must not blame it
        with self.assertRaises(writer.WriteError) as caught:
            table([(300.0, 5.0), (1.0e308, 20.0)], "endless rows")
        self.assertIn("too large", str(caught.exception))
        # A single row goes out as a scalar, it's checked all the same
        for row in ((300.0, float("inf")), (float("inf"), 0.5)):
            with self.assertRaises(writer.WriteError):
                table([row], "endless row")
        with self.assertRaises(writer.WriteError):
            table([(-0.0, 5.0), (0.0, 20.0)], "signed zero")
        # The flat points stay apart from the ends once written, whatever their size
        for last in (1.0e13, 999999.99999999):
            written = table([(300.0, 5.0), (last, 20.0)], "huge rows")
            temperatures = [float(line.split()[0]) for line in written.splitlines()[2:-1]]
            self.assertEqual(temperatures, sorted(set(temperatures)))
        # The range told to the user is the data, not the flat points added around it
        note = writer.table_range_note(
            [(300.0, 401.0), (600.0, 379.0)], "Copper thermal conductivity"
        )
        self.assertIn("Copper thermal conductivity", note)
        self.assertIn("300 K to 600 K", note)
        # A conductivity typed in the wrong unit is named, with the form to use
        rows = materialtables.sorted_rows(["300 K, 6e7 S/m"], "ElectricalConductivityTable", "Cu")
        self.assertEqual(rows[0][1][0].getValueAs("S/m").Value, 6e7)
        with self.assertRaises(ValueError) as caught:
            materialtables.sorted_rows(["300 K, 6e7 V"], "ElectricalConductivityTable", "Cu")
        self.assertIn("Write it as '293.15 K, 5.96e+07 S/m'", str(caught.exception))

    # ********************************************************************************************
    def input_file_writing_test(self, base_name):
        self.document.recompute()

        # get analysis working directory and save FreeCAD file
        working_dir = testtools.get_fem_test_tmp_dir(self.pre_dir_name + base_name)
        save_fc_file = join(working_dir, base_name + ".FCStd")
        # fcc_print("Save FreeCAD file to {} ...".format(save_fc_file))
        self.document.saveAs(save_fc_file)

        # write input file
        w = writer.Writer(self.document.SolverElmer, working_dir)
        w.write_solver_input()

        # compare input file with the given one
        inpfile_given = join(self.test_file_dir, base_name + self.ending)
        inpfile_totest = join(working_dir, self.infilename + self.ending)
        # fcc_print("Comparing {}  to  {}".format(inpfile_given, inpfile_totest))
        ret = testtools.compare_inp_files(inpfile_given, inpfile_totest)
        self.assertFalse(ret, f"Elmer write_inp_file for {base_name} test failed.\n{ret}")
        return inpfile_totest
