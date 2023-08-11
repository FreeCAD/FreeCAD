#***************************************************************************
#*   Copyright (c) 2018 Yorik van Havre <yorik@uncreated.net>              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

import FreeCAD
import FreeCADGui
from StartPage import StartPage

# template will be given before this script is run
template_name = str(template)

match template_name:
    case "empty_file":
        FreeCADGui.runCommand('Std_New')
        StartPage.postStart()
    case "import_file":
        FreeCADGui.runCommand('Std_New')
        StartPage.postStart()
        FreeCADGui.runCommand("Std_Import")
    case "parametric_part":
        FreeCADGui.runCommand('Std_New')
        FreeCADGui.activateWorkbench("PartDesignWorkbench")
        FreeCADGui.runCommand("PartDesign_Body")
        FreeCADGui.Selection.addSelection('Unnamed','Body')
        FreeCADGui.runCommand('PartDesign_NewSketch',0)
        FreeCADGui.Selection.clearSelection()
        StartPage.postStart(False)
    case "csg_part":
        FreeCADGui.runCommand('Std_New')
        FreeCADGui.activateWorkbench("PartWorkbench")
        StartPage.postStart(False)
    case "2d_draft":
        FreeCADGui.runCommand('Std_New')
        FreeCADGui.activateWorkbench("DraftWorkbench")
        FreeCADGui.runCommand("Std_ViewTop")
        StartPage.postStart(False)
    case "architecture":
        FreeCADGui.runCommand('Std_New')
        FreeCADGui.activateWorkbench("ArchWorkbench")
        StartPage.postStart(False)
