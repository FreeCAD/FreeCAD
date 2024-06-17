# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2022 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2022-2023 Larry Woestman <LarryWoestman2@gmail.com>     *
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

import Path
import Tests.PathTestUtils as PathTestUtils
from Path.Post.scripts import refactored_test_post as postprocessor


Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


class TestRefactoredTestPostMCodes(PathTestUtils.PathTestBase):
    """Test the refactored_test_post.py postprocessor."""

    @classmethod
    def setUpClass(cls):
        """setUpClass()...

        This method is called upon instantiation of this test class.  Add code
        and objects here that are needed for the duration of the test() methods
        in this class.  In other words, set up the 'global' test environment
        here; use the `setUp()` method to set up a 'local' test environment.
        This method does not have access to the class `self` reference, but it
        is able to call static methods within this same class.
        """
        # Open existing FreeCAD document with test geometry
        FreeCAD.newDocument("Unnamed")

    @classmethod
    def tearDownClass(cls):
        """tearDownClass()...

        This method is called prior to destruction of this test class.  Add
        code and objects here that cleanup the test environment after the
        test() methods in this class have been executed.  This method does
        not have access to the class `self` reference.  This method is able
        to call static methods within this same class.
        """
        # Close geometry document without saving
        FreeCAD.closeDocument(FreeCAD.ActiveDocument.Name)

    # Setup and tear down methods called before and after each unit test

    def setUp(self):
        """setUp()...

        This method is called prior to each `test()` method.  Add code and
        objects here that are needed for multiple `test()` methods.
        """
        self.doc = FreeCAD.ActiveDocument
        self.con = FreeCAD.Console
        self.docobj = FreeCAD.ActiveDocument.addObject("Path::Feature", "testpath")
        #
        # Re-initialize all of the values before doing a test.
        #
        postprocessor.UNITS = "G21"
        postprocessor.init_values(postprocessor.global_values)

    def tearDown(self):
        """tearDown()...

        This method is called after each test() method. Add cleanup instructions here.
        Such cleanup instructions will likely undo those in the setUp() method.
        """
        FreeCAD.ActiveDocument.removeObject("testpath")

    def single_compare(self, path, expected, args, debug=False):
        """Perform a test with a single comparison."""
        nl = "\n"
        self.docobj.Path = Path.Path(path)
        postables = [self.docobj]
        gcode = postprocessor.export(postables, "-", args)
        if debug:
            print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(gcode, expected)

    def compare_third_line(self, path_string, expected, args, debug=False):
        """Perform a test with a single comparison to the third line of the output."""
        nl = "\n"
        if path_string:
            self.docobj.Path = Path.Path([Path.Command(path_string)])
        else:
            self.docobj.Path = Path.Path([])
        postables = [self.docobj]
        gcode = postprocessor.export(postables, "-", args)
        if debug:
            print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(gcode.splitlines()[2], expected)

    #############################################################################
    #
    # The tests are organized into groups:
    #
    #   00000 - 00099  tests that don't fit any other category
    #   00100 - 09999  tests for all of the various arguments/options
    #   10000 - 19999  tests for the various G codes at 10000 + 10 * g_code_value
    #   20000 - 29999  tests for the various M codes at 20000 + 10 * m_code_value
    #
    #############################################################################

    def test20000(self):
        """Test M0 command Generation."""
        self.compare_third_line("M0", "M0", "")
        self.compare_third_line("M00", "M00", "")

    #############################################################################

    def test20010(self):
        """Test M1 command Generation."""
        self.compare_third_line("M1", "M1", "")
        self.compare_third_line("M01", "M01", "")

    #############################################################################

    def test20020(self):
        """Test M2 command Generation."""
        self.compare_third_line("M2", "M2", "")
        self.compare_third_line("M02", "M02", "")

    #############################################################################

    def test20030(self):
        """Test M3 command Generation."""
        self.compare_third_line("M3", "M3", "")
        self.compare_third_line("M03", "M03", "")

    #############################################################################

    def test20040(self):
        """Test M4 command Generation."""
        self.compare_third_line("M4", "M4", "")
        self.compare_third_line("M04", "M04", "")

    #############################################################################

    def test20050(self):
        """Test M5 command Generation."""
        self.compare_third_line("M5", "M5", "")
        self.compare_third_line("M05", "M05", "")

    #############################################################################

    def test20060(self):
        """Test M6 command Generation."""
        self.compare_third_line("M6", "M6", "")
        self.compare_third_line("M06", "M06", "")

    #############################################################################

    def test20070(self):
        """Test M7 command Generation."""
        self.compare_third_line("M7", "M7", "")
        self.compare_third_line("M07", "M07", "")

    #############################################################################

    def test20080(self):
        """Test M8 command Generation."""
        self.compare_third_line("M8", "M8", "")
        self.compare_third_line("M08", "M08", "")

    #############################################################################

    def test20090(self):
        """Test M9 command Generation."""
        self.compare_third_line("M9", "M9", "")
        self.compare_third_line("M09", "M09", "")

    #############################################################################

    def test20300(self):
        """Test M30 command Generation."""
        self.compare_third_line("M30", "M30", "")

    #############################################################################

    def test20480(self):
        """Test M48 command Generation."""
        self.compare_third_line("M48", "M48", "")

    #############################################################################

    def test20490(self):
        """Test M49 command Generation."""
        self.compare_third_line("M49", "M49", "")

    #############################################################################

    def test20600(self):
        """Test M60 command Generation."""
        self.compare_third_line("M60", "M60", "")
