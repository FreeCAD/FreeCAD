#***************************************************************************
#*   Copyright (c) 2024 Yorik van Havre <yorik@uncreated.net>              *
#*                                                                         *
#*   This file is part of the FreeCAD CAx development system.              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful,            *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with FreeCAD; if not, write to the Free Software        *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************/

# Unit test for the Help module

import unittest
import Help

class HelpTest(unittest.TestCase):

    def setUp(self):
        self.BUGGY_QT = False

    def test_awebwidgets(self):
        import PySide
        from PySide import QtGui, QtWebEngineWidgets
        # workaround Qt5.12 bug
        if PySide.__version_info__[0] == 5:
            if PySide.__version_info__[1] <= 15:
                Help.WEBWB = True
                self.BUGGY_QT = True

    def test_browser1(self):
        if not self.BUGGY_QT:
            print("Help: Opening an external browser")
            Help.show("https://raw.githubusercontent.com/FreeCAD/FreeCAD-documentation/main/wiki/Draft_Line.md", mode=1)

    def test_browser2(self):
        if not self.BUGGY_QT:
            print("Help: Opening an external browser")
            Help.show("https://wiki.freecadweb.org/Draft_Line", mode=1)

    def test_dialog(self):
        if not self.BUGGY_QT:
            print("Help: Opening a standaline dialog")
            Help.show("Draft_Line", mode=2)
    def test_tab(self):
        if not self.BUGGY_QT:
            print("Help: Opening an MDI tab")
            Help.show("Draft_Line", mode=3)

    def tearDown(self):
        pass
