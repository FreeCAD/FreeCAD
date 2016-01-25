# -*- coding: utf-8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2015 Dan Falck <ddfalck@gmail.com>                      *
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
''' Used to make GCode from FreeCAD shapes - Wires and Edges/Curves '''

import FreeCAD,FreeCADGui,Path,PathGui
from PathScripts import PathProject
from PySide import QtCore,QtGui

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)

#TODO make the shape parametric
class FromShape:
    
    
    def __init__(self,obj):
        obj.addProperty("App::PropertyLink","Base","Shape",translate("Shape Object","The base Shape of this toolpath"))
        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None

    def execute(self,obj):
        pass


class _ViewProviderFromShape:


    def __init__(self,vobj): #mandatory
        vobj.Proxy = self

    def attach(self, vobj):
        self.Object = vobj.Object

    def __getstate__(self): #mandatory
        return None

    def __setstate__(self,state): #mandatory
        return None

    def getIcon(self): #optional
        return ":/icons/Path-Shape.svg"


class CommandFromShape:
    def GetResources(self):
        return {'Pixmap'  : 'Path-Shape',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_FromShape","Path from a Shape"),
                'Accel': "P, S",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_FromShape","Creates a Path from a wire/curve")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None
        
    def Activated(self):
        
        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) != 1:
            FreeCAD.Console.PrintError(translate("Path_FromShape","Please select exactly one Part-based object\n"))
            return
        if not(selection[0].isDerivedFrom("Part::Feature")):
            FreeCAD.Console.PrintError(translate("Path_FromShape","Please select exactly one Part-based object\n"))
            return
        
        FreeCAD.ActiveDocument.openTransaction(translate("Path_FromShape","Create path from shape"))
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand("obj = FreeCAD.activeDocument().addObject('Path::FeatureShape','PathShape')")
        FreeCADGui.doCommand("obj.Shape = FreeCAD.activeDocument()."+selection[0].Name+".Shape")
        FreeCADGui.doCommand('PathScripts.PathUtils.addToProject(obj)')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

if FreeCAD.GuiUp: 
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_FromShape',CommandFromShape())
