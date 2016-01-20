# -*- coding: utf-8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2015 Yorik van Havre <yorik@uncreated.net>              *
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

"""Path ToolChange object and FreeCAD command"""

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)


class ObjectToolChange:


    def __init__(self,obj):
        obj.addProperty("App::PropertyInteger","ToolNumber","Path","The tool number to be used")
        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None
        
    def execute(self,obj):
        path = Path.Path("M6T"+str(obj.ToolNumber))
        obj.Path = path


class CommandPathToolChange:


    def GetResources(self):
        return {'Pixmap'  : 'Path-ToolChange',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_ToolChange","Tool Change"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_ToolChange","Changes the current tool")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None
        
    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create Tool Change")
        FreeCADGui.addModule("PathScripts.PathToolChange")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","ToolChange")')
        FreeCADGui.doCommand('PathScripts.PathToolChange.ObjectToolChange(obj)')
        FreeCADGui.doCommand('obj.ViewObject.Proxy = 0')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp: 
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_ToolChange',CommandPathToolChange())
