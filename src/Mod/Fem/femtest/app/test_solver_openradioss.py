# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 Nicholas Herrmann <nicholas.herrmann@proton.me>    *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

__title__ = "Solver OpenRadioss FEM unit tests"
__author__ = "Nicholas Herrmann"
__url__ = "https://www.freecad.org"

import json
import os
import pathlib
import tempfile
import unittest
import xml.etree.ElementTree as ET
import zipfile
from os.path import join

import FreeCAD

import femsolver.run
from femsolver.openradioss import results
from femsolver.openradioss import runner
from femsolver.openradioss import writer
from . import support_utils as testtools
from .support_utils import fcc_print
from .support_utils import get_namefromdef


def _solver_installed():
    return not runner.get_installation().problems()


class TestSolverOpenRadioss(unittest.TestCase):
    fcc_print("import TestSolverOpenRadioss")

    # ********************************************************************************************
    def setUp(self):
        # setUp is executed before every test

        # new document
        self.document = FreeCAD.newDocument(self.__class__.__name__)

        # more inits
        self.pre_dir_name = "solver_openradioss_"
        self.ending = ".k"
        self.infilename = "radioss"
        self.test_file_dir = join(testtools.get_fem_test_home_dir(), "openradioss")

    # ********************************************************************************************
    def tearDown(self):
        # tearDown is executed after every test
        FreeCAD.closeDocument(self.document.Name)

    # ********************************************************************************************
    def test_00print(self):
        # since method name starts with 00 this will be run first
        # this test just prints a line with stars

        fcc_print(
            "\n{0}\n{1} run FEM TestSolverOpenRadioss tests {2}\n{0}".format(
                100 * "*", 10 * "*", 51 * "*"
            )
        )

    # ********************************************************************************************
    def test_solver_properties(self):
        import ObjectsFem

        solver = ObjectsFem.makeSolverOpenRadioss(self.document)
        self.assertEqual(solver.AnalysisType, "explicit")
        self.assertEqual(solver.LoadCurve, "Ramp")
        self.assertAlmostEqual(solver.EndTime.getValueAs("s").Value, 0.01)
        self.assertEqual(solver.OutputFrames, 20)
        self.assertEqual(solver.MassScalingMicroseconds, 0.0)
        self.assertIsNone(solver.AdditionalKeywords)

    # ********************************************************************************************
    def test_ccx_cantilever_faceload(self):
        fcc_print("")
        self.setup_cantilever("ccx_cantilever_faceload")
        self.input_file_writing_test(get_namefromdef("test_"))

    # ********************************************************************************************
    def test_ccx_cantilever_nodeload(self):
        fcc_print("")
        self.setup_cantilever("ccx_cantilever_nodeload")
        self.input_file_writing_test(get_namefromdef("test_"))

    # ********************************************************************************************
    @unittest.skipUnless(_solver_installed(), "OpenRadioss/OpenCourant not installed")
    def test_run_ccx_cantilever_faceload(self):
        fcc_print("")
        self.setup_cantilever("ccx_cantilever_faceload")
        self.document.recompute()
        working_dir = testtools.get_fem_test_tmp_dir(self.pre_dir_name + "run")
        solver = self.document.SolverOpenRadioss
        solver.Threads = 1
        machine = solver.Proxy.createMachine(solver, working_dir, True)
        machine.target = femsolver.run.RESULTS
        machine.start()
        machine.join()
        self.assertFalse(machine.failed, f"OpenRadioss run failed, see {working_dir}")
        result = self.document.getObject("OpenRadiossResult")
        self.assertIsNotNone(result)
        # the beam bends in -z
        self.assertLess(min(v.z for v in result.DisplacementVectors), 0.0)

    # ********************************************************************************************
    def setup_cantilever(self, example):
        module = __import__(f"femexamples.{example}", fromlist=["setup"])
        module.setup(self.document, "openradioss", test_mode=True)
        # an explicit solver needs the density, the CalculiX example does not set it
        material_obj = self.document.FemMaterial
        mat = material_obj.Material
        mat["Density"] = "7900 kg/m^3"
        material_obj.Material = mat

    # ********************************************************************************************
    def input_file_writing_test(self, base_name):
        self.document.recompute()

        # get analysis working directory and save FreeCAD file
        working_dir = testtools.get_fem_test_tmp_dir(self.pre_dir_name + base_name)
        save_fc_file = join(working_dir, base_name + ".FCStd")
        self.document.saveAs(save_fc_file)

        # write input file
        machine = self.document.SolverOpenRadioss.Proxy.createMachine(
            self.document.SolverOpenRadioss, working_dir, True  # set testmode to True
        )
        machine.target = femsolver.run.PREPARE
        machine.start()
        machine.join()  # wait for the machine to finish

        # compare input file with the given one
        inpfile_given = join(self.test_file_dir, base_name + self.ending)
        inpfile_totest = join(working_dir, self.infilename + self.ending)
        ret = testtools.compare_inp_files(inpfile_given, inpfile_totest)
        self.assertFalse(ret, f"OpenRadioss deck writing for {base_name} test failed.\n{ret}")


