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

class TestArchPipe(TestArchBase.TestArchBase):

    def test_makePipe(self):
        """Test the makePipe function."""
        operation = "Testing makePipe function"
        self.printTestMessage(operation)

        pipe = Arch.makePipe(diameter=200, length=1000, name="TestPipe")
        self.assertIsNotNone(pipe, "makePipe failed to create a pipe object.")
        self.assertEqual(pipe.Label, "TestPipe", "Pipe label is incorrect.")

    def test_makePipeConnector(self):
        """Test the makePipeConnector function."""
        operation = "Testing makePipeConnector function"
        self.printTestMessage(operation)

        pipe1 = Arch.makePipe(diameter=200, length=1000, name="Pipe1")
        pipe2 = Arch.makePipe(diameter=200, length=1000, name="Pipe2")
        connector = Arch.makePipeConnector([pipe1, pipe2], radius=100, name="TestConnector")
        self.assertIsNotNone(connector, "makePipeConnector failed to create a pipe connector object.")
        self.assertEqual(connector.Label, "TestConnector", "Pipe connector label is incorrect.")