# ***************************************************************************
# *   Copyright (c) 2002,2003 Jürgen Riegel <juergen.riegel@web.de>         *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Lesser General Public License for more details.                   *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# ***************************************************************************/

# Web gui init module
#
# Gathering all the information to start FreeCAD
# This is the second one of three init scripts, the third one
# runs when the gui is up


class WebWorkbench(Workbench):
    "Web workbench object"

    def __init__(self):
        self.__class__.Icon = FreeCAD.getResourceDir() + "Mod/Web/Resources/icons/WebWorkbench.svg"
        self.__class__.MenuText = "Web"
        self.__class__.ToolTip = "Web workbench"

    def Initialize(self):
        # load the module
        import WebGui

    def GetClassName(self):
        return "WebGui::Workbench"


Gui.addWorkbench(WebWorkbench())

# Append the open handler
FreeCAD.addImportType("Web Page (*.html *.xhtml)", "WebGui")

FreeCAD.__unit_test__ += ["TestWebGui"]
