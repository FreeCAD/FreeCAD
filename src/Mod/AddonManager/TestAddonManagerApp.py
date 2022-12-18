# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022 FreeCAD Project Association                        *
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

# Unit tests for the Addon Manager module
from AddonManagerTest.app.test_utilities import (
    TestUtilities as AddonManagerTestUtilities,
)
from AddonManagerTest.app.test_addon import (
    TestAddon as AddonManagerTestAddon,
)
from AddonManagerTest.app.test_macro import (
    TestMacro as AddonManagerTestMacro,
)
from AddonManagerTest.app.test_git import (
    TestGit as AddonManagerTestGit,
)
from AddonManagerTest.app.test_installer import (
    TestAddonInstaller as AddonManagerTestAddonInstaller,
    TestMacroInstaller as AddonManagerTestMacroInstaller,
)
from AddonManagerTest.app.test_dependency_installer import (
    TestDependencyInstaller as AddonManagerTestDependencyInstaller,
)
from AddonManagerTest.app.test_uninstaller import (
    TestAddonUninstaller as AddonManagerTestAddonUninstaller,
    TestMacroUninstaller as AddonManagerTestMacroUninstaller,
)

# dummy usage to get flake8 and lgtm quiet
False if AddonManagerTestUtilities.__name__ else True
False if AddonManagerTestAddon.__name__ else True
False if AddonManagerTestMacro.__name__ else True
False if AddonManagerTestGit.__name__ else True
False if AddonManagerTestAddonInstaller.__name__ else True
False if AddonManagerTestMacroInstaller.__name__ else True
False if AddonManagerTestDependencyInstaller.__name__ else True
False if AddonManagerTestAddonUninstaller.__name__ else True
False if AddonManagerTestMacroUninstaller.__name__ else True