# ************************************************************************************************
# deck writer, installation and result files, without documents

LEGACY_VTK = """# vtk DataFile Version 3.0
vtk output
ASCII
DATASET UNSTRUCTURED_GRID
FIELD FieldData 2
TIME 1 1 double
0.5
CYCLE 1 1 int
10
POINTS 4 float
0 0 0
1 0 0
1 1 0
0 1 0

CELLS 1 5
4 0 1 2 3

CELL_TYPES 1
9

POINT_DATA 4
SCALARS NODE_ID int 1
LOOKUP_TABLE default
1
2
3
4

VECTORS Velocity float
0 0 0 0 0 0 0 0 0 0 0 0
VECTORS Displacement float
0 0 0 0.1 0 0 0.1 0 0 0 0 0
CELL_DATA 1
SCALARS ELEMENT_ID int 1
LOOKUP_TABLE default
7
TENSORS 2DELEM_Stress_(upper) float
100 0 0
0 0 0
0 0 0
"""


def block_model(nx=4, ny=1, nz=1, size=10.0, **overrides):
    """A cantilever block of hexahedra along x, fixed at x = 0."""
    nodes, nid = {}, {}
    for k in range(nz + 1):
        for j in range(ny + 1):
            for i in range(nx + 1):
                n = len(nodes) + 1
                nodes[n] = (i * size, j * size, k * size)
                nid[i, j, k] = n
    elements = []
    for k in range(nz):
        for j in range(ny):
            for i in range(nx):
                corners = [
                    nid[i, j, k],
                    nid[i + 1, j, k],
                    nid[i + 1, j + 1, k],
                    nid[i, j + 1, k],
                    nid[i, j, k + 1],
                    nid[i + 1, j, k + 1],
                    nid[i + 1, j + 1, k + 1],
                    nid[i, j + 1, k + 1],
                ]
                elements.append((len(elements) + 1, corners))
    tip = [n for (i, _j, _k), n in nid.items() if i == nx]
    # counter-clockwise from above: normal +z (outward)
    top = [
        [nid[i, j, nz], nid[i + 1, j, nz], nid[i + 1, j + 1, nz], nid[i, j + 1, nz]]
        for i in range(nx)
        for j in range(ny)
    ]
    model = {
        "title": "block",
        "end_time": 0.002,
        "output_interval": 0.0005,
        "load_curve": "ramp",
        "nodes": nodes,
        "parts": [
            {
                "pid": 1,
                "title": "Steel",
                "kind": "solid",
                "elform": 1,
                "material": {"rho": 7.9e-9, "E": 210000.0, "nu": 0.3},
                "elements": elements,
            }
        ],
        "fixed": [
            {
                "title": "Fixed",
                "nodes": [n for (i, _j, _k), n in nid.items() if i == 0],
                "dofs": (1, 1, 1, 1, 1, 1),
            }
        ],
        "node_forces": [
            {"title": "Force", "loads": {n: (0.0, 0.0, 100.0 / len(tip)) for n in tip}}
        ],
        "pressures": [{"title": "Pressure", "pressure": 0.01, "segments": top}],
        "gravity": (0.0, 0.0, -9806.65),
        "extra": "*KEYWORD\n*DATABASE_GLSTAT\n1.0e-4\n*END",
    }
    model.update(overrides)
    return model, tip


