# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2023 Yorik van Havre
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

"""Unit tests for the Native IFC module that require the GUI"""

import FreeCAD

from bimtests import TestArchBaseGui
from bimtests.TestNativeIfc import SINGLEDOC, getIfcFilePath

from nativeifc import ifc_import
from nativeifc import ifc_tools


class TestNativeIfcGui(TestArchBaseGui.TestArchBaseGui):

    def test12_RemoveObject(self):
        from nativeifc import ifc_observer

        ifc_observer.add_observer()
        FreeCAD.Console.PrintMessage("NativeIFC 12: Remove object...")
        fp = getIfcFilePath()
        ifc_import.insert(
            fp,
            self.doc_name,
            strategy=2,
            shapemode=0,
            switchwb=0,
            silent=True,
            singledoc=SINGLEDOC,
        )
        ifcfile = ifc_tools.get_ifcfile(self.document.Objects[-1])
        count1 = len(ifcfile.by_type("IfcProduct"))
        self.document.removeObject("IfcObject004")
        count2 = len(ifcfile.by_type("IfcProduct"))
        self.assertTrue(count2 < count1, "RemoveObject failed")
