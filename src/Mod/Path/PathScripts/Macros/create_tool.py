#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2014 Daniel Falck  <ddfalck@gmail.com>                  *  
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

'''
This macro is used in conjunction with the toolpathparams script to create an object that represents a tool for use in a CNC program. Create a group and then select it- then run the macro.
You will have to edit the parameters inside the Data tab of the tool object.
'''

import FreeCAD
import FreeCADGui
import PathScripts
import toolpathparams as tp

tl = FreeCAD.ActiveDocument.addObject("App::FeaturePython","Tools")

tp.ToolParams(tl)
tp.ViewProviderToolParams(tl.ViewObject)

sel = FreeCADGui.Selection.getSelection()
g = sel[0]
g.addObject(tl)
App.activeDocument().recompute()
