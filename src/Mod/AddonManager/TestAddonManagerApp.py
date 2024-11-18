# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022-2023 FreeCAD Project Association                   *
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

import addonmanager_freecad_interface as fci

# Unit tests for the Addon Manager module
from AddonManagerTest.app.test_utilities import (
    TestUtilities as AddonManagerTestUtilities,
)
from AddonManagerTest.app.test_addon import (
    TestAddon as AddonManagerTestAddon,
)
from AddonManagerTest.app.test_cache import (
    TestCache as AddonManagerTestCache,
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
from AddonManagerTest.app.test_freecad_interface import (
    TestConsole as AddonManagerTestConsole,
    TestParameters as AddonManagerTestParameters,
    TestDataPaths as AddonManagerTestDataPaths,
)
from AddonManagerTest.app.test_metadata import (
    TestDependencyType as AddonManagerTestDependencyType,
    TestMetadataReader as AddonManagerTestMetadataReader,
    TestMetadataReaderIntegration as AddonManagerTestMetadataReaderIntegration,
    TestUrlType as AddonManagerTestUrlType,
    TestVersion as AddonManagerTestVersion,
    TestMetadataAuxiliaryFunctions as AddonManagerTestMetadataAuxiliaryFunctions,
)


class TestListTerminator:
    pass


# Basic usage mostly to get static analyzers to stop complaining about unused imports
try:
    import FreeCAD
except ImportError:
    FreeCAD = None
loaded_gui_tests = [
    AddonManagerTestUtilities,
    AddonManagerTestAddon,
    AddonManagerTestCache,
    AddonManagerTestMacro,
    AddonManagerTestGit,
    AddonManagerTestAddonInstaller,
    AddonManagerTestMacroInstaller,
    AddonManagerTestDependencyInstaller,
    AddonManagerTestAddonUninstaller,
    AddonManagerTestMacroUninstaller,
    AddonManagerTestConsole,
    AddonManagerTestParameters,
    AddonManagerTestDataPaths,
    AddonManagerTestDependencyType,
    AddonManagerTestMetadataReader,
    AddonManagerTestMetadataReaderIntegration,
    AddonManagerTestUrlType,
    AddonManagerTestVersion,
    AddonManagerTestMetadataAuxiliaryFunctions,
    TestListTerminator,  # Needed to prevent the last test from running twice
]
for test in loaded_gui_tests:
    fci.Console.PrintLog(f"Loaded tests from {test.__name__}\n")
