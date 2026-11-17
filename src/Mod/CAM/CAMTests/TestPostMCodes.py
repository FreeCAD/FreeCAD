# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2022 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2022-2025 Larry Woestman <LarryWoestman2@gmail.com>     *
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


import Path
import CAMTests.PostTestSupport as PostTestSupport

Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


class TestPostMCodes(PostTestSupport.LegacyPostTestBase):
    """Test the test_post.py postprocessor M codes."""

    POST_NAME = "test"

    #############################################################################
    #
    # The tests are organized into groups:
    #
    #   00000 - 00099  tests that don't fit any other category
    #   00100 - 09999  tests for all of the various arguments/options
    #   10000 - 18999  tests for the various G codes at 10000 + 10 * g_code_value
    #   19000 - 19999  tests for the A, B, and C axis outputs
    #   20000 - 29999  tests for the various M codes at 20000 + 10 * m_code_value
    #
    #############################################################################

    def test20000(self):
        """Test M0 command Generation."""
        self.single_compare("M0", "M0", "")
        self.single_compare("M00", "M00", "")

    #############################################################################

    def test20010(self):
        """Test M1 command Generation."""
        self.single_compare("M1", "M1", "")
        self.single_compare("M01", "M01", "")

    #############################################################################

    def test20020(self):
        """Test M2 command Generation."""
        self.single_compare("M2", "M2", "")
        self.single_compare("M02", "M02", "")

    #############################################################################

    def test20030(self):
        """Test M3 command Generation."""
        self.single_compare("M3", "M3", "")
        self.single_compare("M03", "M03", "")

    #############################################################################

    def test20040(self):
        """Test M4 command Generation."""
        self.single_compare("M4", "M4", "")
        self.single_compare("M04", "M04", "")

    #############################################################################

    def test20050(self):
        """Test M5 command Generation."""
        self.single_compare("M5", "M5", "")
        self.single_compare("M05", "M05", "")

    #############################################################################

    def test20060(self):
        """Test M6 command Generation."""

        c = Path.Command("M6 T2")

        self.profile_op.Path = Path.Path([c])

        self.job.PostProcessorArgs = "--tool_change"
        gcode = self.post.export()[0][1]
        split_gcode = gcode.splitlines()
        # print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(split_gcode[4], "M6 T2")

        c = Path.Command("M06 T02")

        self.profile_op.Path = Path.Path([c])

        self.job.PostProcessorArgs = "--tool_change"
        gcode = self.post.export()[0][1]
        split_gcode = gcode.splitlines()
        # print(f"--------{nl}{gcode}--------{nl}")
        self.assertEqual(split_gcode[4], "M06 T2")

    #############################################################################

    def test20070(self):
        """Test M7 command Generation."""
        self.single_compare("M7", "M7", "")
        self.single_compare("M07", "M07", "")

    #############################################################################

    def test20080(self):
        """Test M8 command Generation."""
        self.single_compare("M8", "M8", "")
        self.single_compare("M08", "M08", "")

    #############################################################################

    def test20090(self):
        """Test M9 command Generation."""
        self.single_compare("M9", "M9", "")
        self.single_compare("M09", "M09", "")

    #############################################################################

    def test20300(self):
        """Test M30 command Generation."""
        self.single_compare("M30", "M30", "")

    #############################################################################

    def test20480(self):
        """Test M48 command Generation."""
        self.single_compare("M48", "M48", "")

    #############################################################################

    def test20490(self):
        """Test M49 command Generation."""
        self.single_compare("M49", "M49", "")

    #############################################################################

    def test20600(self):
        """Test M60 command Generation."""
        self.single_compare("M60", "M60", "")
