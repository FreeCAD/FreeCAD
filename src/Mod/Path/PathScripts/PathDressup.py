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

"""Path Dressup object and FreeCAD command"""

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)


class ObjectDressup:
    

    def __init__(self,obj):
        obj.addProperty("App::PropertyLink","Base","Path",translate("PathDressup","The base path to modify"))
        obj.addProperty("App::PropertyInteger","Position","Path",translate("PathDressup","The position of this dressup in the base path"))
        obj.addProperty("Path::PropertyPath","Modification","Path",translate("PathDressup","The modification to be added"))
        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None

    def execute(self,obj):
        
        if obj.Base:
            if obj.Base.isDerivedFrom("Path::Feature"):
                before = []
                after = []
                oldtool = None
                if obj.Base.Path:
                    if obj.Base.Path.Commands:
                        # split the base path
                        before = obj.Base.Path.Commands[:obj.Position]
                        after = obj.Base.Path.Commands[obj.Position:]
                # join everything
                commands = before + obj.Modification.Commands + after
                path = Path.Path(commands)
                obj.Path = path
                
                
class ViewProviderDressup:


    def __init__(self,vobj):
        vobj.Proxy = self

    def attach(self,vobj):
        self.Object = vobj.Object
        return    
    
    def claimChildren(self):
        return [self.Object.Base]

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None


class CommandPathDressup:


    def GetResources(self):
        return {'Pixmap'  : 'Path-Dressup',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Dressup","Dress-up"),
                'Accel': "P, S",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Dressup","Creates a Path Dess-up object from a selected path")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None
        
    def Activated(self):
        
        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) != 1:
            FreeCAD.Console.PrintError(translate("Path_Dressup","Please select one path object\n"))
            return
        if not selection[0].isDerivedFrom("Path::Feature"):
            FreeCAD.Console.PrintError(translate("Path_Dressup","The selected object is not a path\n"))
            return
        if selection[0].isDerivedFrom("Path::FeatureCompoundPython"):
            FreeCAD.Console.PrintError(translate("Path_Dressup", "Please select a Path object"))
            return

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction(translate("Path_Dressup","Create Dress-up"))
        FreeCADGui.addModule("PathScripts.PathDressup")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","Dressup")')
        FreeCADGui.doCommand('PathScripts.PathDressup.ObjectDressup(obj)')
        FreeCADGui.doCommand('obj.Base = FreeCAD.ActiveDocument.' + selection[0].Name)
        FreeCADGui.doCommand('PathScripts.PathDressup.ViewProviderDressup(obj.ViewObject)')
        FreeCADGui.doCommand('PathScripts.PathUtils.addToProject(obj)')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp: 
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Dressup',CommandPathDressup())

FreeCAD.Console.PrintLog("Loading PathDressup... done\n")
