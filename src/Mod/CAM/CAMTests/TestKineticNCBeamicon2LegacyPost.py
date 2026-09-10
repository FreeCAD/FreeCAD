# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

from importlib import reload

import FreeCAD

import Path
import CAMTests.PathTestUtils as PathTestUtils
from Path.Post.scripts import KineticNCBeamicon2_legacy_post as postprocessor

Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


class TestKineticNCBeamicon2LegacyPost(PathTestUtils.PathTestBase):
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
        FreeCAD.closeDocument(FreeCAD.ActiveDocument.Name)

    def setUp(self):
        """setUp()...
        This method is called prior to each `test()` method.  Add code and
        objects here that are needed for multiple `test()` methods.
        """
        self.doc = FreeCAD.ActiveDocument
        self.con = FreeCAD.Console
        self.docobj = FreeCAD.ActiveDocument.addObject("Path::Feature", "testpath")
        reload(
            postprocessor
        )  # technical debt.  This shouldn't be necessary but here to bypass a bug

    def tearDown(self):
        """tearDown()...
        This method is called after each test() method. Add cleanup instructions here.
        Such cleanup instructions will likely undo those in the setUp() method.
        """
        FreeCAD.ActiveDocument.removeObject("testpath")

    def test000(self):
        """Test Output Generation.
        Empty path.  Produces only the preamble and postamble.
        """

        self.docobj.Path = Path.Path([])
        postables = [self.docobj]

        expected = """(begin preamble)
%
G17 G21 G40 G49 G80 G90
G21
(begin operation: testpath)
(machine: not set, mm/min)
(finish operation: testpath)
(begin postamble)
M05
M09
G17 G90 G80 G40
M30
"""

        args = "--no-header --no-show-editor"
        gcode = postprocessor.export(postables, "-", args)
        self.assertEqual(gcode, expected)

    def test010(self):
        """Test that the default preamble and postamble survive an empty argstring.

        Regression guard for issue #31387: argparse defaults of "" for
        --preamble/--postamble silently overwrote the module defaults, which
        dropped the spindle stop (M05) and program end (M30) from every file
        this post produced.
        """

        self.docobj.Path = Path.Path([])
        postables = [self.docobj]

        args = "--no-header --no-comments --no-show-editor"
        gcode = postprocessor.export(postables, "-", args)

        self.assertEqual(
            gcode,
            """%
G17 G21 G40 G49 G80 G90
G21
M05
M09
G17 G90 G80 G40
M30
""",
        )

    def test020(self):
        """Test that an explicit preamble and postamble still override the defaults."""

        self.docobj.Path = Path.Path([])
        postables = [self.docobj]

        args = "--no-header --no-comments --no-show-editor "
        args += "--preamble='G18 G55' --postamble='G0 Z50\nM2'"
        gcode = postprocessor.export(postables, "-", args)
        lines = gcode.splitlines()

        self.assertEqual(lines[0], "G18 G55")
        self.assertEqual(lines[-2], "G0 Z50")
        self.assertEqual(lines[-1], "M2")

    def test030(self):
        """Test that the argument help text is well formed.

        The default preamble starts with "%", which argparse treats as a
        format specifier when it expands a help string.  An unescaped "%"
        makes parser.format_help() -- used to build TOOLTIP_ARGS -- raise
        ValueError: badly formed help string.
        """

        tooltip_args = postprocessor.parser.format_help()
        self.assertIn("--preamble", tooltip_args)
        self.assertIn("--postamble", tooltip_args)
