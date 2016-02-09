# -*- coding: utf-8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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

import FreeCAD,FreeCADGui,Path,PathGui
from PySide import QtCore,QtGui

"""Path Copy object and FreeCAD command"""

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)


class ObjectPathCopy:
    

    def __init__(self,obj):
        obj.addProperty("App::PropertyLink","Base","Path",translate("PathCopy","The path to be copied"))
        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None
        
    def execute(self,obj):
        if obj.Base:
            if obj.Base.Path:
                obj.Path = obj.Base.Path.copy()


class ViewProviderPathCopy:

    def __init__(self,vobj):
        vobj.Proxy = self

    def attach(self,vobj):
        self.Object = vobj.Object
        return

    def getIcon(self):
        return ":/icons/Path-Copy.svg"

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None


class CommandPathCopy:


    def GetResources(self):
        return {'Pixmap'  : 'Path-Copy',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Copy","Copy"),
                'Accel': "P, Y",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Copy","Creates a linked copy of another path")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None
        
    def Activated(self):

        FreeCAD.ActiveDocument.openTransaction(translate("Path_Copy","Create Copy"))
        FreeCADGui.addModule("PathScripts.PathCopy")

        consolecode = '''
import Path
import PathScripts
from PathScripts import PathCopy
selGood = True
# check that the selection contains exactly what we want
selection = FreeCADGui.Selection.getSelection()
proj = selection[0].InList[0] #get the group that the selectied object is inside

if len(selection) != 1:
    FreeCAD.Console.PrintError(translate("PathCopy","Please select one path object\\n"))
    selGood = False

if not selection[0].isDerivedFrom("Path::Feature"):
    FreeCAD.Console.PrintError(translate("PathCopy","The selected object is not a path\\n"))
    selGood = False

if selGood:
    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", str(selection[0].Name)+'_Copy')
    PathScripts.PathCopy.ObjectPathCopy(obj)
    PathScripts.PathCopy.ViewProviderPathCopy(obj.ViewObject)
    obj.Base = FreeCAD.ActiveDocument.getObject(selection[0].Name)

g = proj.Group
g.append(obj)
proj.Group = g

FreeCAD.ActiveDocument.recompute()

'''

        FreeCADGui.doCommand(consolecode)
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp: 
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Copy',CommandPathCopy())

FreeCAD.Console.PrintLog("Loading PathCopy... done\n")
