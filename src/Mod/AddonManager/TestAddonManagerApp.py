# -*- coding: utf-8 -*-

# ***************************************************************************
# *   Copyright (c) 2022 FreeCAD Project Association                        *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This library is free software; you can redistribute it and/or         *
# *   modify it under the terms of the GNU Lesser General Public            *
# *   License as published by the Free Software Foundation; either          *
# *   version 2.1 of the License, or (at your option) any later version.    *
# *                                                                         *
# *   This library is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with this library; if not, write to the Free Software   *
# *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA         *
# *   02110-1301  USA                                                       *
# *                                                                         *
# ***************************************************************************

# Unit test for the Addon Manager module
from AddonManagerTest.app.test_utilities import (
    TestUtilities as AddonManagerTestUtilities,
)
from AddonManagerTest.app.test_addon import (
    TestAddon as AddonManagerTestAddon,
)
from AddonManagerTest.app.test_macro import (
    TestMacro as AddonManagerTestMacro,
)

# dummy usage to get flake8 and lgtm quiet
False if AddonManagerTestUtilities.__name__ else True
False if AddonManagerTestAddon.__name__ else True
False if AddonManagerTestMacro.__name__ else True
