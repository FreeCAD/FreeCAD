# ***************************************************************************
# *   Copyright (c) 2015 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "Common FEM unit tests"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecadweb.org"

import sys
import unittest

import FreeCAD

import ObjectsFem
from . import support_utils as testtools
from .support_utils import fcc_print


class TestFemCommon(unittest.TestCase):
    fcc_print("import TestFemCommon")

    # ********************************************************************************************
    def setUp(
        self
    ):
        # setUp is executed before every test

        # new document
        self.document = FreeCAD.newDocument(self.__class__.__name__)

    # ********************************************************************************************
    def tearDown(
        self
    ):
        # tearDown is executed after every test
        FreeCAD.closeDocument(self.document.Name)

    # ********************************************************************************************
    def test_00print(
        self
    ):
        # since method name starts with 00 this will be run first
        # this test just prints a line with stars

        fcc_print("\n{0}\n{1} run FEM TestFemCommon tests {2}\n{0}".format(
            100 * "*",
            10 * "*",
            61 * "*"
        ))

    # ********************************************************************************************
    def test_adding_refshaps(
        self
    ):
        doc = self.document
        slab = doc.addObject("Part::Plane", "Face")
        slab.Length = 500.00
        slab.Width = 500.00
        cf = ObjectsFem.makeConstraintFixed(doc)
        ref_eles = []
        # FreeCAD list property doesn't seem to support append,
        # thus we need a workaround
        # which on many elements is even much faster
        for i, face in enumerate(slab.Shape.Edges):
            ref_eles.append("Edge%d" % (i + 1))
        cf.References = [(slab, ref_eles)]
        doc.recompute()
        expected_reflist = [(slab, ("Edge1", "Edge2", "Edge3", "Edge4"))]
        assert_err_message = (
            "Adding reference shapes did not result in expected list {} != {}"
            .format(cf.References, expected_reflist)
        )
        self.assertEqual(cf.References, expected_reflist, assert_err_message)

    # ********************************************************************************************
    def test_pyimport_all_FEM_modules(
        self
    ):
        # we're going to try to import all python modules from FreeCAD FEM
        pymodules = []

        # collect all Python modules in FEM
        pymodules += testtools.collect_python_modules("")  # FEM main dir
        pymodules += testtools.collect_python_modules("femexamples")
        pymodules += testtools.collect_python_modules("feminout")
        pymodules += testtools.collect_python_modules("femmesh")
        pymodules += testtools.collect_python_modules("femobjects")
        pymodules += testtools.collect_python_modules("femresult")
        pymodules += testtools.collect_python_modules("femtest")
        pymodules += testtools.collect_python_modules("femtools")
        pymodules += testtools.collect_python_modules("femsolver")
        # TODO test with join on Windows, the use of os.path.join
        # in the following code seems to create problems on Windows OS
        pymodules += testtools.collect_python_modules("femsolver/elmer")
        pymodules += testtools.collect_python_modules("femsolver/elmer/equations")
        pymodules += testtools.collect_python_modules("femsolver/z88")
        pymodules += testtools.collect_python_modules("femsolver/calculix")
        if FreeCAD.GuiUp:
            pymodules += testtools.collect_python_modules("femcommands")
            pymodules += testtools.collect_python_modules("femguiobjects")
            pymodules += testtools.collect_python_modules("femguiutils")
            pymodules += testtools.collect_python_modules("femtaskpanels")
            pymodules += testtools.collect_python_modules("femviewprovider")

        # import all collected modules
        # fcc_print(pymodules)
        for mod in pymodules:
            # migrate modules do not import on Python 2
            if (
                mod == "femtools.migrate_app"
                or mod == "femguiutils.migrate_gui"
            ) and sys.version_info.major < 3:
                continue

            if (
                mod == "femsolver.solver_taskpanel"
                or mod == "femexamples.examplesgui"
                or mod == "TestFemGui"
            ) and not FreeCAD.GuiUp:
                continue

            fcc_print("Try importing {0} ...".format(mod))
            try:
                im = __import__("{0}".format(mod))
            except ImportError:
                im = False
            if not im:
                # to get an error message what was going wrong
                __import__("{0}".format(mod))
            self.assertTrue(im, "Problem importing {0}".format(mod))
