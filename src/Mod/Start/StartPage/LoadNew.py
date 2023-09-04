# ***************************************************************************
# *   Copyright (c) 2018 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import FreeCAD
import FreeCADGui
from StartPage import StartPage

# template will be given before this script is run
template_name = str(template)

if template_name == "empty_file":
    FreeCADGui.runCommand("Std_New")
    StartPage.postStart()
elif template_name == "open_file":
    previous_doc = FreeCADGui.ActiveDocument
    FreeCADGui.runCommand("Std_Open")
    # workaround to not run postStart() if user cancels the Open dialog
    if FreeCADGui.ActiveDocument != previous_doc:
        StartPage.postStart()
elif template_name == "parametric_part":
    FreeCADGui.runCommand("Std_New")
    FreeCADGui.activateWorkbench("PartDesignWorkbench")
    FreeCADGui.runCommand("PartDesign_Body")
    StartPage.postStart(False)
# elif template_name == "csg_part":
#         FreeCADGui.runCommand('Std_New')
#         FreeCADGui.activateWorkbench("PartWorkbench")
#         StartPage.postStart(False)
elif template_name == "2d_draft":
    FreeCADGui.runCommand("Std_New")
    FreeCADGui.activateWorkbench("DraftWorkbench")
    FreeCADGui.runCommand("Std_ViewTop")
    StartPage.postStart(False)
elif template_name == "architecture":
    FreeCADGui.runCommand("Std_New")
    try:
        import BimCommands
    except Exception:
        FreeCADGui.activateWorkbench("ArchWorkbench")
    else:
        FreeCADGui.activateWorkbench("BIMWorkbench")
    StartPage.postStart(False)
