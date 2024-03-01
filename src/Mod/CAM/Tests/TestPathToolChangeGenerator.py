# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2021 sliptonic <shopinthewoods@gmail.com>               *
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
import Path.Base.Generator.toolchange as generator
import Tests.PathTestUtils as PathTestUtils

Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


class TestPathToolChangeGenerator(PathTestUtils.PathTestBase):
    def test00(self):
        """Test Basic Tool Change Generator Return"""

        args = {
            "toolnumber": 1,
            "toollabel": "My Label",
            "spindlespeed": 500,
            "spindledirection": generator.SpindleDirection.OFF,
        }

        results = generator.generate(**args)

        # Get a label
        self.assertTrue(len(results) == 2)
        commentcommand = results[0]
        self.assertTrue(isinstance(commentcommand, Path.Command))
        self.assertTrue(commentcommand.toGCode() == "(My Label)")

        # Get a tool command
        toolcommand = results[1]
        self.assertTrue(toolcommand.Name == "M6")

        # Turn on the spindle
        args["spindledirection"] = generator.SpindleDirection.CW
        results = generator.generate(**args)
        self.assertTrue(len(results) == 3)

        speedcommand = results[2]
        self.assertTrue(speedcommand.Name == "M3")
        self.assertTrue(speedcommand.Parameters["S"] == 500)

        # speed zero with spindle on
        args["spindlespeed"] = 0
        results = generator.generate(**args)
        self.assertTrue(len(results) == 2)
        Path.Log.track(results)

        # negative spindlespeed
        args["spindlespeed"] = -10
        self.assertRaises(ValueError, generator.generate, **args)
