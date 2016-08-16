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

import FreeCAD,FreeCADGui
from PySide import QtCore,QtGui

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)

class CommandPathToolTableEdit:
    def GetResources(self):
        return {'Pixmap'  : 'Path-ToolTable',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_ToolTableEdit","EditToolTable"),
                'Accel': "P, T",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_ToolTableEdit","Edits a Tool Table in a selected Project")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction(translate("Path_ToolTableEdit","Edits a Tool Table in a selected Project"))
        FreeCADGui.doCommand("from PathScripts import TooltableEditor")
        FreeCADGui.doCommand("from PathScripts import PathUtils")
        FreeCADGui.doCommand('machine = PathUtils.findMachine()')
        FreeCADGui.doCommand('TooltableEditor.edit(machine.Name)')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

if FreeCAD.GuiUp: 
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_ToolTableEdit',CommandPathToolTableEdit())


FreeCAD.Console.PrintLog("Loading PathToolTableEdit... done\n")

