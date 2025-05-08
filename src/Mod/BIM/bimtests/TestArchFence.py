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

import FreeCAD as App
import Arch
import Draft
from bimtests import TestArchBase

class TestArchFence(TestArchBase.TestArchBase):

    def test_makeFence(self):
        # Create section, post, and path objects
        section = Draft.makeRectangle(100, 10)
        post = Draft.makeRectangle(10, 10)
        path = Draft.makeLine(App.Vector(0, 0, 0), App.Vector(1000, 0, 0))

        # Create a fence
        fence = Arch.makeFence(section, post, path)
        self.assertIsNotNone(fence, "Failed to create fence")
        self.assertEqual(fence.Section, section, "Fence section is incorrect")
        self.assertEqual(fence.Post, post, "Fence post is incorrect")
        self.assertEqual(fence.Path, path, "Fence path is incorrect")