def fake_installation(root, executable=True, cfg=True):
    """A directory tree that looks like an unpacked release (empty executables)."""
    inst = runner.Installation(root)
    os.makedirs(os.path.join(root, "exec"), exist_ok=True)
    for tool in ("starter", "engine", "anim_to_vtk"):
        path = inst.executable(tool)
        with open(path, "w") as f:
            f.write("")
        os.chmod(path, 0o755 if executable else 0o644)
    if cfg:
        os.makedirs(os.path.join(root, "hm_cfg_files", "config", "CFG"), exist_ok=True)
    os.makedirs(os.path.join(root, "extlib", "hm_reader", inst.libdir), exist_ok=True)
    os.makedirs(os.path.join(root, "extlib", "h3d", "lib", inst.libdir), exist_ok=True)
    return inst


class TestOpenRadiossFiles(unittest.TestCase):
    fcc_print("import TestOpenRadiossFiles")

    # --- deck writer ---------------------------------------------------------------------------
    def test_deck_keywords_and_cards(self):
        deck = writer.write_deck(block_model()[0])
        lines = deck.splitlines()
        self.assertEqual((lines[0], lines[-1]), ("*KEYWORD", "*END"))
        self.assertEqual(deck.count("*KEYWORD"), 1)
        self.assertEqual(deck.count("*END"), 1)
        for keyword in (
            "*CONTROL_TERMINATION",
            "*DATABASE_BINARY_D3PLOT",
            "*DEFINE_CURVE",
            "*PART",
            "*SECTION_SOLID",
            "*MAT_ELASTIC",
            "*NODE",
            "*ELEMENT_SOLID",
            "*SET_NODE_LIST",
            "*BOUNDARY_SPC_SET",
            "*LOAD_NODE_POINT",
            "*LOAD_SEGMENT",
            "*LOAD_BODY_Z",
            "*DATABASE_GLSTAT",
        ):
            self.assertIn(keyword + "\n", deck)
        self.assertIn("*MAT_ELASTIC\n1, 7.9e-09, 210000, 0.3\n", deck)
        # two-card format of solids
        self.assertIn("*ELEMENT_SOLID\n1, 1\n1, 2, 7, 6, 11, 12, 17, 16\n", deck)
        # gravity in -z is a positive body load
        self.assertIn("*LOAD_BODY_Z\n1, 9806.65\n", deck)
        self.assertNotIn("*CONTROL_TIMESTEP", deck)

    def test_deck_ramp_and_constant_curves(self):
        ramp = writer.write_deck(block_model()[0])
        self.assertIn("0.0, 0.0\n0.002, 1.0", ramp)
        constant = writer.write_deck(block_model(load_curve="constant")[0])
        self.assertIn("0.0, 1.0\n0.002, 1.0", constant)

    def test_deck_mass_scaling(self):
        deck = writer.write_deck(block_model(mass_scaling_dt=1e-6)[0])
        self.assertIn("*CONTROL_TIMESTEP\n", deck)
        self.assertIn("\n0.0, 0.9, 0, 0.0, -1e-06\n", deck)

    def test_deck_shells_and_triangles(self):
        nodes = {1: (0, 0, 0), 2: (1, 0, 0), 3: (1, 1, 0), 4: (0, 1, 0)}
        model = {
            "end_time": 0.001,
            "nodes": nodes,
            "parts": [
                {
                    "pid": 1,
                    "kind": "shell",
                    "elform": 2,
                    "thickness": 2.0,
                    "material": {"rho": 1e-9, "E": 1000.0, "nu": 0.3},
                    "elements": [(1, [1, 2, 3]), (2, [1, 3, 4])],
                }
            ],
            "pressures": [{"pressure": 0.5, "segments": [[1, 2, 3]]}],
        }
        deck = writer.write_deck(model)
        self.assertIn("*SECTION_SHELL\n1, 2\n2, 2, 2, 2\n", deck)
        self.assertIn("*ELEMENT_SHELL\n1, 1, 1, 2, 3, 3\n2, 1, 1, 3, 4, 4\n", deck)
        self.assertIn("*LOAD_SEGMENT\n1, 0.5, 0.0, 1, 2, 3, 3\n", deck)

    def test_deck_only_used_nodes_are_written(self):
        model, _tip = block_model()
        model["nodes"][999] = (5.0, 5.0, 5.0)  # e.g. a node of a face element
        model["fixed"][0]["nodes"].append(999)
        deck = writer.write_deck(model)
        self.assertNotIn("\n999, ", deck)

    def test_install_check_deck(self):
        deck = runner.install_check_deck()
        self.assertIn("*ELEMENT_SOLID\n1, 1\n1, 2, 3, 4, 5, 6, 7, 8\n", deck)
        self.assertIn("*BOUNDARY_SPC_SET\n", deck)

    # --- job and result files ------------------------------------------------------------------
    def test_result_files(self):
        for name in (
            "job_0000.out",
            "job_0001.rad",
            "job_0000_0001.rst",
            "jobA001",
            "jobA012.vtu",
            "job.pvd",
            "jobT01",
        ):
            self.assertTrue(runner.is_result_file(name, "job"), name)
        for name in ("job.k", "job_notes.txt", "jobAx", "other_0000.out"):
            self.assertFalse(runner.is_result_file(name, "job"), name)

    def test_listing_summary_and_messages(self):
        with tempfile.TemporaryDirectory() as tmp:
            normal = os.path.join(tmp, "a.out")
            with open(normal, "w") as f:
                f.write(
                    "WARNING ID :   1084\n** WARNING IN MATERIAL LAW1\n\n"
                    "     TERMINATION WITH WARNING\n"
                    "              0 ERROR(S)\n              1 WARNING(S)\n"
                )
            self.assertEqual(runner.listing_summary(normal), (0, 1))
            self.assertEqual(
                runner.listing_messages(normal, "WARNING"),
                ["WARNING ID :   1084 ** WARNING IN MATERIAL LAW1"],
            )
            early = os.path.join(tmp, "b.out")
            with open(early, "w") as f:
                f.write(
                    " +=====+\n   NODES\n +=====+\n\nERROR ID :    760\n** ERROR TRAPPED\n\n"
                    " RADIOSS STOP DUE TO INPUT ERROR\n"
                )
            self.assertEqual(runner.listing_summary(early), (1, None))
            self.assertEqual(
                runner.listing_messages(early), ["ERROR ID :    760 ** ERROR TRAPPED [NODES]"]
            )
            missing = os.path.join(tmp, "missing.out")
            self.assertEqual(runner.listing_summary(missing), (None, None))

    def test_job_files(self):
        with tempfile.TemporaryDirectory() as tmp:
            for name in (
                "job_0000.rad",
                "job_0001.rad",
                "job_0002.rad",
                "jobA001",
                "jobA002",
                "jobA001.vtu",
                "x_0001.rad",
            ):
                open(os.path.join(tmp, name), "w").close()
            self.assertEqual(
                [os.path.basename(f) for f in runner.engine_inputs(tmp, "job")],
                ["job_0001.rad", "job_0002.rad"],
            )
            self.assertEqual(
                [os.path.basename(f) for f in runner.animation_files(tmp, "job")],
                ["jobA001", "jobA002"],
            )

    def test_legacy_vtk_to_vtu_and_result_reading(self):
        with tempfile.TemporaryDirectory() as tmp:
            legacy, vtu = os.path.join(tmp, "a.vtk"), os.path.join(tmp, "a.vtu")
            with open(legacy, "w") as f:
                f.write(LEGACY_VTK)
            self.assertEqual(runner.legacy_vtk_to_vtu(legacy, vtu), 0.5)
            arrays = {a.get("Name"): a for a in ET.parse(vtu).getroot().iter("DataArray")}
            for name in ("TIME", "NODE_ID", "Velocity", "Displacement", "ELEMENT_ID"):
                self.assertIn(name, arrays)
            self.assertEqual(arrays["Displacement"].text.split()[3], "0.1")
            self.assertEqual(arrays["2DELEM_Stress_(upper)"].get("NumberOfComponents"), "9")
            # uniaxial stress
            mises = arrays["2DELEM_Stress_(upper) von Mises"]
            self.assertAlmostEqual(float(mises.text), 100.0)
            self.assertEqual(arrays["types"].text, "9")
            self.assertEqual(arrays["offsets"].text, "4")

            data = results.read_vtu(vtu)
            self.assertEqual(data["cells"], [[0, 1, 2, 3]])
            stresses = results.element_stresses(data)
            self.assertEqual(stresses, {7: (100.0, 0.0, 0.0, 0.0, 0.0, 0.0)})

    def test_von_mises_pure_shear(self):
        self.assertAlmostEqual(runner.von_mises([0, 10, 0, 10, 0, 0, 0, 0, 0]), 10 * 3**0.5)

    # --- installation --------------------------------------------------------------------------
    def test_environment(self):
        inst = runner.Installation("/opt/or")
        env = inst.environment(threads=3, base={"LD_LIBRARY_PATH": "/x", "PATH": "/x"})
        self.assertEqual(env["RAD_CFG_PATH"], os.path.join(inst.path, "hm_cfg_files"))
        self.assertEqual(env["OMP_NUM_THREADS"], "3")
        key = "PATH" if inst.ext == ".exe" else "LD_LIBRARY_PATH"
        self.assertTrue(env[key].endswith(os.pathsep + "/x"))
        self.assertEqual(
            inst.starter_command("/w/job.k", 2)[1:], ["-i", "job.k", "-nt", "2", "-np", "1"]
        )
        self.assertTrue(runner.Installation("/nonexistent").problems())

    def test_candidate_order(self):
        paths = runner.candidate_paths(
            environ={"OPENRADIOSS_PATH": "/x/or", "OPENCOURANT_PATH": "/x/oc"}, home="/h"
        )
        self.assertEqual(
            paths[:4],
            [
                "/x/oc",
                "/x/or",
                os.path.join("/h", "OpenCourant"),
                os.path.join("/h", "OpenRadioss"),
            ],
        )

    def test_detect_prefers_first_usable(self):
        with tempfile.TemporaryDirectory() as home:
            os.makedirs(os.path.join(home, "OpenCourant"))  # empty: not usable
            fake_installation(os.path.join(home, "OpenRadioss"))
            found = runner.detect_installation(environ={}, home=home)
            self.assertEqual(found.path, os.path.join(home, "OpenRadioss"))
            none = runner.detect_installation(environ={}, home=os.path.join(home, "none"))
            self.assertIsNone(none)

    def test_requisites(self):
        with tempfile.TemporaryDirectory() as tmp:
            inst = fake_installation(tmp, cfg=False)
            items = {name: ok for name, ok, _detail in inst.requisites()}
            self.assertTrue(items["Solver folder"])
            self.assertTrue(items["starter executable"])
            self.assertFalse(items["Reader configuration"])
            inst = fake_installation(tmp, executable=False)
            items = {name: ok for name, ok, _detail in inst.requisites()}
            self.assertTrue(items["Reader configuration"])
            self.assertEqual(items["engine executable"], inst.ext == ".exe")

    def test_test_run_reports_missing_installation(self):
        with tempfile.TemporaryDirectory() as tmp:
            ok, message = runner.Installation(os.path.join(tmp, "nothing")).test_run()
            self.assertFalse(ok)
            self.assertIn("not found", message)

    @unittest.skipUnless(_solver_installed(), "OpenRadioss/OpenCourant not installed")
    def test_test_run_with_real_solver(self):
        ok, message = runner.get_installation().test_run()
        self.assertTrue(ok, message)
        self.assertIn("0 errors", message)

    # --- download --------------------------------------------------------------------------------
    def test_release_asset_name(self):
        self.assertEqual(runner.release_asset_name("Linux", "x86_64"), "OpenCourant_linux64.zip")
        self.assertEqual(runner.release_asset_name("Linux", "aarch64"), "OpenCourant_linuxa64.zip")
        self.assertEqual(runner.release_asset_name("Windows", "AMD64"), "OpenCourant_win64.zip")
        with self.assertRaises(runner.OpenRadiossError):
            runner.release_asset_name("Darwin", "arm64")

    def test_latest_release_skips_prereleases(self):
        linux = "OpenCourant_linux64.zip"
        releases = [
            {
                "tag_name": "rc",
                "prerelease": True,
                "assets": [{"name": linux, "browser_download_url": "u0", "size": 1}],
            },
            {
                "tag_name": "latest-20261011",
                "assets": [
                    {"name": "OpenCourant_win64.zip", "browser_download_url": "w", "size": 2},
                    {"name": linux, "browser_download_url": "u1", "size": 83933903},
                ],
            },
        ]
        fetch = lambda _url: json.dumps(releases)
        self.assertEqual(
            runner.latest_release(linux, fetch=fetch), ("latest-20261011", "u1", 83933903)
        )
        with self.assertRaises(runner.OpenRadiossError):
            runner.latest_release("OpenCourant_other.zip", fetch=fetch)

    def make_release_zip(self, path, extra=None):
        with zipfile.ZipFile(path, "w") as z:
            info = zipfile.ZipInfo("OpenCourant/exec/starter_linux64_gf")
            info.external_attr = 0o100755 << 16
            z.writestr(info, "#!/bin/sh\n")
            z.writestr("OpenCourant/hm_cfg_files/config/CFG/README", "cfg")
            for name in extra or []:
                z.writestr(name, "x")

    def test_extract_release_keeps_executable_bit(self):
        with tempfile.TemporaryDirectory() as tmp:
            archive = os.path.join(tmp, "release.zip")
            self.make_release_zip(archive)
            path = runner.extract_release(archive, os.path.join(tmp, "OpenCourant_latest"))
            self.assertEqual(path, os.path.join(tmp, "OpenCourant_latest", "OpenCourant"))
            if os.name == "posix":
                starter = os.path.join(path, "exec", "starter_linux64_gf")
                self.assertTrue(os.access(starter, os.X_OK))

    def test_extract_release_rejects_unsafe_paths(self):
        with tempfile.TemporaryDirectory() as tmp:
            archive = os.path.join(tmp, "evil.zip")
            self.make_release_zip(archive, extra=["../outside.txt"])
            with self.assertRaises(runner.OpenRadiossError):
                runner.extract_release(archive, os.path.join(tmp, "target"))
            self.assertFalse(os.path.exists(os.path.join(tmp, "outside.txt")))

    def test_download_and_cancel(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = os.path.join(tmp, "source.bin")
            with open(source, "wb") as f:
                f.write(os.urandom(3 * 1024))
            url = pathlib.Path(source).as_uri()
            seen = []
            target = runner.download(
                url,
                os.path.join(tmp, "copy.bin"),
                lambda done, total: seen.append((done, total)),
                chunk_size=1024,
            )
            with open(target, "rb") as a, open(source, "rb") as b:
                self.assertEqual(a.read(), b.read())
            self.assertEqual(seen[-1], (3072, 3072))
            cancelled = os.path.join(tmp, "cancelled.bin")
            with self.assertRaises(runner.OpenRadiossError):
                runner.download(url, cancelled, lambda d, t: False, chunk_size=1024)
            self.assertFalse(os.path.exists(cancelled))
            self.assertFalse(os.path.exists(cancelled + ".part"))
