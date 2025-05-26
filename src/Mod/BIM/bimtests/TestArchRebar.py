# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 Furgo                                              *
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

import Arch
from bimtests import TestArchBase

class TestArchRebar(TestArchBase.TestArchBase):

    # TODO: remove NOT_ prefix once it is understood why Arch.makeRebar fails
    # Check https://wiki.freecad.org/Arch_Rebar#Scripting
    def NOT_test_makeRebar(self):
        """Test the makeRebar function."""
        operation = "Testing makeRebar function"
        self.printTestMessage(operation)

        rebar = Arch.makeRebar(diameter=16, amount=5, name="TestRebar")
        self.assertIsNotNone(rebar, "makeRebar failed to create a rebar object.")
        self.assertEqual(rebar.Label, "TestRebar", "Rebar label is incorrect.")