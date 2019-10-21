# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
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
from PySide import QtCore

class CommandToolBitCreate:
    '''
    Command used to create a new Tool.
    '''

    def __init__(self):
        pass

    def GetResources(self):
        return {'Pixmap': 'Path-ToolBit',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Tool", "Create Tool"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Job", "Creates a new ToolBit object")}

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def Activated(self):
        import PathScripts.PathToolBitGui as PathToolBitGui
        obj = PathToolBitGui.Create()
        obj.ViewObject.Proxy.setCreate(obj.ViewObject)

if FreeCAD.GuiUp:
    import FreeCADGui
    FreeCADGui.addCommand('Path_ToolBitCreate', CommandToolBitCreate())

FreeCAD.Console.PrintLog("Loading PathToolBitCmd... done\n")
