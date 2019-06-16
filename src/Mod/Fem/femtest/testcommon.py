# ***************************************************************************
# *   Copyright (c) 2015 - FreeCAD Developers                               *
# *   Author: Bernd Hahnebach <bernd@bimstatik.org>                         *
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
import unittest
from . import utilstest as testtools
from .utilstest import fcc_print


class TestFemCommon(unittest.TestCase):
    fcc_print('import TestFemCommon')

    # ********************************************************************************************
    def setUp(
        self
    ):
        # init, is executed before every test
        self.doc_name = "TestsFemCommon"
        try:
            FreeCAD.setActiveDocument(self.doc_name)
        except:
            FreeCAD.newDocument(self.doc_name)
        finally:
            FreeCAD.setActiveDocument(self.doc_name)
        self.active_doc = FreeCAD.ActiveDocument

    # ********************************************************************************************
    def test_adding_refshaps(
        self
    ):
        doc = self.active_doc
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
        expected_reflist = [(slab, ('Edge1', 'Edge2', 'Edge3', 'Edge4'))]
        assert_err_message = (
            'Adding reference shapes did not result in expected list {} != {}'
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
        pymodules += testtools.collect_python_modules('')  # FEM main dir
        pymodules += testtools.collect_python_modules('femexamples')
        pymodules += testtools.collect_python_modules('feminout')
        pymodules += testtools.collect_python_modules('femmesh')
        pymodules += testtools.collect_python_modules('femobjects')
        pymodules += testtools.collect_python_modules('femresult')
        pymodules += testtools.collect_python_modules('femtest')
        pymodules += testtools.collect_python_modules('femtools')
        pymodules += testtools.collect_python_modules('femsolver')
        # TODO test with join on Windows, the use of os.path.join
        # in the following code seems to create problems on Windows OS
        pymodules += testtools.collect_python_modules('femsolver/elmer')
        pymodules += testtools.collect_python_modules('femsolver/elmer/equations')
        pymodules += testtools.collect_python_modules('femsolver/z88')
        pymodules += testtools.collect_python_modules('femsolver/calculix')
        if FreeCAD.GuiUp:
            pymodules += testtools.collect_python_modules('femcommands')
            pymodules += testtools.collect_python_modules('femguiobjects')

        # import all collected modules
        # fcc_print(pymodules)
        for mod in pymodules:
            fcc_print('Try importing {0} ...'.format(mod))
            try:
                im = __import__('{0}'.format(mod))
            except:
                im = False
            if not im:
                # to get an error message what was going wrong
                __import__('{0}'.format(mod))
            self.assertTrue(im, 'Problem importing {0}'.format(mod))

    # ********************************************************************************************
    def tearDown(
        self
    ):
        # clearance, is executed after every test
        FreeCAD.closeDocument(self.doc_name)
        pass
