# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2013 Yorik van Havre <yorik@uncreated.net>              *
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

"""Import all Arch module unit tests in GUI mode."""

from bimtests.TestArchImportersGui import TestArchImportersGui
from bimtests.TestArchAxisGui import TestArchAxisGui
from bimtests.TestArchBuildingPartGui import TestArchBuildingPartGui
from bimtests.TestArchStairsGui import TestArchStairsGui
from bimtests.TestArchReportGui import TestArchReportGui
from bimtests.TestArchSiteGui import TestArchSiteGui
from bimtests.TestArchWallGui import TestArchWallGui
from bimtests.TestArchWindowGui import TestArchWindowGui
from bimtests.TestWebGLExportGui import TestWebGLExportGui
from bimtests.TestArchCoveringGui import TestArchCoveringGui

TEST_CLASSES = (
    TestArchImportersGui,
    TestArchAxisGui,
    TestArchBuildingPartGui,
    TestArchStairsGui,
    TestArchReportGui,
    TestArchSiteGui,
    TestArchWallGui,
    TestArchWindowGui,
    TestWebGLExportGui,
    TestArchCoveringGui,
)


def load_tests(loader, _tests, _pattern):
    """Return the complete Arch GUI test suite explicitly."""
    suite = loader.suiteClass()
    for test_class in TEST_CLASSES:
        suite.addTests(loader.loadTestsFromTestCase(test_class))
    return suite
